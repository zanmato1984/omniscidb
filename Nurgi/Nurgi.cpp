#include "Nurgi.h"
#include "QueryEngine/Descriptors/RelAlgExecutionDescriptor.h"
#include "QueryRunner/QueryRunner.h"

namespace Nurgi {

void Context::setResult(ResultSetPtr result_) {
  result = result_;

  mat_output.size = result->rowCount();
  for (size_t i = 0; i < result->colCount(); i++) {
    CHECK(result->isZeroCopyColumnarConversionPossible(i));
    mat_output.columns.emplace_back(
        MatColumnData{result->getColumnarBuffer(i), mat_output.size});
  }
}

int runMat(const std::string& ra_str, Context& context, ExecutorDeviceType device_type) {
  try {
    auto result = QueryRunner::QueryRunner::get()->runNurgiRelAlg(
        ra_str, &context, device_type, true);
    CHECK(result);
    context.setResult(result->getRows());
  } catch (const std::exception& e) {
    LOG(ERROR) << e.what();
    return -1;
  }
  return 0;
}

}  // namespace Nurgi