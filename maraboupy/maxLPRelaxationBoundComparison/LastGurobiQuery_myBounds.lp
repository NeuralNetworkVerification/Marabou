\ LP format - for model browsing. Use MPS format to capture full model detail.
Minimize
  x9
Subject To
 R0: x0 - x1 - x4 = 0
 R1: x1 - x2 - x5 = 0
 R2: x2 - x3 - x6 = 0
 R3: - x4 + x7 >= 0
 R4: - x5 + x7 >= 0
 R5: x7 <= 3
 R6: - x4 - 0.8333333333333334 x5 + x7 <= 2.5
 R7: - 0.1666666666666667 x5 + x7 <= 2.5
 R8: - x5 + x8 >= 0
 R9: - x6 + x8 >= 0
 R10: x8 <= 4
 R11: - x5 - 0.875 x6 + x8 <= 3.5
 R12: - 0.125 x6 + x8 <= 3.5
 R13: x7 + x8 - x9 = 0
Bounds
 -1 <= x0 <= 1
 -1 <= x1 <= 1
 -2 <= x2 <= 2
 -2 <= x3 <= 2
 -2 <= x4 <= 2
 -3 <= x5 <= 3
 -4 <= x6 <= 4
 -1.5 <= x7 <= 3
 -1.5 <= x8 <= 4
 -5 <= x9 <= 7
End
