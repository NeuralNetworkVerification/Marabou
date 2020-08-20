marabou_add_regress_mps_test(0 "lp_feasible_1.mps" "sat" "" "mps")
marabou_add_regress_mps_test(0 "lp_infeasible_1.mps" "unsat" "" "mps")

marabou_add_acasxu_test(0 "ACASXU_experimental_v2a_1_7.nnet" "3" sat)
marabou_add_acasxu_dnc_test(0 "ACASXU_experimental_v2a_1_9.nnet" "4" sat)
marabou_add_acasxu_test(0 "ACASXU_experimental_v2a_4_1.nnet" "4" unsat)

marabou_add_mnist_test(0 "mnist10x20.nnet" "image1_target1_epsilon0.005.txt" unsat)
marabou_add_mnist_test(0 "mnist2x256.nnet" "image3_target9_epsilon0.005.txt" unsat)

marabou_add_coav_test(0 "reluBenchmark0.067841053009s_UNSAT.nnet" unsat)
marabou_add_coav_test(0 "reluBenchmark0.0518190860748s_UNSAT.nnet" unsat)
marabou_add_coav_test(0 "reluBenchmark0.536728143692s_SAT.nnet" sat)
marabou_add_coav_test(0 "reluBenchmark0.899523973465s_SAT.nnet" sat)
