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

struct Context {
  std::vector<const TableData> inputs;
  TableData output;
};

int run(const std::string& ra_str, Context& context);

}  // namespace Nurgi