#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Nurgi {

struct ColumnData {
  int8_t* data;
  size_t size;
};

struct TableData {
  std::vector<ColumnData> columns;
};

int run(const std::string& ra_str,
        const std::vector<TableData>& inputs,
        TableData& output);

}  // namespace Nurgi