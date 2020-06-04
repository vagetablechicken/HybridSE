/*-------------------------------------------------------------------------
 * Copyright (C) 2020, 4paradigm
 * struct_ir_builder.h
 *
 * Author: chenjing
 * Date: 2020/5/26
 *--------------------------------------------------------------------------
 **/

#ifndef SRC_CODEGEN_STRUCT_IR_BUILDER_H_
#define SRC_CODEGEN_STRUCT_IR_BUILDER_H_
#include "base/fe_status.h"
#include "codegen/cast_expr_ir_builder.h"
#include "codegen/scope_var.h"
#include "codegen/type_ir_builder.h"
#include "llvm/IR/IRBuilder.h"
#include "proto/fe_type.pb.h"

namespace fesql {
namespace codegen {

class StructTypeIRBuilder : public TypeIRBuilder {
 public:
    explicit StructTypeIRBuilder(::llvm::Module*);
    ~StructTypeIRBuilder();
    virtual void InitStructType() = 0;
    ::llvm::Type* GetType();
    bool Create(::llvm::BasicBlock* block, ::llvm::Value** output);
    bool Get(::llvm::BasicBlock* block, ::llvm::Value* struct_value,
             unsigned int idx, ::llvm::Value** output);
    bool Set(::llvm::BasicBlock* block, ::llvm::Value* struct_value,
             unsigned int idx, ::llvm::Value* value);

 protected:
    ::llvm::Module* m_;
    ::llvm::Type* struct_type_;
};
}  // namespace codegen
}  // namespace fesql
#endif  // SRC_CODEGEN_STRUCT_IR_BUILDER_H_