/*
 * Copyright 2016 The Android Open Source Project
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

#ifndef SYSFUZZER_VTSCOMPILER_CODE_GEN_PROFILER_HALHIDLPROFILERCODEGEN_H_
#define SYSFUZZER_VTSCOMPILER_CODE_GEN_PROFILER_HALHIDLPROFILERCODEGEN_H_

#include "code_gen/profiler/ProfilerCodeGenBase.h"

namespace android {
namespace vts {
/**
 * Class that generates the profiler code for Hidl HAL.
 */
class HalHidlProfilerCodeGen : public ProfilerCodeGenBase {
 public:
  HalHidlProfilerCodeGen(const std::string& input_vts_file_path,
    const std::string& vts_name)
      : ProfilerCodeGenBase(input_vts_file_path, vts_name) {};

 protected:
  virtual void GenerateProfilerForScalarVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) override;

  virtual void GenerateProfilerForStringVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) override;

  virtual void GenerateProfilerForEnumVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) override;

  virtual void GenerateProfilerForVectorVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) override;

  virtual void GenerateProfilerForArrayVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) override;

  virtual void GenerateProfilerForStructVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) override;

  virtual void GenerateProfilerForUnionVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) override;

  virtual void GenerateProfilerForHidlCallbackVariable(Formatter& out,
    const VariableSpecificationMessage& val, const std::string& arg_name,
    const std::string& arg_value) override;

  virtual void GenerateProfilerForMethod(Formatter& out,
    const FunctionSpecificationMessage& method) override;

  virtual void GenerateHeaderIncludeFiles(Formatter& out,
    const ComponentSpecificationMessage& message) override;
  virtual void GenerateSourceIncludeFiles(Formatter& out,
    const ComponentSpecificationMessage& message) override;
  void GenerateUsingDeclaration(Formatter& out,
    const ComponentSpecificationMessage& message) override;
  void GenerateMacros(Formatter& out,
    const ComponentSpecificationMessage& message) override;
  virtual void GenerateProfierSanityCheck(Formatter& out,
    const ComponentSpecificationMessage& message) override;
  virtual void GenerateLocalVariableDefinition(Formatter& out,
    const ComponentSpecificationMessage& message) override;

 private:
  DISALLOW_COPY_AND_ASSIGN (HalHidlProfilerCodeGen);
};

}  // namespace vts
}  // namespace android
#endif  // SYSFUZZER_VTSCOMPILER_CODE_GEN_PROFILER_HALHIDLPROFILERCODEGEN_H_
