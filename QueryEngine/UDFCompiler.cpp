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

#include "UDFCompiler.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/raw_ostream.h>
#include <boost/process/search_path.hpp>
#include <iterator>
#include <memory>

#include "Execute.h"
#include "Shared/Logger.h"

using namespace clang;
using namespace clang::tooling;

static llvm::cl::OptionCategory ToolingSampleCategory("UDF Tooling");

namespace {

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.

class FunctionDeclVisitor : public RecursiveASTVisitor<FunctionDeclVisitor> {
 public:
  FunctionDeclVisitor(llvm::raw_fd_ostream& ast_file,
                      SourceManager& s_manager,
                      ASTContext& context)
      : ast_file_(ast_file), source_manager_(s_manager), context_(context) {
    source_manager_.getDiagnostics().setShowColors(false);
  }

  bool VisitFunctionDecl(FunctionDecl* f) {
    // Only function definitions (with bodies), not declarations.
    if (f->hasBody()) {
      if (getMainFileName() == getFuncDeclFileName(f)) {
        auto printing_policy = context_.getPrintingPolicy();
        printing_policy.FullyQualifiedName = 1;
        printing_policy.UseVoidForZeroParams = 1;
        printing_policy.PolishForDeclaration = 1;
        printing_policy.TerseOutput = 1;
        f->print(ast_file_, printing_policy);
        ast_file_ << "\n";
      }
    }

    return true;
  }

 private:
  std::string getMainFileName() const {
    auto f_entry = source_manager_.getFileEntryForID(source_manager_.getMainFileID());
    return f_entry->getName().str();
  }

  std::string getFuncDeclFileName(FunctionDecl* f) const {
    SourceLocation spell_loc = source_manager_.getSpellingLoc(f->getLocation());
    PresumedLoc p_loc = source_manager_.getPresumedLoc(spell_loc);

    return std::string(p_loc.getFilename());
  }

 private:
  llvm::raw_fd_ostream& ast_file_;
  SourceManager& source_manager_;
  ASTContext& context_;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class DeclASTConsumer : public ASTConsumer {
 public:
  DeclASTConsumer(llvm::raw_fd_ostream& ast_file,
                  SourceManager& s_manager,
                  ASTContext& context)
      : visitor_(ast_file, s_manager, context) {}

  // Override the method that gets called for each parsed top-level
  // declaration.
  bool HandleTopLevelDecl(DeclGroupRef decl_reference) override {
    for (DeclGroupRef::iterator b = decl_reference.begin(), e = decl_reference.end();
         b != e;
         ++b) {
      // Traverse the declaration using our AST visitor.
      visitor_.TraverseDecl(*b);
    }
    return true;
  }

 private:
  FunctionDeclVisitor visitor_;
};

// For each source file provided to the tool, a new FrontendAction is created.
class HandleDeclAction : public ASTFrontendAction {
 public:
  HandleDeclAction(llvm::raw_fd_ostream& ast_file) : ast_file_(ast_file) {}

  ~HandleDeclAction() override {}

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& instance,
                                                 StringRef file) override {
    return std::make_unique<DeclASTConsumer>(
        ast_file_, instance.getSourceManager(), instance.getASTContext());
  }

 private:
  llvm::raw_fd_ostream& ast_file_;
};

class ToolFactory : public FrontendActionFactory {
 public:
#if LLVM_VERSION_MAJOR >= 10
  using FrontendActionPtr = std::unique_ptr<clang::FrontendAction>;
#define CREATE_FRONTEND_ACTION(ast_file_) std::make_unique<HandleDeclAction>(ast_file_)
#else
  using FrontendActionPtr = clang::FrontendAction*;
#define CREATE_FRONTEND_ACTION(ast_file_) new HandleDeclAction(ast_file_)
#endif

  ToolFactory(llvm::raw_fd_ostream& ast_file) : ast_file_(ast_file) {}

  FrontendActionPtr create() override { return CREATE_FRONTEND_ACTION(ast_file_); }

 private:
  llvm::raw_fd_ostream& ast_file_;
};

const char* convert(const std::string& s) {
  return s.c_str();
}
}  // namespace

UdfClangDriver::UdfClangDriver(const std::string& clang_path)
    : diag_options(new DiagnosticOptions())
    , diag_client(new TextDiagnosticPrinter(llvm::errs(), diag_options.get()))
    , diag_id(new clang::DiagnosticIDs())
    , diags(diag_id, diag_options.get(), diag_client)
    , diag_client_owner(diags.takeClient())
    , the_driver(clang_path.c_str(), llvm::sys::getDefaultTargetTriple(), diags) {}

std::string UdfCompiler::removeFileExtension(const std::string& path) {
  if (path == "." || path == "..") {
    return path;
  }

  size_t pos = path.find_last_of("\\/.");
  if (pos != std::string::npos && path[pos] == '.') {
    return path.substr(0, pos);
  }

  return path;
}

std::string UdfCompiler::getFileExt(std::string& s) {
  size_t i = s.rfind('.', s.length());
  if (1 != std::string::npos) {
    return (s.substr(i + 1, s.length() - i));
  }
}

void UdfCompiler::replaceExtn(std::string& s, const std::string& new_ext) {
  std::string::size_type i = s.rfind('.', s.length());

  if (i != std::string::npos) {
    s.replace(i + 1, getFileExt(s).length(), new_ext);
  }
}

std::string UdfCompiler::genGpuIrFilename(const char* udf_file_name) {
  std::string gpu_file_name(removeFileExtension(udf_file_name));

  gpu_file_name += "_gpu.bc";
  return gpu_file_name;
}

std::string UdfCompiler::genCpuIrFilename(const char* udf_fileName) {
  std::string cpu_file_name(removeFileExtension(udf_fileName));

  cpu_file_name += "_cpu.bc";
  return cpu_file_name;
}

int UdfCompiler::compileFromCommandLine(const std::vector<std::string>& command_line) {
  UdfClangDriver compiler_driver(clang_path_);
  auto the_driver(compiler_driver.getClangDriver());

  std::vector<const char*> clang_command_opts;
  clang_command_opts.reserve(command_line.size() + clang_options_.size());
  // add required options first
  std::transform(std::begin(command_line),
                 std::end(command_line),
                 std::back_inserter(clang_command_opts),
                 [&](const std::string& str) { return str.c_str(); });

  // If there were additional clang options passed to the system, append them here
  if (!clang_options_.empty()) {
    std::transform(std::begin(clang_options_),
                   std::end(clang_options_),
                   std::back_inserter(clang_command_opts),
                   [&](const std::string& str) { return str.c_str(); });
  }

  the_driver->CCPrintOptions = 0;
  std::unique_ptr<driver::Compilation> compilation(
      the_driver->BuildCompilation(clang_command_opts));

  if (!compilation) {
    LOG(FATAL) << "failed to build compilation object!\n";
  }

  llvm::SmallVector<std::pair<int, const driver::Command*>, 10> failing_commands;
  int res = the_driver->ExecuteCompilation(*compilation, failing_commands);

  if (res < 0) {
    for (const std::pair<int, const driver::Command*>& p : failing_commands) {
      if (p.first) {
        the_driver->generateCompilationDiagnostics(*compilation, *p.second);
      }
    }
  }

  return res;
}

int UdfCompiler::compileToGpuByteCode(const char* udf_file_name, bool cpu_mode) {
  std::string gpu_out_filename(genGpuIrFilename(udf_file_name));

  std::vector<std::string> command_line{
      clang_path_, "-c", "-O2", "-emit-llvm", "-o", gpu_out_filename, "-std=c++14"};

  // If we are not compiling for cpu mode, then target the gpu
  // Otherwise assume we can generic ir that will
  // be translated to gpu code during target code generation
  if (!cpu_mode) {
    command_line.emplace_back("--cuda-gpu-arch=" +
                              CudaMgr_Namespace::CudaMgr::deviceArchToSM(target_arch_));
    command_line.emplace_back("--cuda-device-only");
    command_line.emplace_back("-xcuda");
  }

  command_line.emplace_back(udf_file_name);

  return compileFromCommandLine(command_line);
}

int UdfCompiler::compileToCpuByteCode(const char* udf_file_name) {
  std::string cpu_out_filename(genCpuIrFilename(udf_file_name));

  std::vector<std::string> command_line{clang_path_,
                                        "-c",
                                        "-O2",
                                        "-emit-llvm",
                                        "-o",
                                        cpu_out_filename,
                                        "-std=c++14",
                                        udf_file_name};

  return compileFromCommandLine(command_line);
}

int UdfCompiler::parseToAst(const char* file_name) {
  UdfClangDriver the_driver(clang_path_);
  std::string resource_path = the_driver.getClangDriver()->ResourceDir;
  std::string include_option =
      std::string("-I") + resource_path + std::string("/include");

  std::vector<std::string> arg_vector;
  arg_vector.emplace_back("astparser");
  arg_vector.emplace_back(file_name);
  arg_vector.emplace_back("--");
  arg_vector.emplace_back(include_option);

  if (clang_options_.size() > 0) {
    std::copy(
        clang_options_.begin(), clang_options_.end(), std::back_inserter(arg_vector));
  }

  std::vector<const char*> arg_vec2;
  std::transform(
      arg_vector.begin(), arg_vector.end(), std::back_inserter(arg_vec2), convert);

  int num_args = arg_vec2.size();
  CommonOptionsParser op(num_args, &arg_vec2[0], ToolingSampleCategory);
  ClangTool tool(op.getCompilations(), op.getSourcePathList());

  std::string out_name(file_name);
  std::string file_ext("ast");
  replaceExtn(out_name, file_ext);

  std::error_code out_error_info;
  llvm::raw_fd_ostream out_file(
      llvm::StringRef(out_name), out_error_info, llvm::sys::fs::F_None);

  auto factory = std::make_unique<ToolFactory>(out_file);
  return tool.run(factory.get());
}

const std::string& UdfCompiler::getAstFileName() const {
  return udf_ast_file_name_;
}

void UdfCompiler::init(const std::string& clang_path) {
  replaceExtn(udf_ast_file_name_, "ast");

  if (clang_path.empty()) {
    clang_path_.assign(llvm::sys::findProgramByName("clang++").get());
    if (clang_path_.empty()) {
      throw std::runtime_error(
          "Unable to find clang++ to compile user defined functions");
    }
  } else {
    clang_path_.assign(clang_path);

    if (!boost::filesystem::exists(clang_path)) {
      throw std::runtime_error("Path provided for udf compiler " + clang_path +
                               " does not exist.");
    }

    if (boost::filesystem::is_directory(clang_path)) {
      throw std::runtime_error("Path provided for udf compiler " + clang_path +
                               " is not to the clang++ executable.");
    }
  }
}

UdfCompiler::UdfCompiler(const std::string& file_name,
                         CudaMgr_Namespace::NvidiaDeviceArch target_arch,
                         const std::string& clang_path)
    : udf_file_name_(file_name)
    , udf_ast_file_name_(file_name)
    , target_arch_(target_arch) {
  init(clang_path);
}

UdfCompiler::UdfCompiler(const std::string& file_name,
                         CudaMgr_Namespace::NvidiaDeviceArch target_arch,
                         const std::string& clang_path,
                         const std::vector<std::string> clang_options)
    : udf_file_name_(file_name)
    , udf_ast_file_name_(file_name)
    , target_arch_(target_arch)
    , clang_options_(clang_options) {
  init(clang_path);
}

void UdfCompiler::readCpuCompiledModule() {
  std::string cpu_ir_file(genCpuIrFilename(udf_file_name_.c_str()));

  VLOG(1) << "UDFCompiler cpu bc file = " << cpu_ir_file;

  read_udf_cpu_module(cpu_ir_file);
}

void UdfCompiler::readGpuCompiledModule() {
  std::string gpu_ir_file(genGpuIrFilename(udf_file_name_.c_str()));

  VLOG(1) << "UDFCompiler gpu bc file = " << gpu_ir_file;

  read_udf_gpu_module(gpu_ir_file);
}

void UdfCompiler::readCompiledModules() {
  readCpuCompiledModule();
  readGpuCompiledModule();
}

int UdfCompiler::compileForGpu() {
  int gpu_compile_result = 1;

  gpu_compile_result = compileToGpuByteCode(udf_file_name_.c_str(), false);

  // If gpu compilation fails but cpu compilation has succeeded, try compiling
  // for the cpu with the assumption the user does not have the CUDA toolkit
  // installed
  if (gpu_compile_result != 0) {
    gpu_compile_result = compileToGpuByteCode(udf_file_name_.c_str(), true);
  }

  return gpu_compile_result;
}

int UdfCompiler::compileUdf() {
  LOG(INFO) << "UDFCompiler filename to compile: " << udf_file_name_;
  if (!boost::filesystem::exists(udf_file_name_)) {
    LOG(FATAL) << "User defined function file " << udf_file_name_ << " does not exist.";
    return 1;
  }

  auto ast_result = parseToAst(udf_file_name_.c_str());

  if (ast_result == 0) {
    // Compile udf file to generate cpu and gpu bytecode files

    int cpu_compile_result = compileToCpuByteCode(udf_file_name_.c_str());
#ifdef HAVE_CUDA
    int gpu_compile_result = 1;
#endif

    if (cpu_compile_result == 0) {
      readCpuCompiledModule();
#ifdef HAVE_CUDA
      gpu_compile_result = compileForGpu();

      if (gpu_compile_result == 0) {
        readGpuCompiledModule();
      } else {
        LOG(FATAL) << "Unable to compile UDF file for gpu";
        return 1;
      }
#endif
    } else {
      LOG(FATAL) << "Unable to compile UDF file for cpu";
      return 1;
    }
  } else {
    LOG(FATAL) << "Unable to create AST file for udf compilation";
    return 1;
  }

  return 0;
}
