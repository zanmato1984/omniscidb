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

#ifndef QUERY_RUNNER_H
#define QUERY_RUNNER_H

#include <fstream>
#include <memory>
#include <string>

#include "Catalog/SessionInfo.h"
#include "Catalog/SysCatalog.h"
#include "Catalog/TableDescriptor.h"
#include "LeafAggregator.h"
#include "QueryEngine/BaselineJoinHashTable.h"
#include "QueryEngine/CompilationOptions.h"
#include "QueryEngine/JoinHashTable.h"
#include "QueryEngine/OverlapsJoinHashTable.h"
#include "ThriftHandler/QueryState.h"

namespace Catalog_Namespace {
class Catalog;
struct UserMetadata;
}  // namespace Catalog_Namespace

class ResultSet;
class ExecutionResult;

namespace Parser {
class CopyTableStmt;
}

using query_state::QueryStateProxy;

namespace Importer_NS {
class Loader;
}

class Calcite;

namespace QueryRunner {

class QueryRunner {
 public:
  static QueryRunner* init(const char* db_path,
                           const std::string& udf_filename = "",
                           const size_t max_gpu_mem = 0,  // use all available mem
                           const int reserved_gpu_mem = 256 << 20) {
    return QueryRunner::init(db_path,
                             std::string{OMNISCI_ROOT_USER},
                             "HyperInteractive",
                             std::string{OMNISCI_DEFAULT_DB},
                             {},
                             {},
                             udf_filename,
                             true,
                             max_gpu_mem,
                             reserved_gpu_mem);
  }

  static QueryRunner* init(const char* db_path,
                           const std::vector<LeafHostInfo>& string_servers,
                           const std::vector<LeafHostInfo>& leaf_servers) {
    return QueryRunner::init(db_path,
                             std::string{OMNISCI_ROOT_USER},
                             "HyperInteractive",
                             std::string{OMNISCI_DEFAULT_DB},
                             string_servers,
                             leaf_servers);
  }

  static QueryRunner* init(const char* db_path,
                           const std::string& user,
                           const std::string& pass,
                           const std::string& db_name,
                           const std::vector<LeafHostInfo>& string_servers,
                           const std::vector<LeafHostInfo>& leaf_servers,
                           const std::string& udf_filename = "",
                           bool uses_gpus = true,
                           const size_t max_gpu_mem = 0,  // use all available mem
                           const int reserved_gpu_mem = 256 << 20,
                           const bool create_user = false,
                           const bool create_db = false);

  static QueryRunner* init(std::unique_ptr<Catalog_Namespace::SessionInfo>& session) {
    qr_instance_.reset(new QueryRunner(std::move(session)));
    return qr_instance_.get();
  }

  static QueryRunner* get() {
    if (!qr_instance_) {
      throw std::runtime_error("QueryRunner must be initialized before calling get().");
    }
    return qr_instance_.get();
  }

  static void reset();

  std::shared_ptr<Catalog_Namespace::SessionInfo> getSession() const {
    return session_info_;
  }
  std::shared_ptr<Catalog_Namespace::Catalog> getCatalog() const;
  std::shared_ptr<Calcite> getCalcite() const;

  bool gpusPresent() const;
  virtual void clearGpuMemory() const;
  virtual void clearCpuMemory() const;

  virtual void runDDLStatement(const std::string&);
  virtual std::shared_ptr<ResultSet> runSQL(const std::string& query_str,
                                            const ExecutorDeviceType device_type,
                                            const bool hoist_literals = true,
                                            const bool allow_loop_joins = true);
  virtual ExecutionResult runSelectQuery(const std::string& query_str,
                                         const ExecutorDeviceType device_type,
                                         const bool hoist_literals,
                                         const bool allow_loop_joins,
                                         const bool just_explain = false);
  virtual ExecutionResult runNurgiRelAlg(const std::string& nurgi_ra,
                                         const ExecutorDeviceType device_type,
                                         const bool allow_loop_joins,
                                         const bool just_explain = false);
  virtual std::vector<std::shared_ptr<ResultSet>> runMultipleStatements(
      const std::string&,
      const ExecutorDeviceType);

  virtual void runImport(Parser::CopyTableStmt* import_stmt);
  virtual std::unique_ptr<Importer_NS::Loader> getLoader(const TableDescriptor* td) const;

  const std::shared_ptr<std::vector<int32_t>>& getCachedJoinHashTable(size_t idx);
  const std::shared_ptr<std::vector<int8_t>>& getCachedBaselineHashTable(size_t idx);
  size_t getEntryCntCachedBaselineHashTable(size_t idx);
  uint64_t getNumberOfCachedJoinHashTables();
  uint64_t getNumberOfCachedBaselineJoinHashTables();

  virtual ~QueryRunner() {}

  QueryRunner(std::unique_ptr<Catalog_Namespace::SessionInfo> session);

  static query_state::QueryStates query_states_;

  template <typename... Ts>
  static std::shared_ptr<query_state::QueryState> create_query_state(Ts&&... args) {
    return query_states_.create(std::forward<Ts>(args)...);
  }

 protected:
  QueryRunner(const char* db_path,
              const std::string& user,
              const std::string& pass,
              const std::string& db_name,
              const std::vector<LeafHostInfo>& string_servers,
              const std::vector<LeafHostInfo>& leaf_servers,
              const std::string& udf_filename,
              bool uses_gpus,
              const size_t max_gpu_mem,
              const int reserved_gpu_mem,
              const bool create_user,
              const bool create_db);

  static std::unique_ptr<QueryRunner> qr_instance_;

  std::shared_ptr<Catalog_Namespace::SessionInfo> session_info_;
};

class ImportDriver : public QueryRunner {
 public:
  ImportDriver(std::shared_ptr<Catalog_Namespace::Catalog> cat,
               const Catalog_Namespace::UserMetadata& user,
               const ExecutorDeviceType dt = ExecutorDeviceType::GPU);

  void importGeoTable(const std::string& file_path,
                      const std::string& table_name,
                      const bool compression,
                      const bool create_table,
                      const bool explode_collections);
};

}  // namespace QueryRunner

#endif  // QUERY_RUNNER_H
