build_dir=${1:-build32}
if [ -d "$build_dir" ]; then rm -Rf $build_dir; fi
CXXFLAGS_BAK="$CXXFLAGS"
export CXXFLAGS="-m32 $CXXFLAGS"
echo $CXXFLAGS
mkdir ${build_dir} 
cd ${build_dir}
cmake ../.. -DFORCE_PYTHON_BUILD=1
cmake --build .
export CXXFLAGS="$CXXFLAGS_BAK"
