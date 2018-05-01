/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_COMPILER_OPTIMIZING_BUILDER_H_
#define ART_COMPILER_OPTIMIZING_BUILDER_H_

#include "base/arena_object.h"
#include "dex_file-inl.h"
#include "dex_file.h"
#include "driver/compiler_driver.h"
#include "nodes.h"

namespace art {

class ArtMethod;
class CodeGenerator;
class DexCompilationUnit;
class OptimizingCompilerStats;

class HGraphBuilder : public ValueObject {
 public:
  HGraphBuilder(HGraph* graph,
                const DexFile::CodeItem* code_item,
                const DexCompilationUnit* dex_compilation_unit,
                const DexCompilationUnit* outer_compilation_unit,
                CompilerDriver* driver,
                CodeGenerator* code_generator,
                OptimizingCompilerStats* compiler_stats,
                const uint8_t* interpreter_metadata,
                VariableSizedHandleScope* handles);

  // Only for unit testing.
  HGraphBuilder(HGraph* graph,
                const DexCompilationUnit* dex_compilation_unit,
                const DexFile::CodeItem& code_item,
                VariableSizedHandleScope* handles,
                DataType::Type return_type = DataType::Type::kInt32)
      : graph_(graph),
        dex_file_(&graph->GetDexFile()),
        code_item_(&code_item),
        dex_compilation_unit_(dex_compilation_unit),
        outer_compilation_unit_(nullptr),
        compiler_driver_(nullptr),
        code_generator_(nullptr),
        compilation_stats_(nullptr),
        interpreter_metadata_(nullptr),
        handles_(handles),
        return_type_(return_type) {}

  GraphAnalysisResult BuildGraph();
  void BuildIntrinsicGraph(ArtMethod* method);

  static constexpr const char* kBuilderPassName = "builder";

 private:
  bool SkipCompilation(size_t number_of_branches);

  HGraph* const graph_;
  const DexFile* const dex_file_;
  const DexFile::CodeItem* const code_item_;  // null for intrinsic graph.

  // The compilation unit of the current method being compiled. Note that
  // it can be an inlined method.
  const DexCompilationUnit* const dex_compilation_unit_;

  // The compilation unit of the enclosing method being compiled.
  const DexCompilationUnit* const outer_compilation_unit_;

  CompilerDriver* const compiler_driver_;
  CodeGenerator* const code_generator_;

  OptimizingCompilerStats* const compilation_stats_;
  const uint8_t* const interpreter_metadata_;
  VariableSizedHandleScope* const handles_;
  const DataType::Type return_type_;

  DISALLOW_COPY_AND_ASSIGN(HGraphBuilder);
};

}  // namespace art

#endif  // ART_COMPILER_OPTIMIZING_BUILDER_H_
