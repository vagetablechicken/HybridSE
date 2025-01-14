/*
 * Copyright 2021 4Paradigm
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include "codegen/fn_ir_builder.h"
#include "gtest/gtest.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils.h"
#include "node/node_manager.h"
#include "parser/parser.h"
#include "vm/schemas_context.h"

using namespace llvm;       // NOLINT (build/namespaces)
using namespace llvm::orc;  // NOLINT (build/namespaces)

ExitOnError ExitOnErr;

namespace hybridse {
namespace codegen {

class BlockIRBuilderTest : public ::testing::Test {
 public:
    BlockIRBuilderTest() {
        manager_ = new node::NodeManager();
        parser_ = new parser::HybridSeParser();
    }
    ~BlockIRBuilderTest() { delete manager_; }

 protected:
    node::NodeManager *manager_;
    parser::HybridSeParser *parser_;
};

void CheckResult(node::NodeManager *manager, std::string test, int32_t res,
                 int32_t a, int32_t b) {
    node::NodePointVector trees;
    node::PlanNodeList plan_trees;
    base::Status status;
    parser::HybridSeParser parser;
    int ret = parser.parse(test, trees, manager, status);
    ASSERT_EQ(0, ret);
    // Create an LLJIT instance.
    auto ctx = llvm::make_unique<LLVMContext>();
    auto m = make_unique<Module>("custom_fn", *ctx);
    FnIRBuilder fn_ir_builder(m.get());
    ::llvm::Function *func = nullptr;
    bool ok = fn_ir_builder.Build(dynamic_cast<node::FnNodeFnDef *>(trees[0]),
                                  &func, status);
    ASSERT_TRUE(ok);
    m->print(::llvm::errs(), NULL, true, true);
    LOG(INFO) << "before opt with ins cnt " << m->getInstructionCount();
    ::llvm::legacy::FunctionPassManager fpm(m.get());
    fpm.add(::llvm::createPromoteMemoryToRegisterPass());
    fpm.doInitialization();
    ::llvm::Module::iterator it;
    ::llvm::Module::iterator end = m->end();
    for (it = m->begin(); it != end; ++it) {
        fpm.run(*it);
    }
    LOG(INFO) << "after opt with ins cnt " << m->getInstructionCount();
    m->print(::llvm::errs(), NULL, true, true);
    auto J = ExitOnErr(LLJITBuilder().create());
    ExitOnErr(J->addIRModule(ThreadSafeModule(std::move(m), std::move(ctx))));
    auto test_jit = ExitOnErr(J->lookup("test.int32.int32"));
    int32_t (*test_fn)(int32_t, int32_t) =
        (int32_t(*)(int32_t, int32_t))test_jit.getAddress();
    ASSERT_EQ(res, test_fn(a, b));
}

TEST_F(BlockIRBuilderTest, test_mutable_variable_test) {
    const std::string test =
        "%%fun\n"
        "def test(x:i32,y:i32):i32\n"
        "    sum = 0\n"
        "    sum = sum + x\n"
        "    sum = sum + y\n"
        "    sum = sum + 1\n"
        "    return sum\n"
        "end";

    CheckResult(manager_, test, 6, 2, 3);
}

TEST_F(BlockIRBuilderTest, test_if_block) {
    const std::string test =
        "%%fun\n"
        "def test(x:i32,y:i32):i32\n"
        "    if x > 1\n"
        "    \treturn x+y\n"
        "    return x*y\n"
        "end";

    CheckResult(manager_, test, 5, 2, 3);
    CheckResult(manager_, test, 3, 1, 3);
    CheckResult(manager_, test, 2, 1, 2);
}

TEST_F(BlockIRBuilderTest, test_if_else_block) {
    const std::string test =
        "%%fun\n"
        "def test(x:i32,y:i32):i32\n"
        "    if x > 1\n"
        "    \treturn x+y\n"
        "    else\n"
        "    \treturn x*y\n"
        "end";

    CheckResult(manager_, test, 5, 2, 3);
    CheckResult(manager_, test, 3, 1, 3);
    CheckResult(manager_, test, 2, 1, 2);
}

TEST_F(BlockIRBuilderTest, test_if_elif_else_block) {
    const std::string test =
        "%%fun\n"
        "def test(x:i32,y:i32):i32\n"
        "    if x > 1\n"
        "    \treturn x+y\n"
        "    elif y >2\n"
        "    \treturn x-y\n"
        "    else\n"
        "    \treturn x*y\n"
        "end";

    CheckResult(manager_, test, 5, 2, 3);
    CheckResult(manager_, test, -2, 1, 3);
    CheckResult(manager_, test, 2, 1, 2);
}

TEST_F(BlockIRBuilderTest, test_if_else_block_redundant_ret) {
    const std::string test =
        "%%fun\n"
        "def test(x:i32,y:i32):i32\n"
        "    if x > 1\n"
        "    \treturn x+y\n"
        "    elif y >2\n"
        "    \treturn x-y\n"
        "    else\n"
        "    \treturn x*y\n"
        "    return 0\n"
        "end";

    CheckResult(manager_, test, 5, 2, 3);
    CheckResult(manager_, test, -2, 1, 3);
    CheckResult(manager_, test, 2, 1, 2);
}

TEST_F(BlockIRBuilderTest, test_if_else_mutable_var_block) {
    const std::string test =
        "%%fun\n"
        "def test(x:i32,y:i32):i32\n"
        "    ret = 0\n"
        "    if x > 1\n"
        "    \tret = x+y\n"
        "    elif y >2\n"
        "    \tret = x-y\n"
        "    else\n"
        "    \tret = x*y\n"
        "    return ret\n"
        "end";
    CheckResult(manager_, test, 5, 2, 3);
    CheckResult(manager_, test, -2, 1, 3);
    CheckResult(manager_, test, 2, 1, 2);
}

}  // namespace codegen
}  // namespace hybridse

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    return RUN_ALL_TESTS();
}
