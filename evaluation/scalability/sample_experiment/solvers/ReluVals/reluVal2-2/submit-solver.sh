#!/bin/bash

default_qos="normal"
runlim_binary="/export/stanford/barrettlab/local/bin/runlim"
runlim_options=""
color_blue="\e[94m"
color_green="\e[92m"
color_red="\e[91m"
color_default="\e[39m"

qos_normal_tlimit=3600
qos_max2_tlimit=7200
qos_max12_tlimit=43200

re_numeric='^[0-9]+$'

die ()
{
  echo "  *** error: $*" 1>&2
  exit 1
}

msg ()
{
  echo -e "  ${color_red}** $*${color_default}"
}

info ()
{
  echo -e "  ${color_green}$1${color_default}"
}

while [ $# -gt 0 ]
do
  case $1 in
    -s)
      shift
      scrambler="$1"
      ;;
    -t)
      shift
      traceexec="$1"
      ;;
    -c)
      shift
      copy_dir="$1"
      ;;
    *)
      [[ "$solver" != "" ]] && die "solver already set to '$solver'"
      solver="$1"
      ;;
  esac
  shift
done

[ -z "$solver" ] && die "no solver binary specified"
[ ! -e "$solver" ] && die "solver '$solver' does not exist"
[ -d "$solver" ] && die "solver '$solver' is a directory"
[ ! -x "$solver" ] && die "solver '$solver' is not executable"

solver=$(readlink -f "$solver")
solver_name=$(basename "$solver")

solver_path="solver/"
if [ -n "$copy_dir" ]; then
  solver_path=$(realpath --relative-to="$copy_dir" "$solver")
  [[ "$solver_path" == ..* ]] && die "'$solver' is not in a subdirectory of '$copy_dir'"
  solver_path="solver/$(dirname "$solver_path")"
fi

info "using solver '$solver'"

if [ -n "$scrambler" ]; then
  scrambler=$(readlink -f "$scrambler")
  scrambler_seed=$RANDOM
  info "using scrambler '$scrambler' with seed '$scrambler_seed'"
fi

[ -n "$traceexec" ] && info "using trace executor '$traceexec'"
[ -n "$traceexec" -a -z "$scrambler" ] && \
  die "trace executor needs scrambler"

[ -n "$copy_dir" -a ! -e "$copy_dir" ] && \
  die "copy directory '$copy_dir' does not exist"


# find user defined benchmark sets
sets=$(find $benchmark_sets_user $benchmark_sets_system \
         -maxdepth 1 -type f -name 'benchmark_set_*' | sort)

[[ -z "$sets" ]] && \
  die "no benchmark sets found in " \
      "'$benchmark_sets_system' and '$benchmark_sets_user'" \
      "found in current directory"


# print available benchmark sets
declare -A benchmark_list
declare -A benchmark_list_rev
declare -A benchmark_files
benchmark_indices_all=""
cnt=1
echo -e "available benchmark sets:"
for f in $sets; do
  n=$(basename $f)
  s=${n#benchmark_set_}
  benchmark_list[$cnt]=$s
  benchmark_list_rev[${s,,}]=$cnt
  benchmark_files[$cnt]=$f
  num_benchmarks=$(wc -l $f | awk '{print $1}')
  benchmark_indices_all="$benchmark_indices_all $cnt"

  d=$(dirname "$f")
  if [[ "$d" == "$benchmark_sets_system" ]]; then
    printf "  %5s $s ($num_benchmarks) [system]\n" "[$cnt]"
  else
    printf "  ${color_blue}%5s $s ($num_benchmarks) [user]${color_default}\n" \
           "[$cnt]"
  fi
  ((cnt+=1))
done

# select benchmark set
benchmark_indices=""
while true;
do
  read -e -p "select benchmark set (default: ${benchmark_list[1]}): " s

  if [[ -z "$s" ]]; then
    idx=1
    break
  # '*' select all available benchmark sets
  elif [[ "$s" == "*" ]]; then
    benchmark_indices="$benchmark_indices_all"
    break
  else
    # allow range syntax for selecting multiple benchmark sets
    if [[ "$s" = *".."* ]]; then
      s="`eval echo $s`"
    fi
    input_ok=true
    for bset in $s; do
      if [[ "$bset" =~ $re_numeric ]]; then
        if (( bset >= 1 && bset < cnt)); then
          idx=$bset
          benchmark_indices="$benchmark_indices $idx"
          continue
        fi
      else
        s="${bset,,}"
        if [[ -n "${benchmark_list_rev[$s]}" ]]; then
          idx=${benchmark_list_rev[$s]}
          benchmark_indices="$benchmark_indices $idx"
          continue
        fi
      fi
      input_ok=false
    done
    if [ "$input_ok" = true ]; then
      break
    fi
  fi
  msg "invalid benchmark set specified"
done
benchmark_sets=""
num_benchmark_sets=0
info "using benchmark set(s):"
for idx in $benchmark_indices; do
  info "  ${benchmark_files[$idx]}"
  benchmark_sets="$benchmark_sets ${benchmark_files[$idx]}"
  (( num_benchmark_sets++ ))
done

# set working directory
while true
do
  read -e -p "choose working directory: " working_dir
  if [[ "$working_dir" == "" ]]; then
    msg "no working directory specified"
    continue
  fi
  if [[ ! -d $working_dir ]]; then
    break
  fi
  msg "directory '$working_dir' already exists"
done
info "using directory '${working_dir}'"


# configure solver
read -e -p "choose '$solver_name' options: " user_options

if [ -n "$user_options" ]; then
  solver_options="$user_options"
fi
[ -n "$solver_options" ] && info "using options '$solver_options'"


# number of CPUs depends on selected
num_virtual_cores=$(nproc)
num_physical_cores=$((num_virtual_cores / 2))

max_num_cpus=$num_virtual_cores
#if [[ "$qos" == "normal" ]]; then
#  default_num_cpus=2
#else
#  default_num_cpus=1
#fi

# choose time limit
while true
do
  read -p "choose time limit (default: $default_time_limit): " time_limit

  if [[ "$time_limit" == "" ]]; then
    time_limit=$default_time_limit
    break
  fi
  if [[ ! $time_limit =~ $re_numeric ]]; then
    msg "time limit '$time_limit' is not a number"
    continue
  else
    break
  fi
done


# choose space limit
while true
do
  read -p "choose space limit (default: 4000 per CPU): " space_limit

  if [[ "$space_limit" == "" ]]; then
    space_limit=$default_space_limit
    break
  fi
  if [[ ! $space_limit =~ $re_numeric ]]; then
    msg "space limit '$space_limit' is not a number"
    continue
  else
    break
  fi
done

# choose num cpus
while true
do
  read -p "choose num cpu: [max 112]" num_cpus

  if [[ "$num_cpus" == "" ]]; then
    num_cpus=$default_num_cpus
    break
  fi
  if [[ ! $num_cpus =~ $re_numeric ]]; then
    msg "space limit '$space_limit' is not a number"
    continue
  else
    break
  fi
done

# set runlim limits if specified
if [[ $time_limit != 0 ]]; then
  runlim_options="--time-limit=$time_limit"
fi
if [[ $space_limit != 0 ]]; then
  runlim_options="$runlim_options --space-limit=$space_limit"
fi
[[ -n "$runlim_options" ]] && info "using runlim options $runlim_options"


# create working directory
mkdir -p $working_dir
working_dir="$(realpath $working_dir)"

## create options file
echo "time limit:     $time_limit" > $working_dir/options
echo "space limit:    $space_limit" >> $working_dir/options
echo "num_cpus:       $num_cpus" >> $working_dir/options
echo "solver:         $solver_name" >> $working_dir/options
echo "solver options: $solver_options" >> $working_dir/options
echo "runlim options: $runlim_options" >> $working_dir/options
echo "qos:            $qos" >> $working_dir/options
if [ -n "$scrambler" ]; then
  echo "scrambler:      $scrambler" >> $working_dir/options
  echo "scrambler seed: $scrambler_seed" >> $working_dir/options
fi
if [ -n "$traceexec" ]; then
  echo "traceexec:      $traceexec" >> $working_dir/options
fi

## setup binaries/scripts

solver_dir="$working_dir/solver"
working_solver_path="$working_dir/$solver_path"

mkdir $solver_dir

# copy solver binary
[ -z "$copy_dir" ] && cp "$solver" "$solver_dir"

# copy contents of directory
[ -n "$copy_dir" ] && cp -a "$copy_dir/." "$solver_dir"

# copy scrambler
[ -n "$scrambler" ] && cp "$scrambler" "$working_solver_path"

# create solver wrapper for trace executor
if [ -n "$traceexec" ]; then
  cp "$traceexec" "$working_solver_path/"
  echo "#!/bin/sh" > "$working_solver_path/solver_wrapper.sh"
  echo "./$solver_name $solver_options \$1" >> "$working_solver_path/solver_wrapper.sh"
  chmod +x "$working_solver_path/solver_wrapper.sh"

  solver_name="$(basename "$traceexec")"
  solver_options="./solver_wrapper.sh"
fi

solver_name="./$solver_name"
[ -n "$scrambler" ] && scrambler_name="$working_dir/${solver_path}/$(basename "$scrambler")"

# create array job for each benchmark set
for benchmark_set in $benchmark_sets; do
  set_name=$(basename $benchmark_set)
  set_name="${set_name#benchmark_set_}"
  if [ $num_benchmark_sets -eq 1 ]; then
    working_dir_set="$working_dir"
  else
    working_dir_set="$working_dir/$set_name"
    mkdir -p $working_dir_set
  fi

  # save options, benchmarks file, solver
  cp $benchmark_set $working_dir_set/benchmarks

  # number of benchmark files = number of jobs in the array job
  ntasks=$(wc -l $benchmark_set | cut -d ' ' -f 1)

  # get most common prefix of all benchmark files
  prefix=$(
python - <<EOF
import os
prefix = os.path.commonprefix(open('$benchmark_set').readlines())
path = os.path.dirname(prefix)
if len(path) > 0:
    print('{}/'.format(path))
EOF
  )

  # create sbatch script
  cat > $working_dir_set/script.sh << EOF
#!/bin/sh
#PBS -J 1-$ntasks
#PBS -l walltime=00:00:$time_limit
#PBS -lselect=1:ncpus=$num_cpus

prefix="$prefix"
runlim_binary="$runlim_binary"
runlim_options="$runlim_options"
solver="$solver_name"
solver_options="$solver_options"
benchmarks="$working_dir_set/benchmarks"

# pick benchmark file from list of benchmarks
benchmark="\`sed \${PBS_ARRAY_INDEX}'q;d' \$benchmarks\`"
out="\$(echo \${benchmark#\$prefix} | sed -e 's,/,-,g')"
# set stdout log file
log="$working_dir_set/\${out}.log"
# set stderr log file
err="$working_dir_set/\${out}.err"

(
echo "c host: \$(hostname)"
echo "c start: \$(date)"
echo "c benchmark: \$benchmark"
echo "c solver: \$solver"
echo "c solver options: \$solver_options"

# copy benchmark from NFS to /tmp
cd "$working_dir/$solver_path" && \$runlim_binary \$runlim_options "\$solver" \$benchmark "\$solver_options"


echo "c done"
) > "\$log" 2> "\$err"
EOF

  # create sub shell, change working directory and execute script
  (cd $working_dir_set && exec qsub ./script.sh)
done
