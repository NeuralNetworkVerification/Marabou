- Overview

This folder contains scripts to conduct small-scale scalability experiment.
It also contains the log files of the scalability experiment we reported in the paper.
The experiment was conducted on the Intel Cluster vLab to test the impact of the number
of allocated CPUs on the performance of solvers that support parallel execution.
We ran two solvers on the AcasXU benchmarks with increasing amount of cpu 
allowance  (2, 4, 8, 16, 32, 64). The two solvers are Marabou in the Divide-and-Conquer
mode and ReluVal. We used a wall-clock timeout of 2 hours across the board and allocated
80000 MB memory for each CPU.
In the following sections we describe the content of this folder, how to analyze existing
data, and how to conduct small-scale scalability experiment using the provided scripts.

- File organization

There are five folders in this directory.
  Marabou
    log files for Marabou
    The log files are organized in the following order:
     Marabou/{2, 4, 8, 16, 32, 64}_nodes/property{1, 2, 3, 4}/
    Since the full log data is more than 500 MB, we provide a lightweight snapshot of it.
    Concretely, we provide the log files only for checking property 3 with different
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

  sample_experiment
    This folder contains the networks, the property, and the solver scripts to be used in
    the sample experiment.

- Log files

We provide the standard output (.log files) and standard error (.err files) of each
solver run. We use run-lim to limit time and memory usage of a program, as well as 
benchmarking the runtime of the solver. Though both solver can themselves report runtime,
we used the runtime reported by run-lim for both solvers to ensure fair comparison.

We also provide the summary files that solvers generate.

For Marabou, we also provide the summary files (with suffix "-summary-{2, 4, 8...}.txt")
that the solver produces only when it solves a query. These files mainly contains SAT/UNSAT 
information.

- Scripts

We provide scripts that we use to analyze the log files and generate plots from them.

To get a summary of the performance of Marabou on all benchmarks, call
   - python3 scripts/parse_result.py Marabou/ [filename]

If a filename is provided as a second argument, the script will write the summary to
a csv file. Otherwise, it will print out the summary.

Similarly, call
   - python3 scripts/parse_result.py ReluVal/ 
to get the performance summary of ReluVal on all benchmarks.

You could also check the performance of a solver on a particular property with a particular
number of cpu allowance. For instance, to get the performance summary of Marabou on checking
property3 given 4 cpus, call

    - python3 scripts/parse_result.py Marabou/4_nodes/property3/

To generate the scalability plot similar to the one in the paper call
    - python3 scripts/compare_scalability.py


- A sample scalability experiment

Since it is unrealistic to reproduce the large-scale experiment that we conducted in a virtual
machine environment, we provide the scripts to run a small-scale experiment. Concretely, we ask
ReluVal and Marabou to check property 3 on only 2 networks, with an increasing amount of spawned
threads. The two networks are located in sample_experiment/networks/, the path to
the property file is sample_experiment/property3.txt. Due to the different name of the created log file,
we use a slightly different version of parse_result.py and compare_scalability.py, namely
parse_result_mini.py and compare_scalability_mini.py. You do not need to explicitly call them, as
the experiment can be run by simply calling
   - ./run_sample_experiments.sh

The experiment should take less than 5 minute to finish on machines with 4 cpus.
If the script runs properly, you should see the performance summary printed, and a plot comparing
the scalability of Marabou and ReluVal on these two small benchmark displayed.

Note that you can also use run_sample_experiment.sh to solve other benchmarks: you could add more networks
(.nnet and .pb both required) to "sample_experiment/networks/". You could also replace the content of
sample_experiment/property3.txt with other properties.

After doing this, re-run the run_sample_experiment.sh script and it will try to solve the new benchmarks
that you added.
