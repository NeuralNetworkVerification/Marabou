Dear artifact evaluation committee,

This readme file is organized as follows:

- In part (i), we explain in detail the different files and
directories of the "CnnAbs_Artifact" directory in which our artifact is
stored.

- In part (ii), we explain how to replay the experiments reported in
our paper. Specifically, we explain how to configure and compile the
Marabou framework, and then how to run the different experiments.

We note that we intend to make the whole artifact, including the
complete source code & result summary, publicly available
online permanently, together with the final version of this paper.


###############################
  _____           _      _____ 
 |  __ \         | |    |_   _|
 | |__) |_ _ _ __| |_     | |  
 |  ___/ _` | '__| __|    | |  
 | |  | (_| | |  | |_    _| |_ 
 |_|   \__,_|_|   \__|  |_____|
###############################

###################################################
Different files and directories in CNN-Abs_Artifact
###################################################

Below is a summary of the contents of the CNN-Abs zipped directory. To make sense of some of the decisions made, one should know that two featues were impacted by the artifact's requirments:
1) We used the MNIST dataset, and the artifact contains an offline version of it as a Numpy array to accomodate the offline requirement.
2) We used the Gurobi LP solver, but the artifact can not use it since it requires a license. Therefore we added pre-calculated propagated bounds to be used in experiments reruns.
Now we continue to artifact's contents.

In the top of tree:


(-) Readme.txt:

This file.


(-) License.txt

A license for the artifact.


(-) CnnAbs_Artifact Directory


(-----) installations

A directory with 48 whl-formatted files, which will later be
installed (by the "make_installations.sh" script), in order for the
experiments python scripts to run with adequate modules imported.


(-----) make_installations.sh

A script that is called once, before compiling & running Marabou, and
installs the python packages (saved as whl files) in the
"installations" directory. The script also activates additional
scripts in the Marabou/tools sub-directory, for updating the relevant
versions of "boost" and "pybind" in order for Marabou's compilation
to succeed.


(-----) compile_marabou.sh

A script for creating the Marabou/build sub-directory, then building
Marabou and compiling it.


(-----) paper_results

This directory includes files for the submitted paper, with emphasis
on the experiments presented in the "Evaluations" section of the
paper:

  (*) CnnAbs.pdf - The submitted paper.
						
  (*) ResultsRaw.zip - Experiments output that can be unziped and rerun to produce results using Exepriments_OnlyGraphs.py

  (*) Results - Figures used in the paper which can be compared to reproduced results.

(-----) tool

A directory containing the CNN-Abs tool core files, including:

(---------) Marabou - The Marabou framework we used as the backend verifier. 
It is extensive, and we do not cover its features. The reader is referred to the original Marabou paper:

  https://link.springer.com/chapter/10.1007/978-3-030-25540-4_26

You can download the Marabou framework from the main Marabou repository - which is an open-source, free tool, available on GitHub:

  https://github.com/NeuralNetworkVerification/Marabou

  A notable file that we added to in Marabou, as part of our bound propagation technique is Marabou/src/nlr/LPFormulator.cpp, in which added our technique to the LPFormulator::addMaxLayerToLpRelaxation() function.

(---------) CnnAbsFiles - Containing the CNN-Abs abstraction-refinement engine files.


(-------------) CnnAbs.py - The module implementing the CNN-Abs tool. Contains the CnnAbs class facilitating the solving process, the ModelUtils class implementing Tensorflow interface, the QueryUtils class implementing Marabou interface, and a few more classes. CnnAbs is the only one meant to be used by the end-user, and it contains the solveAdversarial and solve function meant for end-user use. Additionaly, it defines the used dataset, for which we used MNIST - but others can be easily incoorporated.


(-------------) CnnAbsTB.py - A wrapper of CnnAbs.py allowing for easy adversarial robustness property. An intuitive interface that will be explained in the following parts allows the user to define a data sample to pertube around, a pertubation distance, timeout, and abstraction policy to use (out of these defined in the paper). As said earlier, the data samples are from MNIST.


(-------------) Experiment_CnnAbsVsVanilla.py - Run the CNN-Abs vs Vanilla experiment from the paper, and creates appropraite graphs named according to their figure number in submitted paper version.


(-------------) Experiment_ComparePolicies.py - Run the policy comparison experiment from the paper, and creates appropraite graphs named according to their figure number in submitted paper version.


(-------------) Experiment_OnlyGraphs.py - Using the raw results we used in the paper (same results, not a rerun), that are added as a zip file that should be extracted by the user, one can reproduce precisly our graphs from the paper.


(-------------) mnist - directory containing the MNIST dataset. Added to allow offline use.


(-------------) evaluation - directory containing scripts and data required for evaluation.


(-----------------) bounds - directory containing propegated bounds for networks A,B,C and distances 0.01,0.02,0.03 used in the papers/ Required since no Gurobi licence is available for the artifact evaluation. Bounds are saved as JSON dictionaries.


(-----------------) boundsDumpGurobi.py - Scripts calculating bounds propegation for the LP relaxation example shown in Fig5. Can not be run in the artifact since Gurobi can not run without license.


(-----------------) launcher.py - Script creating batches of queries used for different experiments. In the artifact it produces a JSON file containing command line python3 commands for running seperate queries in CnnAbsTB.py, but in our original experiments we used it for cluster Slurm runs (not required to be directly used by user).

(-----------------) resultParser.py - Script parsing results logged by batch runs of launch.py, which are run by the Marabou/maraboupy file executeFromJson.py (not required to be directly used by user).


(-----------------) CreateGraph.py - Script taking the results of resultsParser.py and gathers them to the paper results and plots (not required to be directly used by user).


(-----------------) downloadMnist.py - Script downloading mnist, for this offline use (not required to be used by user)


(-----------------) dumpAllBounds.py - Script creating propagated bounds' JSON files, for this no-Gurobi use (not required to be used by user).


(-----------------) launchDumpAllBounds.py - Script creating propagated bounds' JSON files, for this no-Gurobi use (not required to be used by user).


(-----------------) TrainEvaluatedNetworks.py - Script creating the networks used in the paper (not required to be used by user).


(-----------------) networkA.h5 , networkB.h5 , networkC.h5 - CNNs used for evaluation.



#####################################
  _____           _      _____ _____ 
 |  __ \         | |    |_   _|_   _|
 | |__) |_ _ _ __| |_     | |   | |  
 |  ___/ _` | '__| __|    | |   | |  
 | |  | (_| | |  | |_    _| |_ _| |_ 
 |_|   \__,_|_|   \__|  |_____|_____|
#####################################
##########################################
Using the tool and running the experiments
##########################################

In this part we will explain the steps required to run the
experiments described in our paper:

    1. Installing the necessary packages and compiling Marabou 
    2. Running the experiments

We note that for ease of explanations both <> and "" parenthesis were
added in the below description of files.

####################################################
Installation Required Packages and Compiling Marabou
####################################################

Before running the scripts - please update the permissions for all
files by running this command:

(1) Stand inside CnnAbs_Artifact
(2) Run: chmod 777 -R *

    This is required in order to run all executable bash & python files.


(3) Run : ./make_installations.sh
    Installs the relevant python modules (with pip3) in the "installations" directory, in order for the experiment scripts to work properly.

(4) Run : ./compile_marabou.sh
							
Creates a "build" directory and compiles the Marabou framework with the updated settings. We note that the compilation process itself may take time, due to running 60 tests for different aspects of the code.

After both scripts completed their execution, add directories
tool and tool/Marabou to PYTHONPATH by running these commands:

(5) export PYTHONPATH="$PYTHONPATH:$(realpath ./tool)"
(6) export PYTHONPATH="$PYTHONPATH:$(realpath ./tool/Marabou)"
(7) export PYTHONPATH="$PYTHONPATH:$(realpath ./tool/Marabou/maraboupy)"
(8) export PYTHONPATH="$PYTHONPATH:$(realpath ./tool/CnnAbsFiles)"
(8) export PYTHONPATH="$PYTHONPATH:$(realpath ./tool/CnnAbsFiles/evaluation)"

#######################
Running the Experiments
#######################


After compiling Marabou (stage 1) and choosing our configuration 
(stage 2) we are now ready to run the actual experiments.

(*) Move to the CnnAbs_Artifact/tool/CnnAbsFiles directory to run the experiments.
						
All scripts use a Python3 Argparse interface, which is intuitive and uses '--' flags to pass command line arguments. Whenever a question rise, the user can use --help and recieve a documention of the available flags.

(-) Using the CnnAbs module:

    CnnAbs.py

One can use the solve() and solveAdversarial() functions to solve a query. Before hand it will have to instantiate a CnnAbs object as described in the code. Since using these functions requires an interface through Marabou, we provide the CnnAbsTB.py wrapper that allows easy use of the solveAdversarial function. These functions could be used to provide a wide use of our technique, but since we focus on adversarial robustness we provide a wrapper implementing all our desired functionality.

(-) Run basic adversarial robustness query:

    CnnAbsTB.py

python3 CnnAbsTB.py --net <relative/path/to/net.h5> --sample <sampleIndex> --prop_distance <pertubationDistance> --policy <abstractionPolicy> --gtimeout <globalTimeout> --propagate_from_file

net : Network used. In our case, we can use the ones in evaluation directory.
sample : Chosen sample. We use 0 to 99, since these have propagated bounds pre-calculated in Gurobi.
prop_distance : Distance to pertube around the sample, in the adversarial robustness query. Default is 0.03.
policy : Abstraction policy to use. Can be : Centered, AllSamplesRank, SingleClassRank, MajorityClassVote, SampleRank, Random, Vanilla.
gtimeout : timeout for the entire verification query. The default is 3600 seconds, but can be reduced.
propagate_from_file : flag that should be added to read from Gurobi pre-calculated bounds. Please use this in every time.

One can see a SAT example by running
python3 CnnAbsTB.py --net evaluation/networkA.h5 --sample 8 --prop_distance 0.03 --policy SingleClassRank --propagate_from_file

One can see UNSAT examples by running
python3 CnnAbsTB.py --net evaluation/networkA.h5 --sample 0 --prop_distance 0.03 --policy SingleClassRank --propagate_from_file
python3 CnnAbsTB.py --net evaluation/networkA.h5 --sample 13 --prop_distance 0.03 --policy SingleClassRank --propagate_from_file

The scripts runs and prints its run result in the end, print a location of CEX numpy array if relevant, and a location of log directory. In the log directory the are 'Result.json' files that log the run. The important keys in that log are 'totalRuntime' logging the solving runtime, 'Result' logging the result (GTIMEOUT, SAT, UNSAT), and more.


(-) Run policy comparison experiment from paper:

python3 Experiment_ComparePolicies --instances <numberOfInstances> --gtimeout <TimeoutInSeconds>

This will print results of this specific experiment (Fig6) in CnnAbsFiles/Results. The number of instances will dictate the number of input samples run on each policy in the experiment. We used 100 in out experiment, but it will be realistic to use small numbers (1-10). The default timeout we used was an hour (3600 seconds), but we recomend a smaller number (30 seconds) for a reasonable ending time.

(-) Run CNN-Abs / Vanilla comparison experiment from paper:

python3 Experiment_CnnAbsVsVanilla --instances <numberOfInstances> --gtimeout <TimeoutInSeconds>

This will print results of this specific experiment (Fig 7-9, 13-15) in CnnAbsFiles/Results. The number of instances will dictate the number of input samples run vanilla / policy in the experiment. We used 100 in out experiment, but it will be realistic to use small numbers (1-5). The default timeout we used was an hour (3600 seconds), but we recomend a smaller number (30 seconds) for a reasonable ending time. Remember that the experiment runs Vanilla / policy comparison for each network / distance combination mentioned in the paper, total of nine combinations.

(-) Create graph for zipped logged actual raw results from the paper.

python3 Experiment_OnlyGraphs.py --logs <path/to/unzipped/logs>

In paper_results/ResultsRaw.zip we have the raw results we used in the paper. One can unzip them, and give as the argument to this scripts, to produce all the results in CnnAbsFiles/Results.






Thank you for your time!

Matan Ostrovsky
Clark Barrett
Guy Katz
