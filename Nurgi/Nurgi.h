#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Nurgi {

struct ColumnData {
  int8_t* data = nullptr;
  size_t size = 0;
};

struct TableData {
  std::vector<ColumnData> columns;
  size_t size = 0;
};

struct Context {
  std::unordered_map<int, const TableData> inputs;
  TableData output;
};

int run(const std::string& ra_str, Context& context);

}  // namespace Nurgi