#include "Catalog.h"
#include "QueryEngine/Descriptors/RowSetMemoryOwner.h"
#include "QueryEngine/ResultSet.h"

namespace Nurgi::Catalog {

void TableDescriptor::getRows() {
  // TODO: Real logic.
  std::vector<TargetInfo> targets;
  QueryMemoryDescriptor query_mem_desc;
  auto row_set_mem_owner = std::make_shared<RowSetMemoryOwner>();
  rows = std::make_shared<ResultSet>(
      targets, ExecutorDeviceType::CPU, query_mem_desc, row_set_mem_owner, nullptr);
}

}  // namespace Nurgi::Catalog
