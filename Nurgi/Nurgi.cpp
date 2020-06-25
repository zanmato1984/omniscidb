#include "Nurgi.h"
#include "QueryEngine/CompilationOptions.h"
#include "QueryEngine/Descriptors/RelAlgExecutionDescriptor.h"
#include "QueryRunner/QueryRunner.h"

namespace Nurgi {

int runMat(const std::string& ra_str, Context& context) {
  auto device_type =
#ifdef HAVE_CUDA
      ExecutorDeviceType::GPU
#else
      ExecutorDeviceType::CPU
#endif
      ;
  QueryRunner::QueryRunner::get()->runNurgiRelAlg(ra_str, &context, device_type, true);
  return 0;
}

}  // namespace Nurgi