#pragma once

namespace Nurgi {

namespace RelAlg {

struct RelScan : public RelAlgNode {
 public:
  RelScan(const Catalog::TableDescriptor* td_,
          const std::vector<std::string>& field_names_)
      : td(td_), field_names(field_names_) {}

  size_t size() const override { return field_names.size(); }

  const Catalog::TableDescriptor* getTableDescriptor() const { return td; }

  const std::vector<std::string>& getFieldNames() const { return field_names; }

  const std::string getFieldName(const size_t i) const { return field_names[i]; }

  std::string toString() const override {
    return "(NurgiRelScan<" + std::to_string(reinterpret_cast<uint64_t>(this)) + "> " +
           std::to_string(td->id) + ")";
  }

  std::shared_ptr<RelAlgNode> deepCopy() const override {
    CHECK(false);
    return nullptr;
  };

 private:
  const Catalog::TableDescriptor* td;
  const std::vector<std::string> field_names;
};

}  // namespace RelAlg

}  // namespace Nurgi