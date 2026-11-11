// K-D tree and brute force nearest neighbor search (Euclidean distance, any dimension k).
// Build: g++ -std=c++17 -O2 -o kdtree kdtree.cpp

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <tuple>
#include <vector>

using Point = std::vector<double>;

// Squared Euclidean distance (avoids sqrt; ordering is preserved).
double sqDist(const Point& a, const Point& b) {
    double s = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        double d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

double euclidean(const Point& a, const Point& b) { return std::sqrt(sqDist(a, b)); }


// Brute force: check every point, return the closest (nullopt if empty).

std::optional<Point> bruteForceNN(const std::vector<Point>& points, const Point& query) {
    if (points.empty()) return std::nullopt;
    const Point* best = nullptr;
    double bestD = std::numeric_limits<double>::infinity();
    for (const Point& p : points) {
        double d = sqDist(p, query);
        if (d < bestD) {
            bestD = d;
            best = &p;
        }
    }
    return *best;
}

// K-D tree
class KDTree {
public:
    // Builds a tree from points (empty tree if the list is empty).
    explicit KDTree(std::vector<Point> points = {}) {
        if (points.empty()) return;
        k_ = points[0].size();
        for (const Point& p : points)
            if (p.size() != k_) throw std::invalid_argument("All points must have the same dimension");
        root_ = build(points.begin(), points.end(), 0);
    }

    // Returns the point in the tree closest to query (nullopt if tree is empty).
    std::optional<Point> nearestNeighbor(const Point& query) const {
        if (!root_) return std::nullopt;
        if (query.size() != k_) throw std::invalid_argument("Query dimension does not match tree dimension");
        const Point* best = nullptr;
        double bestD = std::numeric_limits<double>::infinity();
        search(root_.get(), query, best, bestD);
        return *best;
    }

private:
    struct Node {
        Point point;
        size_t axis;
        std::unique_ptr<Node> left, right;
    };

    using Iter = std::vector<Point>::iterator;

    size_t k_ = 0;
    std::unique_ptr<Node> root_;

    std::unique_ptr<Node> build(Iter first, Iter last, size_t depth) {
        if (first == last) return nullptr;
        size_t axis = depth % k_;
        Iter mid = first + (last - first) / 2;  // median -> balanced tree
        std::nth_element(first, mid, last,
                         [axis](const Point& a, const Point& b) { return a[axis] < b[axis]; });
        auto node = std::make_unique<Node>();
        node->point = *mid;
        node->axis = axis;
        node->left = build(first, mid, depth + 1);
        node->right = build(mid + 1, last, depth + 1);
        return node;
    }

    void search(const Node* node, const Point& query, const Point*& best, double& bestD) const {
        if (!node) return;

        // Check the current node.
        double d = sqDist(node->point, query);
        if (d < bestD) {
            bestD = d;
            best = &node->point;
        }

        // Descend first into the side of the splitting plane containing the query.
        double diff = query[node->axis] - node->point[node->axis];
        const Node* nearSide = diff < 0 ? node->left.get() : node->right.get();
        const Node* farSide = diff < 0 ? node->right.get() : node->left.get();
        search(nearSide, query, best, bestD);

        // Only search the far side if the splitting plane is closer than the best so far.
        if (diff * diff < bestD) search(farSide, query, best, bestD);
    }
};

// Benchmarking
using Duration = std::chrono::nanoseconds;

/**
 * Benchmark KDTree against brute force nearest neighbor.
 * 1000 test points are generated to test against the training points.
 *
 * @param k          the dimensionality of the dataset to create
 * @param numPoints  the number of points to use to match against
 *                   (1000 test points will be used)
 * @return a triple of Durations: (time to build the tree,
 *         time to query the tree with 1,000 points,
 *         time for brute force to answer the same 1,000 queries)
 */
std::tuple<Duration, Duration, Duration> runExperiment(int k, int numPoints) {
    const int kNumTests = 1000;
    using Clock = std::chrono::steady_clock;

    std::mt19937 rng(12345);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    auto randomPoint = [&]() {
        Point p(k);
        for (auto& x : p) x = dist(rng);
        return p;
    };

    // Data generation is not timed.
    std::vector<Point> train(numPoints), tests(kNumTests);
    for (auto& p : train) p = randomPoint();
    for (auto& p : tests) p = randomPoint();

    // sink keeps the compiler from optimizing away the queries.
    double sink = 0.0;

    // 1. Build the tree.
    auto t0 = Clock::now();
    KDTree tree(train);
    auto t1 = Clock::now();

    // 2. Query the tree.
    for (const Point& q : tests) sink += (*tree.nearestNeighbor(q))[0];
    auto t2 = Clock::now();

    // 3. Brute force.
    for (const Point& q : tests) sink += (*bruteForceNN(train, q))[0];
    auto t3 = Clock::now();

    return {std::chrono::duration_cast<Duration>(t1 - t0),
            std::chrono::duration_cast<Duration>(t2 - t1),
            std::chrono::duration_cast<Duration>(t3 - t2)};
}

// Self-test + benchmark: K-D tree must agree with brute force
int main() {
    std::mt19937 rng(0);
    std::uniform_real_distribution<double> dist(-100.0, 100.0);

    for (size_t k : {1, 2, 3, 5, 10}) {
        std::vector<Point> pts(2000, Point(k));
        for (auto& p : pts)
            for (auto& x : p) x = dist(rng);

        KDTree tree(pts);
        for (int t = 0; t < 200; ++t) {
            Point q(k);
            for (auto& x : q) x = dist(rng);
            auto kd = tree.nearestNeighbor(q);
            auto bf = bruteForceNN(pts, q);
            assert(kd && bf && euclidean(*kd, q) == euclidean(*bf, q));
        }
        std::cout << "k=" << k << ": K-D tree matches brute force on 200 queries\n";
    }

    // Small example
    std::vector<Point> pts = {{2, 3}, {5, 4}, {9, 6}, {4, 7}, {8, 1}, {7, 2}};
    KDTree tree(pts);
    Point q = {9, 2};
    auto kd = tree.nearestNeighbor(q);
    auto bf = bruteForceNN(pts, q);
    std::cout << "K-D tree     : (" << (*kd)[0] << ", " << (*kd)[1] << ")\n";
    std::cout << "Brute force  : (" << (*bf)[0] << ", " << (*bf)[1] << ")\n\n";

    // Benchmark (times in milliseconds)
    auto ms = [](Duration d) { return std::chrono::duration<double, std::milli>(d).count(); };
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::setw(4) << "k" << std::setw(10) << "n" << std::setw(14) << "build (ms)"
              << std::setw(14) << "kd query (ms)" << std::setw(16) << "brute (ms)" << "\n";
    for (int k : {2, 5, 10, 20}) {
        for (int n : {1000, 10000, 100000}) {
            auto [build, query, brute] = runExperiment(k, n);
            std::cout << std::setw(4) << k << std::setw(10) << n << std::setw(14) << ms(build)
                      << std::setw(14) << ms(query) << std::setw(16) << ms(brute) << "\n";
        }
    }
    return 0;
}