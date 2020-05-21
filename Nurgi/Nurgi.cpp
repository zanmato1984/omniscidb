#include "Nurgi.h"
#include "QueryEngine/CompilationOptions.h"
#include "QueryEngine/Descriptors/RelAlgExecutionDescriptor.h"
#include "QueryRunner/QueryRunner.h"

namespace Nurgi {

int run(const std::string& ra_str,
        const std::vector<TableData>& inputs,
        TableData& output) {
  QueryRunner::QueryRunner::get()->runNurgiRelAlg(ra_str, ExecutorDeviceType::GPU, true);
  return 0;
}

}  // namespace Nurgi