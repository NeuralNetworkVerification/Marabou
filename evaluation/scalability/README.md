- Overview

This folder contains the log files conducted on the Intel Cluster vLab to test the 
scalability of two neural network verifiers that supports parallel execution. 
We run the two solvers on the AcasXU benchmarks with increasing amount of cpu 
allowance  (2, 4, 8, 16, 32, 64). The two solvers are Marabou in the Divide-and-Conquer
mode and ReluVal. We use a wall-clock timeout of 2 hours across the board and allocate
80000 MB memory for each CPU.
In the following sections we introduce in detail what the folder contains.
We also provide a mini-example where you could run scalability test on a small set of
benchmarks with 1, 2, 4, 8 cpus, extract the results, and obtain the plot of the same
fashion as the one presented in the paper.

- File organization

There are five folders in this directory.
  Marabou
    log files for Marabou
    The log files are organized in the following order:
     Marabou/{2, 4, 8, 16, 32, 64}_nodes/property{1, 2, 3, 4}/
    Since the full log files is more than 500 MB, we provide only a lightweight snapshot
    of it. Concretely, we provide the log files only for checking property 3 with different
    cpu nodes.
  
  ReluVal
    log files for ReluVal
    The log files are organized in the same way as in the Marabou folder. And again only
    the lightweight version is provided.

  scripts
    python scripts that parse the results to create csv summary files, and generate
    plots comparing the scalability of the two solvers. Instructions about how to use 
    the scripts are provided in the Scripts section below.

  csv
    This folder contains the full performance summary of Marabou and ReluVal on all benchmarks
    we run. These two csvs are generated using "scripts/parse_result.py"

  ReluVals
    This folder contains the different compilation versions of ReluVal that we ran, as the largest
    number of threads (MAX_THREAD) to spawn is a compilation parameter for ReluVal. Each executable,
    network_test{i}, is compiled with MAX_THREAD set to i.

- Log files

We provide the standard output (.log files) and standard error (.err files) of each
solver run. We use run-lim to limit time and memory usage of a program, as well as 
benchmarking the runtime of the solver. Though both solver can themselves report runtime,
we used the runtime reported by run-lim for both solvers to ensure fair comparison.

We also provide the summary files that solvers generate.

For Marabou, we provide the summary files (with suffix "-summary-{2, 4, 8...}.txt") that
the solver produces only when it solves a query. These files mainly contains SAT/UNSAT 
information.

For ReluVal, we provide the summary file that it generates when tasked with a query.
The summary file also contains SAT/UNSAT information in addition to the standard output
of the solver. Unlike Marabou, the presence of a summary file does not indicate that
ReluVal has solved the query.


- Scripts

We provide scripts that we use to analyze the log files and generate plots from them.

To get a summary of the performance of Marabou on all benchmarks, call
   - python3 scripts/parse_result.py Marabou/ [filename]

If a filename is provided as an argument, the script will write the summary to a csv file.
Otherwise, it will print out the summary.

Similarly, call
   - python3 scripts/parse_result.py ReluVal/ 
to get the performance summary of ReluVal on all benchmarks.

You could also check the performance of a solver on a particular property with a particular
number of cpu allowance. For instance, to get the performance summary of Marabou on checking
property3 given 4 cpus, call

    - python3 scripts/parse_result.py Marabou/4_nodes/property3/

To generate the scalability plot similar to the one in the paper call
    - python3 scripts/compare_scalability.py


- A mini-example of scalability test, and how to generalize the experiment

Since it is unrealistic to reproduce the large-scale experiment that we conducted in a virtual
machine environment, we only provide a small snapshot of our experiment.
Concretely, we ask ReluVal and Marabou to check property 4 on 2 networks, with an increasing
amount of spawned threads. The two networks are located in mini-example/networks/, the path to
the property file is mini-example/property4.txt. Due to the different name of the created log file,
we use a slightly different version of parse_result.py and compare_scalability.py, namely
parse_result_mini.py and compare_scalability_mini.py. You do not need to explicitly call them, as
the experiment can be run by simply calling
   - ./run_mini_example.sh

The experiment should take less than 5 minute to finish on machines with 4 cpus.
If the script runs properly, you should see the performance summary printed, and a plot comparing
the scalability of Marabou and ReluVal on these two small benchmark displayed.

Note that you could use run_mini_example.sh to solve other benchmarks.
You could add more networks (.nnet and .pb both required) to the "mini-example/networks/"
folder. You could also replace the content of mini-example/property4.txt with other properties. 

After doing this, re-run the run_mini_example.sh script and it will try to solve the new benchmarks
that you added.
