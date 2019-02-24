NAME      lp_feasible_1
ROWS
 N  obj 
 E  e1
 E  e2
 E  e3
COLUMNS
    x0        e1                   1   e2                  1
    x1        e1                  -1   e3                 -1
    x2        e2                   1   e3                 -1
    x3        e3                   1
RHS
    rhs       e1                   0
    rhs       e2                   0
    rhs       e3                   0
BOUNDS
 LO bnd       x0                   0
 LO bnd       x1                   0
 LO bnd       x2                  -1
 LO bnd       x3                 0.5
 UP bnd       x0                   1
 UP bnd       x1                   1
 UP bnd       x2                   0
 UP bnd       x3                   1
ENDATA
