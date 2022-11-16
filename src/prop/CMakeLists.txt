file(GLOB SRCS "*.cpp"
prop/minisat/core/Solver.cc
prop/minisat/simp/SimpSolver.cc
)

file(GLOB HEADERS "*.h"
  prop/minisat/core/Dimacs.h
  prop/minisat/core/Solver.h
  prop/minisat/core/SolverTypes.h
  prop/minisat/minisat.cpp
  prop/minisat/minisat.h
  prop/minisat/mtl/Alg.h
  prop/minisat/mtl/Alloc.h
  prop/minisat/mtl/Heap.h
  prop/minisat/mtl/IntTypes.h
  prop/minisat/mtl/Map.h
  prop/minisat/mtl/Queue.h
  prop/minisat/mtl/Sort.h
  prop/minisat/mtl/Vec.h
  prop/minisat/mtl/XAlloc.h
  prop/minisat/simp/SimpSolver.h
  prop/minisat/utils/Options.h
)


target_sources(${MARABOU_LIB} PRIVATE ${SRCS})
target_include_directories(${MARABOU_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if (${BUILD_PYTHON})
    target_include_directories(${MARABOU_PY} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
