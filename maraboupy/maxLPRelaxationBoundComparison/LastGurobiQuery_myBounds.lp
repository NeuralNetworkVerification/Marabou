\ LP format - for model browsing. Use MPS format to capture full model detail.
Minimize
  x9
Subject To
 R0: x0 - 2 x1 - x4 = 0
 R1: x1 - 2 x2 - x5 = 0
 R2: x2 - 2 x3 - x6 = 0
 R3: - x4 + x7 >= 0
 R4: - x5 + x7 >= 0
 R5: x7 <= 5
 R6: - x4 - 0.8 x5 + x7 <= 4
 R7: - 0.2 x5 + x7 <= 4
 R8: - x5 + x8 >= 0
 R9: - x6 + x8 >= 0
 R10: x8 <= 6
 R11: - x5 - 0.9166666666666666 x6 + x8 <= 5.5
 R12: - 0.0833333333333333 x6 + x8 <= 5.5
 R13: x7 + x8 - x9 = 0
Bounds
 -1 <= x0 <= 1
 -1 <= x1 <= 1
 -2 <= x2 <= 2
 -2 <= x3 <= 2
 -3 <= x4 <= 3
 -5 <= x5 <= 5
 -6 <= x6 <= 6
 -3 <= x7 <= 5
 -3 <= x8 <= 6
 -8 <= x9 <= 11
End
