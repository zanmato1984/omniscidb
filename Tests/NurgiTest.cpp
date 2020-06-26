#include "Nurgi/Nurgi.h"
#include "QueryRunner/QueryRunner.h"
#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace Nurgi;

const std::string nurgi_json = R"({"rels": [
{
"id": "0",
"relOp": "NurgiTableScan",
"table": {
"id": 100,
"cols": [
{"type": 6, "nullable": false},
{"type": 6, "nullable": false}
]
}
},
{
"id": "1",
"relOp": "LogicalProject",
"fields": [
"y", "x"
],
"exprs": [
{
"input": 1
},
{
"input": 0
}
]
}
]
})";

TEST(Nurgi, RunMatProject) {
  size_t size = 3;
  auto col_200 = std::vector<int>{1000, 10001, 10002};
  auto col_201 = std::vector<int>{1100, 11001, 11002};
  MatTableData tab{{{reinterpret_cast<int8_t*>(col_200.data()), size},
                    {reinterpret_cast<int8_t*>(col_201.data()), size}},
                   size};
  Context context({{100, std::move(tab)}}, {});
  auto device_type =
#ifdef HAVE_CUDA
      ExecutorDeviceType::GPU
#else
      ExecutorDeviceType::CPU
#endif
      ;
  ASSERT_EQ(runMat(nurgi_json, context, device_type), 0);
  ASSERT_EQ(context.mat_output.size, size);
  ASSERT_EQ(context.mat_output.columns.size(), 2);
  ASSERT_EQ(
      std::memcmp(col_200.data(), context.mat_output.columns[1].data, sizeof(int) * size),
      0);
  ASSERT_EQ(
      std::memcmp(col_201.data(), context.mat_output.columns[0].data, sizeof(int) * size),
      0);
}

int main(int argc, char** argv) {
  TestHelpers::init_logger_stderr_only(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  QueryRunner::QueryRunner::init("./data");
  int err = RUN_ALL_TESTS();
  return err;
}
