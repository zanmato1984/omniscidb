#include "Nurgi.h"
#include "QueryEngine/Descriptors/RelAlgExecutionDescriptor.h"
#include "QueryEngine/Execute.h"
#include "QueryRunner/QueryRunner.h"

namespace Nurgi {

Context::Context(std::unordered_map<int, const MatTableData>&& mat_inputs_,
                 MatTableData&& mat_output_)
    : mat_inputs(std::move(mat_inputs_))
    , mat_output(std::move(mat_output_))
    , row_set_mem_owner(
          std::make_shared<RowSetMemoryOwner>(Executor::getArenaBlockSize())) {}

void Context::obtainResult(std::shared_ptr<ExecutionResult> result) {
  const ResultSet& rows = *result->getRows();
  size_t num_cols = rows.colCount();
  std::vector<SQLTypeInfo> col_types;
  for (size_t i = 0; i < num_cols; ++i) {
    col_types.emplace_back(get_logical_type_info(rows.getColType(i)));
  }
  ColumnarResults columnar_results(row_set_mem_owner, rows, num_cols, col_types);

  mat_output.size = columnar_results.size();
  const auto& col_bufs = columnar_results.getColumnBuffers();
  for (size_t i = 0; i < col_bufs.size(); i++) {
    mat_output.columns.emplace_back(MatColumnData{col_bufs[i], mat_output.size});
  }
}

int runMat(const std::string& ra_str, Context& context, ExecutorDeviceType device_type) {
  try {
    auto result = QueryRunner::QueryRunner::get()->runNurgiRelAlg(
        ra_str, &context, device_type, true);
    CHECK(result);
    context.obtainResult(result);
  } catch (const std::exception& e) {
    LOG(ERROR) << e.what();
    return -1;
  }
  return 0;
}

}  // namespace Nurgi