#include <iostream>
#include <string>

#include "cxxopts/cxxopts.hpp"
#include "gtest/gtest.h"

GTEST_API_ int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);

  try {
    cxxopts::Options options("gtest-executable");
    options.add_options()("help", "Print help");

    auto result = options.parse(argc, argv);
    if (result.count("help") != 0U) {
      std::cout << options.help({""}) << "\n";
      return 0;
    }
  } catch (const cxxopts::OptionException& e) {
    std::cout << "Error parsing options: " << e.what() << "\n";
    return 1;
  }

  return RUN_ALL_TESTS();
}
