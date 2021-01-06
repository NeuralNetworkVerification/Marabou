module load gurobi/9.0.0
cd gkatz_space/$1
mkdir -p build
cd build
echo "$PWD"
cmake ..
#cmake .. -DBUILD_PYTHON=ON -DENABLE_GUROBI=ON
make

