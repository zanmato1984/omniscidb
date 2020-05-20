#include "Nurgi/Nurgi.h"
#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace Nurgi;

TEST(Nurgi, Run) {
  std::vector<TableData> inputs;
  TableData output;
  ASSERT_EQ(run("foo", inputs, output), 0);
}

int main(int argc, char** argv) {
  TestHelpers::init_logger_stderr_only(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  int err = RUN_ALL_TESTS();
  return err;
}
