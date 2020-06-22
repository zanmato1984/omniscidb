#pragma once

#include "Shared/sqltypes.h"

#include <map>
#include <vector>

namespace Nurgi::Catalog {

struct ColumnDescriptor {
  int id;
  SQLTypeInfo type;

  ColumnDescriptor(int id_, SQLTypeInfo&& type_) : id(id_), type(std::move(type_)) {}
};

using ColumnDescriptorVec = std::vector<std::shared_ptr<ColumnDescriptor>>;
using ColumnDescriptorMap = std::map<int, std::shared_ptr<ColumnDescriptor>>;

struct TableDescriptor {
  int id;
  ColumnDescriptorVec columns;
  ColumnDescriptorMap columns_map;

  TableDescriptor(int id_, ColumnDescriptorVec&& columns_)
      : id(id_), columns(std::move(columns_)) {
    for (const auto& col : columns) {
      columns_map.emplace(col->id, col);
    }
  }
};

}  // namespace Nurgi::Catalog