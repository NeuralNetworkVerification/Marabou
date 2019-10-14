
 marabou_add_coav_test(1 "reluBenchmark1.30941200256s_UNSAT.nnet" UNSAT) # 6.8  sec
marabou_add_coav_test(1 "reluBenchmark0.94518494606s_SAT.nnet" SAT) # 7sec
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_5_1.nnet" "2" SAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_2_4.nnet" "2" SAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_1_8.nnet" "3" SAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_2_9.nnet" "3" UNSAT)
