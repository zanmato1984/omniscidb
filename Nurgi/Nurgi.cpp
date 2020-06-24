#include "Nurgi.h"
#include "QueryEngine/CompilationOptions.h"
#include "QueryEngine/Descriptors/RelAlgExecutionDescriptor.h"
#include "QueryRunner/QueryRunner.h"

namespace Nurgi {

int run(const std::string& ra_str, Context& context) {
  QueryRunner::QueryRunner::get()->runNurgiRelAlg(
      ra_str, &context, ExecutorDeviceType::CPU, true);
  return 0;
}

}  // namespace Nurgi