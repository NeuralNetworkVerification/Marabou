
marabou_add_acasxu_test(2 "ACASXU_experimental_v2a_3_7" "4" unsat)
marabou_add_coav_test(2 "reluBenchmark2.66962385178s_UNSAT.nnet" unsat) #25 sec
marabou_add_acasxu_dnc_test(1 "ACASXU_experimental_v2a_3_9" "3" unsat) #

 marabou_add_coav_test(2 "reluBenchmark2.64361000061s_SAT.nnet" sat) # 25sec
# marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_5_1" "2" sat) # 480sec
# marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_2_4" "2" sat) # 300sec
# marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_3_7" "4" unsat) # 150sec
# marabou_add_acasxu_dnc_test(1 "ACASXU_experimental_v2a_3_5" "2" sat) # timeout
# marabou_add_acasxu_dnc_test(1 "ACASXU_experimental_v2a_3_9" "3" unsat) # 200sec

#marabou_add_mnist_test(2 "mnist10x10.nnet" "image3_target8_epsilon0.1.txt" sat) #16sec
marabou_add_mnist_test(2 "mnist10x10.nnet" "image3_target9_epsilon0.005.txt" unsat) #17sec

# Add all input query files in regress2/input_queries/ as tests.

file(GLOB sat_ipqs "input_queries/*_sat.ipq")
foreach(file ${sat_ipqs})
  get_filename_component(basename ${file} NAME [CACHE])
  marabou_add_input_query_test(2 ${basename} sat "--verbosity=0" "ipq")
  if (${ENABLE_GUROBI})
    marabou_add_input_query_test(2 ${basename} sat "--milp" "ipq")
  endif()
endforeach()

file(GLOB unsat_ipqs "input_queries/*_unsat.ipq")
foreach(file ${unsat_ipqs})
  get_filename_component(basename ${file} NAME [CACHE])
  marabou_add_input_query_test(2 ${basename} unsat "--verbosity=0" "ipq")
  if (${ENABLE_GUROBI})
    marabou_add_input_query_test(2 ${basename} unsat "--milp" "ipq")
  endif()
endforeach()
