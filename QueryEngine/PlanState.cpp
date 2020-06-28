/*
 * Copyright 2019 OmniSci, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PlanState.h"
#include "Execute.h"

bool PlanState::isLazyFetchColumn(const Analyzer::Expr* target_expr) {
  if (!allow_lazy_fetch_) {
    return false;
  }
  const auto do_not_fetch_column = dynamic_cast<const Analyzer::ColumnVar*>(target_expr);
  if (!do_not_fetch_column || dynamic_cast<const Analyzer::Var*>(do_not_fetch_column) ||
      dynamic_cast<const Analyzer::NurgiColumnVar*>(do_not_fetch_column)) {
    return false;
  }
  if (do_not_fetch_column->get_table_id() > 0) {
    auto cd = get_column_descriptor(do_not_fetch_column->get_column_id(),
                                    do_not_fetch_column->get_table_id(),
                                    *executor_->getCatalog());
    if (cd->isVirtualCol) {
      return false;
    }
  }
  std::set<std::pair<int, int>> intersect;
  std::set_intersection(columns_to_fetch_.begin(),
                        columns_to_fetch_.end(),
                        columns_to_not_fetch_.begin(),
                        columns_to_not_fetch_.end(),
                        std::inserter(intersect, intersect.begin()));
  if (!intersect.empty()) {
    throw CompilationRetryNoLazyFetch();
  }
  return columns_to_fetch_.find(std::make_pair(do_not_fetch_column->get_table_id(),
                                               do_not_fetch_column->get_column_id())) ==
         columns_to_fetch_.end();
}

void PlanState::allocateLocalColumnIds(
    const std::list<std::shared_ptr<const InputColDescriptor>>& global_col_ids) {
  for (const auto& col_id : global_col_ids) {
    CHECK(col_id);
    const auto local_col_id = global_to_local_col_ids_.size();
    const auto it_ok =
        global_to_local_col_ids_.insert(std::make_pair(*col_id, local_col_id));
    // enforce uniqueness of the column ids in the scan plan
    CHECK(it_ok.second);
  }
}

int PlanState::getLocalColumnId(const Analyzer::ColumnVar* col_var,
                                const bool fetch_column) {
  CHECK(col_var);
  const int table_id = col_var->get_table_id();
  int global_col_id = col_var->get_column_id();
  const int scan_idx = col_var->get_rte_idx();
  InputColDescriptor scan_col_desc(global_col_id, table_id, scan_idx);
  const auto it = global_to_local_col_ids_.find(scan_col_desc);
  CHECK(it != global_to_local_col_ids_.end()) << "Expected to find " << scan_col_desc;
  if (fetch_column) {
    columns_to_fetch_.insert(std::make_pair(table_id, global_col_id));
  }
  return it->second;
}

void PlanState::addNurgiTable(TableId table_id,
                              std::shared_ptr<NurgiTableDescriptor> nurgi_table) {
  CHECK(nurgi_tables_.find(table_id) == nurgi_tables_.end());
  nurgi_tables_.insert(std::make_pair(table_id, nurgi_table));
}

std::shared_ptr<NurgiTableDescriptor> PlanState::getNurgiTable(TableId table_id) {
  const auto nurgi_table_it = nurgi_tables_.find(table_id);
  if (nurgi_table_it == nurgi_tables_.end()) {
    return nullptr;
  }
  return nurgi_table_it->second;
}
