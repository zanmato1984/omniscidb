#pragma once

#include "Catalog.h"
#include "QueryEngine/RelAlgDagBuilder.h"

namespace Nurgi::RelAlg {

struct RelScan : public RelAlgNode {
 public:
  RelScan(std::shared_ptr<Catalog::TableDescriptor> td_) : td(td_) {}

  size_t size() const override { return td->columns.size(); }

  std::shared_ptr<Catalog::TableDescriptor> getTableDescriptor() const { return td; }

  std::string getFieldName(const size_t i) const { return td->getColumnName(i); }

  std::string toString() const override {
    return "(NurgiRelScan<" + std::to_string(reinterpret_cast<uint64_t>(this)) + "> " +
           std::to_string(td->id) + ")";
  }

  std::shared_ptr<RelAlgNode> deepCopy() const override {
    CHECK(false);
    return nullptr;
  };

 private:
  std::shared_ptr<Catalog::TableDescriptor> td;
};

}  // namespace Nurgi::RelAlg