// Copyright 2021 Xilinx, Inc.
// Copyright 2022 Advanced Micro Devices, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file
 * @brief Benchmark the xmodel worker
 */

#include "xmodel.hpp"

#include <cstdlib>              // for exit, getenv
#include <cxxopts/cxxopts.hpp>  // for value, OptionAdder, Options, OptionEx...

int main(int argc, char* argv[]) {
  const auto* aks_model_root = std::getenv("AKS_XMODEL_ROOT");
  if (aks_model_root == nullptr) {
    std::cerr << "AKS_XMODEL_ROOT not set in the environment\n";
    return 1;
  }
  auto xmodel = std::string{aks_model_root} +
                "/artifacts/u200_u250/resnet_v1_50_tf/"
                "resnet_v1_50_tf.xmodel";
  const auto default_images = 100;  // arbitrary

  int images = default_images;
  int threads = 4;
  int runners = 4;
  std::string path =
    std::string(std::getenv("AMDINFER_ROOT")) + "/tests/assets";
  bool run_ref = false;

  try {
    cxxopts::Options options("test_xmodel", "Test/benchmark the Xmodel worker");
    options.add_options()("m,model", "Path to xmodel to run",
                          cxxopts::value<std::string>(xmodel))(
      "i,images", "Number of images to send to the server",
      cxxopts::value<int>(images))(
      "t,threads", "Number of threads to use to enqueue/deque images",
      cxxopts::value<int>(threads))(
      "r,runners", "Number of runners (i.e. workers) to use in the server",
      cxxopts::value<int>(runners))(
      "p,path", "Path to directory containing at least one image to send",
      cxxopts::value<std::string>(path))(
      "reference",
      "Run the reference benchmark instead (defaults to using the AMD "
      "Inference Server)",
      cxxopts::value<bool>(run_ref))("help", "Print help");

    auto result = options.parse(argc, argv);

    if (result.count("help") != 0U) {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }
  } catch (const cxxopts::OptionException& e) {
    std::cout << "Error parsing options: " << e.what() << std::endl;
    return 1;
  }

  if (runners < 1 || threads < 1) {
    std::cerr << "There must be at least one runner and thread" << std::endl;
    return 1;
  }

  amdinfer::Server server;
  server.startHttp(kDefaultHttpPort);

  try {
    if (run_ref) {
      runReference(xmodel, images, threads, runners);
    } else {
      run(xmodel, images, threads, runners);
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
