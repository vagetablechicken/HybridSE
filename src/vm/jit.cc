/*
 * jit.cc
 * Copyright (C) 4paradigm.com 2019 wangtaize <wangtaize@4paradigm.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vm/jit.h"

#include <utility>
#include "glog/logging.h"

namespace fesql {
namespace vm {
using ::llvm::orc::LLJIT;

FeSQLJIT::FeSQLJIT(::llvm::orc::LLJITBuilderState& s,
        ::llvm::Error& e):LLJIT(s,e) {}
FeSQLJIT::~FeSQLJIT() {

}

::llvm::Error FeSQLJIT::AddIRModule(::llvm::orc::JITDylib& jd, // NOLINT
            ::llvm::orc::ThreadSafeModule tsm,
            ::llvm::orc::VModuleKey key) {

    if (auto Err = applyDataLayout(*tsm.getModule()))
        return Err;
    LOG(INFO) << "add a module with key " << key;
    return CompileLayer->add(jd, std::move(tsm), key);
}

::llvm::orc::VModuleKey FeSQLJIT::CreateVModule() {
    ::llvm::orc::VModuleKey key =  ES->allocateVModule();
    LOG(INFO) << "allocate a new module key "<< key;
    return key;
}

void FeSQLJIT::ReleaseVModule(::llvm::orc::VModuleKey key) {
    LOG(INFO) << "release module with key " <<  key;
    ES->releaseVModule(key);
}

}  // namespace vm
}  // namespace fesql
