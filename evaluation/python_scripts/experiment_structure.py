"""
    experiment structure:

    resources   |   nnet                    - directory contains nnet files
                |   solvers                 - directory contains exec of solvers
                |   sbatch_files            - directory contains .sbatch files
                |   utils                   - directory contains other utils for later use
                |   tmp                     - directory contains temporary files
                |   result  | job_result    - directory contains result from slurm
                            | solver_result - directory contains result from solved
                            | comparisons   - directory contains comparisons results
                |   property_x.txt          - a file of the property to test
                |   experiment_details      - a file contains the experiment details
"""
import os

BASE_DIR = 'resources'
EXPERIMENT_DETAILS_FILENAME = 'experiment_details'
NNET_DIR = 'nnet'
SOLVERS_DIR = 'solvers'
SBATCH_FILES_DIR = 'sbatch_files'
UTILS_DIR = 'utils'
RESULTS_DIR = 'results'
SOLVER_RESULT_DIR = 'solver_result'
SLURM_RESULT_DIR = 'job_result'
COMPARISION_DIR = 'comparisons'
TMP_DIR = 'tmp'

# relative paths
EXPERIMENT_DETAILS_PATH = os.path.join(BASE_DIR, EXPERIMENT_DETAILS_FILENAME)
NNET_PATH = os.path.join(BASE_DIR, NNET_DIR)
SOLVERS_PATH = os.path.join(BASE_DIR, SOLVERS_DIR)
SBATCH_FILES_PATH = os.path.join(BASE_DIR, SBATCH_FILES_DIR)
UTILS_PATH = os.path.join(BASE_DIR, UTILS_DIR)
RESULT_PATH = os.path.join(BASE_DIR, RESULTS_DIR)
TMP_PATH = os.path.join(BASE_DIR, TMP_DIR)
SOLVER_RESULT_PATH = os.path.join(RESULT_PATH, SOLVER_RESULT_DIR)
SLURM_RESULT_PATH = os.path.join(RESULT_PATH, SLURM_RESULT_DIR)
COMPARISION_PATH = os.path.join(RESULT_PATH, COMPARISION_DIR)
