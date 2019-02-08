rm -rf sample_experiment/tmp/ sample_experiment/benchmark_sets/*

# Create benchmark files
for j in 2 4 8
do
    python3 scripts/create_arguments_reluval.py  sample_experiment/networks/ sample_experiment/property3.txt $j sample_experiment/benchmark_sets/benchmark_set_prop3_"$j"_nodes_reluval
    python3 scripts/create_arguments_marabou.py sample_experiment/networks/ sample_experiment/property3.txt $j sample_experiment/benchmark_sets/benchmark_set_prop3_"$j"_nodes_marabou
done

echo "benchmarking Marabou..."
python3 scripts/run_with_runlim_marabou.py  sample_experiment/solvers/Marabou/run.sh sample_experiment/benchmark_sets/
echo "benchmarking Reluval..."
python3 scripts/run_with_runlim_reluval.py  sample_experiment/solvers/ReluVals/network_test sample_experiment/benchmark_sets/

python3 scripts/parse_result_mini.py sample_experiment/tmp/Marabou/
python3 scripts/parse_result_mini.py sample_experiment/tmp/ReluVal/
python3 scripts/compare_scalability_mini.py
