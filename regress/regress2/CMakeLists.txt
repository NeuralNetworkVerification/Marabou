
marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_3_7.nnet" "4" unsat) 
marabou_add_coav_test(2 "reluBenchmark2.66962385178s_UNSAT.nnet" unsat) #25 sec
marabou_add_acasxu_dnc_test(1 "ACASXU_experimental_v2a_3_9.nnet" "3" unsat) #

 marabou_add_coav_test(2 "reluBenchmark2.64361000061s_SAT.nnet" sat) # 25sec
# marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_5_1.nnet" "2" sat) # 480sec
# marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_2_4.nnet" "2" sat) # 300sec
# marabou_add_acasxu_test(1 "ACASXU_experimental_v2a_3_7.nnet" "4" unsat) # 150sec
# marabou_add_acasxu_dnc_test(1 "ACASXU_experimental_v2a_3_5.nnet" "2" sat) # timeout
# marabou_add_acasxu_dnc_test(1 "ACASXU_experimental_v2a_3_9.nnet" "3" unsat) # 200sec
