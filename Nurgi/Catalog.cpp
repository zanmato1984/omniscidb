#include "Catalog.h"
#include "QueryEngine/Descriptors/RowSetMemoryOwner.h"

namespace Nurgi::Catalog {

void TableDescriptor::getColumnDescs() {
  for (const auto& col : columns) {
    column_descs.emplace_back(
        std::make_shared<::ColumnDescriptor>(id, col->id, col->getName(), col->type));
  }
}

}  // namespace Nurgi::Catalog
