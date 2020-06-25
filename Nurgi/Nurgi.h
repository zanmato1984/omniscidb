#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Nurgi {

/// Column data that is fully materialized in host memory.
struct MatColumnData {
  int8_t* data = nullptr;
  size_t size = 0;
};

/// Table data that encapsulates multiple materialized column data.
struct MatTableData {
  std::vector<MatColumnData> columns;
  size_t size = 0;
};

struct Context {
  std::unordered_map<int, const MatTableData> mat_inputs;
  MatTableData mat_output;
};

/// Run a RA on the given materialized tables.
int runMat(const std::string& ra_str, Context& context);

}  // namespace Nurgi