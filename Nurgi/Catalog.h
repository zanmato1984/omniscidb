#pragma once

#include "Catalog/ColumnDescriptor.h"
#include "Nurgi.h"
#include "Shared/sqltypes.h"

#include <map>
#include <vector>

class ResultSet;
using ResultSetPtr = std::shared_ptr<ResultSet>;

namespace Nurgi::Catalog {

struct ColumnDescriptor {
  int id;
  SQLTypeInfo type;

  ColumnDescriptor(int id_, SQLTypeInfo&& type_) : id(id_), type(std::move(type_)) {}

  std::string getName() const { return std::to_string(id); }
};

struct TableDescriptor {
  int id;
  std::vector<std::shared_ptr<ColumnDescriptor>> columns;
  std::vector<std::shared_ptr<::ColumnDescriptor>> column_descs;
  const MatTableData& mat_table_data;

  TableDescriptor(int id_,
                  std::vector<std::shared_ptr<ColumnDescriptor>>&& columns_,
                  const MatTableData& mat_table_data_)
      : id(id_), columns(std::move(columns_)), mat_table_data(mat_table_data_) {
    getColumnDescs();
  }

  std::string getColumnName(int col_id) const {
    CHECK_LT(col_id, columns.size());
    return columns[col_id]->getName();
  }

  std::shared_ptr<::ColumnDescriptor> getColumnDesc(int col_id) const {
    CHECK_LT(col_id, column_descs.size());
    return column_descs[col_id];
  }

 private:
  void getColumnDescs();
};

}  // namespace Nurgi::Catalog