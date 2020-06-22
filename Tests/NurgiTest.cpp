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
{"id": 200, "type": 6, "nullable": false},
{"id": 201, "type": 6, "nullable": false}
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

TEST(Nurgi, Run) {
  std::vector<TableData> inputs;
  TableData output;
  ASSERT_EQ(run(nurgi_json, inputs, output), 0);
}

int main(int argc, char** argv) {
  TestHelpers::init_logger_stderr_only(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  QueryRunner::QueryRunner::init("./data");
  int err = RUN_ALL_TESTS();
  return err;
}
