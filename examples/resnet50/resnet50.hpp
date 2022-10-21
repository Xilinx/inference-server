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

#ifndef GUARD_EXAMPLES_RESNET50_RESNET50
#define GUARD_EXAMPLES_RESNET50_RESNET50

#include <cxxopts/cxxopts.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Args {
  fs::path path_to_model;
  fs::path path_to_image;
  fs::path path_to_labels;
  uint16_t http_port = 8998;
  uint16_t grpc_port = 50'051;
  int top = 5;
  int input_size = 224;
  int output_classes = 1000;
  std::string input_node;
  std::string output_node;
};

Args parseArgs(int argc, char** argv) {
  Args args;

  try {
    cxxopts::Options options("resnet50", "Run the resnet50 example");
    // clang-format off
    options.add_options()
    ("model", "Path to the resnet50 model on the server",
      cxxopts::value(args.path_to_model))
    ("image", "Path to an image or a directory of images on the client",
      cxxopts::value(args.path_to_image))
    ("labels", "Path to the text file containing the labels on the client",
      cxxopts::value(args.path_to_labels))
    ("http-port", "Port to use for HTTP server",
      cxxopts::value(args.http_port))
    ("grpc-port", "Port to use for gRPC server",
      cxxopts::value(args.grpc_port))
    ("input-size", "Size of the square image in pixels",
      cxxopts::value(args.input_size))
    ("input-node", "Name of the input node",
      cxxopts::value(args.input_node))
    ("output-node", "Name of the output node",
      cxxopts::value(args.output_node))
    ("top", "Number of top categories to print",
      cxxopts::value(args.top))
    ("output-classes", "Number of output classes for this model",
      cxxopts::value(args.output_classes))
    ("help", "Print help");
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result.count("help") != 0U) {
      std::cout << options.help({""}) << "\n";
      exit(0);
    }
  } catch (const cxxopts::OptionException& e) {
    std::cerr << "Error parsing options: " << e.what() << "\n";
    exit(1);
  }

  if (args.path_to_image.empty() || args.path_to_model.empty() ||
      args.path_to_labels.empty()) {
    const char* root_path = std::getenv("PROTEUS_ROOT");
    if (root_path == nullptr) {
      std::cerr << "PROTEUS_ROOT is not defined in the environment\n";
      std::cerr << "-> Needed to infer default values for arguments\n";
      std::cerr << "Either:\n - define PROTEUS_ROOT in the environment\n";
      std::cerr << " - pass all the following flags:\n";
      std::cerr << "     --image\n";
      std::cerr << "     --model\n";
      std::cerr << "     --labels\n";
      exit(1);
    }
    fs::path root{root_path};

    if (args.path_to_image.empty()) {
      args.path_to_image = root / "tests/assets/dog-3619020_640.jpg";
    }

    // args.path_to_model is unset and set by each example

    if (args.path_to_labels.empty()) {
      args.path_to_labels = root / "examples/resnet50/imagenet_classes.txt";
    }
  }

  return args;
}

std::vector<std::string> resolveImagePaths(const fs::path& path_to_image) {
  std::vector<std::string> image_paths;
  if (fs::is_directory(path_to_image)) {
    for (const auto& entry : fs::directory_iterator(path_to_image)) {
      image_paths.emplace_back(entry.path().string());
    }
  } else {
    image_paths.emplace_back(path_to_image.string());
  }
  return image_paths;
}

void printLabel(const std::vector<int>& indices, const fs::path& path_to_labels,
                const std::string& name) {
  std::ifstream in(path_to_labels);
  std::vector<std::string> labels;

  std::string line;
  while (std::getline(in, line)) {
    labels.push_back(std::move(line));
  }

  std::cout << "Top " << indices.size() << " classes for " << name << ":\n";
  for (const auto& index : indices) {
    std::cout << "  " << labels[index] << "\n";
  }
}

#endif  // GUARD_EXAMPLES_RESNET50_RESNET50
