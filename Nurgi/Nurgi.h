#pragma once

#include "QueryEngine/CompilationOptions.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class ResultSet;
using ResultSetPtr = std::shared_ptr<ResultSet>;

namespace Nurgi {

/// Column data that is fully materialized in host memory.
struct MatColumnData {
  const int8_t* data = nullptr;
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

  Context(std::unordered_map<int, const MatTableData>&& mat_inputs_,
          MatTableData&& mat_output_)
      : mat_inputs(std::move(mat_inputs_)), mat_output(std::move(mat_output_)) {}

  void setResult(ResultSetPtr result_);

 private:
  ResultSetPtr result = nullptr;
};

/// Run a RA on the given materialized tables.
int runMat(const std::string& ra_str, Context& context, ExecutorDeviceType device_type);

}  // namespace Nurgi