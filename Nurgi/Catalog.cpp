#include "Catalog.h"
#include "QueryEngine/Descriptors/RowSetMemoryOwner.h"
#include "QueryEngine/ResultSet.h"

namespace Nurgi::Catalog {

void TableDescriptor::getRows(const MatTableData& mat_table_data) {
  std::vector<TargetInfo> targets;
  QueryMemoryDescriptor query_mem_desc(
      nullptr, mat_table_data.size, QueryDescriptionType::Projection, false);
  auto row_set_mem_owner = std::make_shared<RowSetMemoryOwner>();
  for (const auto& column : columns) {
    targets.emplace_back(TargetInfo{
        false, kCOUNT, column->type, SQLTypeInfo(kNULLT, false), false, false});
    query_mem_desc.addColSlotInfo(
        {std::make_tuple(column->type.get_size(), column->type.get_logical_size())});
    row_set_mem_owner->addColBuffer(mat_table_data.columns[column->id].data);
  }
  rows = std::make_shared<ResultSet>(
      targets, ExecutorDeviceType::CPU, query_mem_desc, row_set_mem_owner, nullptr);
}

void TableDescriptor::getColumnDescs() {
  for (const auto& col : columns) {
    column_descs.emplace_back(
        std::make_shared<::ColumnDescriptor>(id, col->id, col->getName(), col->type));
  }
}

}  // namespace Nurgi::Catalog
