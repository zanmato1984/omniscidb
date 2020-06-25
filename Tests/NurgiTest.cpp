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
  size_t size = 0;
  auto col_200 = new int[]{1000, 10001, 10002};
  auto col_201 = new int[]{1100, 11001, 11002};
  MatTableData tab{{{reinterpret_cast<int8_t*>(col_200), size},
                    {reinterpret_cast<int8_t*>(col_201), size}},
                   size};
  Context context{{{100, std::move(tab)}}, {}};
  ASSERT_EQ(runMat(nurgi_json, context), 0);
}

int main(int argc, char** argv) {
  TestHelpers::init_logger_stderr_only(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  QueryRunner::QueryRunner::init("./data");
  int err = RUN_ALL_TESTS();
  return err;
}
