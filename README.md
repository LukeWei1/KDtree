| k | n | build (ms) | kd query (ms) | brute (ms) |
|---|---|---|---|---|
| 2 | 1,000 | 0.15 | 0.19 | 1.29 |
| 2 | 10,000 | 2.54 | 0.37 | 12.45 |
| 2 | 100,000 | 34.04 | 1.13 | 119.96 |
| 5 | 1,000 | 0.14 | 0.88 | 1.47 |
| 5 | 10,000 | 2.05 | 1.45 | 15.55 |
| 5 | 100,000 | 25.97 | 4.49 | 172.85 |
| 10 | 1,000 | 0.13 | 7.67 | 2.54 |
| 10 | 10,000 | 2.17 | 30.61 | 28.77 |
| 10 | 100,000 | 31.87 | 101.91 | 315.35 |
| 20 | 1,000 | 0.17 | 11.71 | 11.16 |
| 20 | 10,000 | 3.71 | 169.56 | 55.72 |
| 20 | 100,000 | 42.25 | 4038.36 | 1828.41 |

At low dimensionality (small values k), kd trees outperform brute force. When k = 2, the kd tree consistently outperformed brute force searching.However, at larger dimensionality (large values k), brute force search actually starts to outperform kd trees. When k = 10, brute force was able to outperform kd tree at low values of n (1000 and 10,000). At even larger values of k (k=20) brute force out performs kd tree at all values of n tested (1,000 10,000 and 100,000)
