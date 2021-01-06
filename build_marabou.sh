export GUROBI_HOME="/cs/labs/guykatz/matanos/gurobi911"
export PATH="${PATH}:${GUROBI_HOME}/bin"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib"
export GRB_LICENSE_FILE="/cs/share/etc/license/gurobi/gurobi.lic"
mkdir -p build
cd build
echo "$PWD"
#cmake ..
cmake --build . -j 8 -DBUILD_PYTHON=ON -DENABLE_GUROBI=ON
#cmake .. -DBUILD_PYTHON=ON -DENABLE_GUROBI=ON
make

