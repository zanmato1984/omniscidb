#include "Nurgi/Nurgi.h"
#include "QueryRunner/QueryRunner.h"
#include "TestHelpers.h"

#include <gtest/gtest.h>

using namespace Nurgi;

TEST(NurgiLegacy, RunMatGbyAgg) {
  const std::string nurgi_json = R"({"rels": [
{
  "id": "0",
  "relOp": "EnumerableTableScan",
  "fieldNames": [
    "x",
    "y",
    "rowid"
  ],
  "table": [
    "omnisci",
    "t"
  ],
  "inputs": []
},
{
  "id": "1",
  "relOp": "LogicalFilter",
  "condition": {
    "op": "=",
    "operands": [
      {"input": 0},
      {
        "literal": 10001,
        "type": "DECIMAL",
        "target_type": "INTEGER",
        "scale": 0,
        "precision": 1,
        "type_scale": 0,
        "type_precision": 10
      }
    ],
    "type": {
      "type": "BOOLEAN",
      "nullable": true
    }
  }
},
{
  "id": "2",
  "relOp": "LogicalProject",
  "fields": ["y", "x"],
  "exprs": [
    {"input": 1},
    {"input": 0}
  ]
},
{
  "id": "3",
  "relOp": "LogicalAggregate",
  "fields": ["y", "cnt"],
  "group": [0],
  "aggs": [
    {
      "agg": "COUNT",
      "type": {
        "type": "BIGINT",
        "nullable": false
      },
      "distinct": false,
      "operands": [
        1
      ]
    }
  ]
},
{
  "id": "4",
  "relOp": "LogicalSort",
  "collation": [
    {
      "field": 0,
      "direction": "ASCENDING",
      "nulls": "LAST"
    }
  ]
}
]})";

  Context context({}, {});
  auto device_type =
#ifdef HAVE_CUDA
      ExecutorDeviceType::GPU
#else
      ExecutorDeviceType::CPU
#endif
      ;
  ASSERT_EQ(runMat(nurgi_json, context, device_type), 0);
  ASSERT_EQ(context.mat_output.size, 2);
  ASSERT_EQ(context.mat_output.columns.size(), 2);
  ASSERT_EQ(context.mat_output.columns[0].size, 2);
  ASSERT_EQ(reinterpret_cast<const int*>(context.mat_output.columns[0].data)[0], 11001);
  ASSERT_EQ(reinterpret_cast<const int*>(context.mat_output.columns[0].data)[1], 11002);
  ASSERT_EQ(context.mat_output.columns[1].size, 2);
  ASSERT_EQ(reinterpret_cast<const int*>(context.mat_output.columns[1].data)[0], 2);
  ASSERT_EQ(reinterpret_cast<const int*>(context.mat_output.columns[1].data)[1], 1);
}

int main(int argc, char** argv) {
  TestHelpers::init_logger_stderr_only(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  QueryRunner::QueryRunner::init("./data");
  int err = RUN_ALL_TESTS();
  return err;
}
