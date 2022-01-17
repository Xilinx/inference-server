#include "proteus/batching/soft.hpp"

#include "gtest/gtest.h"
#include "proteus/core/worker_info.hpp"

namespace proteus {

TEST(UnitSoftBatcher, ConstructAndStart) {
  SoftBatcher batcher;
  batcher.setName("test");

  WorkerInfo fake("", nullptr);
  batcher.start(&fake);

  batcher.end();
}

}  // namespace proteus
