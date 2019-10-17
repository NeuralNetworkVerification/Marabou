
marabou_add_coav_test(1 "reluBenchmark1.30941200256s_UNSAT.nnet" UNSAT) # 6.8  sec
marabou_add_coav_test(1 "reluBenchmark0.94518494606s_SAT.nnet" SAT) # 7sec
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_5_1.nnet" "2" SAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_2_4.nnet" "2" SAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_1_8.nnet" "3" SAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_2_9.nnet" "3" UNSAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_1_6.nnet" "3" UNSAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_3_6.nnet" "4" UNSAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_3_7.nnet" "4" UNSAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_1_7.nnet" "4" SAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_1_8.nnet" "4" SAT)
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_1_9.nnet" "4" SAT)

marabou_add_acasxu_dnc_test(1 "ACASXU_experimental_v2a_3_5.nnet" "2" SAT)
marabou_add_acasxu_dnc_test(1 "ACASXU_experimental_v2a_5_7.nnet" "3" UNSAT)
marabou_add_acasxu_dnc_test(1 "ACASXU_experimental_v2a_3_9.nnet" "3" UNSAT)
marabou_add_acasxu_dnc_test(1 "ACASXU_experimental_v2a_4_7.nnet" "4" UNSAT)
