#!/usr/bin/env bash
# Copyright 2021 Xilinx, Inc.
# Copyright 2022 Advanced Micro Devices, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


usage() {
cat << EOF
usage: ./xmodel_benchmark.sh

xmodel_benchmark.sh is a benchmarking script for the xmodel worker which just
runs a single xmodel, without pre- or post-processing. It's a useful way to
sweep different parameters that are available in the xmodel.cpp test.

To use:

1. Add to the Xmodel worker:
    #include <mutex>
    #include <thread>
2. Add "std::mutex mutex_;" as a class member to the XModel class
3. Replace the execute_async() call in the xmodel.cpp worker with:
    {
      std::lock_guard<std::mutex> guard(this->mutex_);
      std::this_thread::sleep_for(DURATION);
    }
4. Add the define to CMakeLists.txt for the xmodel.cpp worker:
  target_compile_definitions(workerXmodel PRIVATE DURATION=${DURATION})

To modify the reference implementation for the xmodel test,

1. Add to xmodel.hpp:
    #include <mutex>
2. Add mutexes as a class member to the Inference class
    std::vector<std::unique_ptr<std::mutex>> mutexes_;
3. When creating the runners, also make mutexes (# runners = # mutexes)
    mutexes_.push_back(make_unique<std::mutex>());
4. In run_thread(), get a reference to the appropriate runner
    auto& mutex = mutexes_[ridx];
5. Replace the execute_async() and wait() call to the runnner with:
    {
      std::lock_guard<std::mutex> guard(*mutex);
      std::this_thread::sleep_for(DURATION);
    }
Add the define to CMakeLists.txt for the xmodel.cpp test:
  target_compile_definitions(xmodel PRIVATE DURATION=${DURATION})

After making the above changes, configure the parameters to sweep in this script
and run. To run the reference implementation, add the --reference flag to the
program invocation.

Note: not all parameter configurations may be valid. Check in the xmodel.cpp
test case to see what is permissible.
EOF
}

# number of times to repeat the test for averaging
repeat=3
# these arrays should be of equal length. For duration[0], images[0] are used
durations=("1ms" "100us")
images=(100000 500000)

# syntax is runners:threads. The threads are divided among the runners
parameters=(
  "1:1"
  "1:3"
  "3:3"
  "3:9"
)

cwd=$(pwd)
cd $AMDINFER_ROOT


for index in ${!durations[*]}; do
  duration=${durations[$index]}
  image=${images[$index]}

  ./amdinfer build --release -DDURATION=$duration &> /dev/null
  if [[ $? != 0 ]]; then
    echo "Skipping build with a $duration sleep time. Build failed."
    continue
  fi

  for parameter in ${parameters[@]}; do
    parameter_array=(${parameter//:/ })
    runners=${parameter_array[0]}
    threads=${parameter_array[1]}

    sum=0
    success_count=0
    for (( i = 0; i < $repeat; i++ )) ; do
      retval=$(./build/Release/tests/cpp/native/xmodel -r $runners -t $threads -i $image 2> /dev/null | grep '^Average queries per second')
      if [[ -n $retval ]]; then
        qps=$(echo $retval | awk -F ' ' '{print $5}')
        success_count=$((success_count + 1))
        sum=$(echo - | awk "{print $sum + $qps}")
      fi
    done
    if [[ $success_count > 0 ]]; then
      avg_qps=$(echo - | awk "{print $sum / $success_count}")
      time=$(echo - | awk "{print $image / $avg_qps}")
      echo "delay: $duration, runners: $runners, threads: $threads, time: $time s, qps: $avg_qps (avg of $success_count run[s])"
    else
      echo "delay: $duration, runners: $runners, threads: $threads, time: Failed, qps: Failed"
    fi
  done
done

cd $cwd
