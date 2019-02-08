
# Create benchmark files
for j in 1 2 4 8
do
    python3 scripts/create_arguments_reluval.py  mini-example/networks/ mini-example/property4.txt $j mini-example/benchmark_sets/benchmark_set_prop4_"$j"_nodes_reluval
    python3 scripts/create_arguments_marabou.py mini-example/networks/ mini-example/property4.txt $j mini-example/benchmark_sets/benchmark_set_prop4_"$j"_nodes_marabou
done

echo "benchmarking Marabou..."
python3 scripts/run_with_runlim_marabou.py  mini-example/solvers/Marabou/run.sh mini-example/benchmark_sets/
echo "benchmarking Reluval..."
python3 scripts/run_with_runlim_reluval.py  mini-example/solvers/ReluVals/network_test mini-example/benchmark_sets/

python3 scripts/parse_result_mini.py mini-example/tmp/Marabou/
python3 scripts/parse_result_mini.py mini-example/tmp/ReluVal/
python3 scripts/compare_scalability_mini.py
