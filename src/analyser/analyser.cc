/*-------------------------------------------------------------------------
 * Copyright (C) 2019, 4paradigm
 * analyser.cc
 *
 * Author: chenjing
 * Date: 2019/10/30
 *--------------------------------------------------------------------------
 **/
#include "analyser/analyser.h"
#include <string>
#include "../node/node_enum.h"
namespace fesql {
namespace analyser {

int FeSQLAnalyser::Analyse(NodePointVector &parser_trees,
                           NodePointVector &query_trees,
                           Status &status) {  // NOLINT (runtime/references)
    if (parser_trees.empty()) {
        status.code = error::kAnalyserErrorParserTreeEmpty;
        status.msg = "fail to analyse: parser trees is empty";
        return status.code;
    }

    for (auto tree : parser_trees) {
        Analyse(tree, status);
        if (0 != status.code) {
            return status.code;
        }
        query_trees.push_back(tree);
    }
    return 0;
}

void FeSQLAnalyser::Analyse(SQLNode *parser_tree,
                            Status &status) {  // NOLINT (runtime/references)
    if (nullptr == parser_tree) {
        status.code = error::kPlanErrorNullNode;
        status.msg = "fail to analyse: parser tree node it null";
        return;
    }
    switch (parser_tree->GetType()) {
        case node::kSelectStmt:
            return TransformSelectNode(
                dynamic_cast<node::SelectStmt *>(parser_tree), status);
        case node::kCreateStmt:
            return TransformCreateNode(
                dynamic_cast<node::CreateStmt *>(parser_tree), status);
        case node::kCmdStmt:
            return TransformCmdNode(dynamic_cast<node::CmdNode *>(parser_tree),
                                    status);
        case node::kInsertStmt:
            return TransformInsertNode(
                dynamic_cast<node::InsertStmt *>(parser_tree), status);
        default: {
            status.msg = "can not support " +
                         node::NameOfSQLNodeType(parser_tree->GetType());
            status.code = error::kAnalyserErrorSQLTypeNotSupport;
            return;
        }
    }
}

void FeSQLAnalyser::TransformMultiTableSelectNode(
    node::SelectStmt *parser_tree,
    Status &status) {  // NOLINT (runtime/references)
    status.code = error::kAnalyserErrorQueryMultiTable;
    status.msg = "can not support select query on multi tables";
}

void FeSQLAnalyser::TransformSelectNode(
    node::SelectStmt *parser_tree,
    Status &status) {  // NOLINT (runtime/references)
    if (parser_tree->GetTableRefList().empty()) {
        status.msg =
            "can not transform select node when table references (from "
            "table list) is empty";
        status.code = error::kAnalyserErrorFromListEmpty;
        return;
    }

    if (parser_tree->GetTableRefList().size() == 1) {
        TransformSingleTableSelectNode(parser_tree, status);
    } else {
        TransformMultiTableSelectNode(parser_tree, status);
    }
}
void FeSQLAnalyser::TransformSingleTableSelectNode(
    node::SelectStmt *parser_tree,
    Status &status) {  // NOLINT (runtime/references)
    node::TableNode *table_ref =
        (node::TableNode *)(parser_tree->GetTableRefList().at(0));

    if (nullptr == table_ref) {
        status.msg =
            "can not transform select node when table reference is null";
        status.code = error::kAnalyserErrorTableRefIsNull;
        return;
    }

    if (false == IsTableExist(table_ref->GetOrgTableName())) {
        status.msg = "can not query select when table " +
                     table_ref->GetOrgTableName() + " is not exist in db";
        status.code = error::kAnalyserErrorTableNotExist;
        return;
    }

    for (auto node : parser_tree->GetSelectList()) {
        if (node::kResTarget != node->GetType()) {
            status.msg = "Fail to handle select list node type " +
                         node::NameOfSQLNodeType(node->GetType());
            status.code = error::kAnalyserErrorUnSupport;
            LOG(WARNING) << status.msg;
        }
        node::ResTarget *target = (node::ResTarget *)node;

        if (nullptr == target->GetVal()) {
            status.msg = "Fail to handle select list node null";
            status.code = error::kAnalyserErrorTargetIsNull;
            LOG(WARNING) << status.msg;
        }
        switch (target->GetVal()->GetExprType()) {
            case node::kExprColumnRef: {
                TransformColumnRef((node::ColumnRefNode *)target->GetVal(),
                                   table_ref->GetOrgTableName(), status);
                break;
            }
            case node::kExprFunc: {
                TransformFuncNode((node::FuncNode *)target->GetVal(),
                                  table_ref->GetOrgTableName(), status);
                break;
            }
            case node::kExprAll: {
                TransformAllRef((node::AllNode *)target->GetVal(),
                                table_ref->GetOrgTableName(), status);
                break;
            }
            default: {
                status.msg =
                    "SELECT error: can not handle " +
                    node::NameOfSQLNodeType(target->GetVal()->GetType());
                status.code = error::kAnalyserErrorSQLTypeNotSupport;
                LOG(WARNING) << status.msg;
            }
        }
        if (0 != status.code) {
            return;
        }
    }
}

bool FeSQLAnalyser::IsTableExist(std::string basic_string) {
    return table_map_.find(basic_string) != table_map_.end();
}

bool FeSQLAnalyser::IsColumnExistInTable(const std::string &column_name,
                                         const std::string &table_name) {
    if (table_map_.find(table_name) == table_map_.end()) {
        return false;
    }

    auto map = table_map_[table_name];
    return map.find(column_name) != map.end();
}

void FeSQLAnalyser::TransformFuncNode(
    node::FuncNode *node_ptr, const std::string &table_name,
    Status &status) {  // NOLINT (runtime/references)
    // TODO(chenjing): 细化参数校验
    // TODO(chenjing): 表达式节点修改：需要带上DataType属性
    for (int i = 0; i < static_cast<int>(node_ptr->GetArgs().size()); ++i) {
        TransformExprNode(node_ptr->GetArgs()[i], table_name, status);
        if (0 != status.code) {
            return;
        }
    }
    // TODO(chenjing): add function signature validate
    FuncDefType func_type = GetAggFunDefType(node_ptr);
    switch (func_type) {
        case kFuncTypeUnknow:
            status.msg =
                "function '" + node_ptr->GetFunctionName() + "' is undefined";
            status.code = error::kAnalyserErrorGlobalAggFunction;
            break;
        case kFuncTypeAgg:
            node_ptr->SetAgg(true);
            break;
        case kFuncTypeScalar:
            node_ptr->SetAgg(false);
            break;
        default: {
            status.msg =
                "FUNCTION error: can not hanlde " + std::to_string(func_type);
            status.code = error::kAnalyserErrorUnSupportFunction;
        }
    }

    if (0 != status.code) {
        return;
    }

    if (nullptr == node_ptr->GetOver() && node_ptr->GetIsAgg()) {
        status.msg = "can not apply agg function without 'over' window";
        status.code = error::kAnalyserErrorGlobalAggFunction;
        return;
    }

    if (nullptr != node_ptr->GetOver()) {
        TransformWindowDef(node_ptr->GetOver(), table_name, status);
    }
}
void FeSQLAnalyser::TransformColumnRef(
    node::ColumnRefNode *node_ptr, const std::string &table_name,
    Status &status) {  // NOLINT (runtime/references)
    if (node_ptr->GetColumnName().empty()) {
        status.msg = "can not query select when column is empty";
        status.code = error::kAnalyserErrorColumnNameIsEmpty;
        return;
    }

    if (node_ptr->GetRelationName().empty()) {
        node_ptr->SetRelationName(table_name);
    }
    if (false == IsColumnExistInTable(node_ptr->GetColumnName(),
                                      node_ptr->GetRelationName())) {
        status.msg = "can not query select when column " +
                     node_ptr->GetColumnName() + " is not exit in table " +
                     node_ptr->GetRelationName();
        status.code = error::kAnalyserErrorColumnNotExist;
        return;
    }
}

void FeSQLAnalyser::Initialize() {
    func_defs["SUBSTR"] = kFuncTypeScalar;
    func_defs["TRIM"] = kFuncTypeScalar;
    func_defs["COUNT"] = kFuncTypeAgg;
    func_defs["SUM"] = kFuncTypeAgg;
    func_defs["AVG"] = kFuncTypeAgg;
    func_defs["MIN"] = kFuncTypeAgg;
    func_defs["MAX"] = kFuncTypeAgg;

    if (tables_.empty()) {
        return;
    }
    table_map_.clear();
    for (auto table : tables_) {
        if (table_map_.find(table->name()) != table_map_.end()) {
            table_map_.clear();
            LOG(WARNING)
                << "error occur when initialize tables: table duplicate in db";
            return;
        }

        std::map<std::string, const ColumnDef *> column_map;
        for (int i = 0; i < table->columns().size(); ++i) {
            column_map[table->columns(i).name()] = &(table->columns(i));
        }
        table_map_[table->name()] = column_map;
    }
}
void FeSQLAnalyser::TransformAllRef(
    node::AllNode *node_ptr, const std::string &relation_name,
    Status &status) {  // NOLINT (runtime/references)
    if (node_ptr->GetRelationName().empty()) {
        node_ptr->SetRelationName(relation_name);
        return;
    }

    if (node_ptr->GetRelationName() == relation_name) {
        return;
    }

    status.msg = "can not query " + node_ptr->GetRelationName() +
                 ".* from table " + relation_name;
    status.code = error::kAnalyserErrorTableNotExist;
}

FuncDefType FeSQLAnalyser::GetAggFunDefType(node::FuncNode *node_ptr) {
    if (func_defs.find(node_ptr->GetFunctionName()) == func_defs.end()) {
        return kFuncTypeUnknow;
    }
    return func_defs[node_ptr->GetFunctionName()];
}

void FeSQLAnalyser::TransformWindowDef(
    node::WindowDefNode *node_ptr, const std::string &table_name,
    Status &status) {  // NOLINT (runtime/references)
    // TODO(chenjing): window is exist
    // TODO(chenjing): window is redefined
    for (auto partition : node_ptr->GetPartitions()) {
        TransformPartition(partition, table_name, status);
        if (0 != status.code) {
            return;
        }
    }

    for (auto order : node_ptr->GetOrders()) {
        TransformOrder(order, table_name, status);
        if (0 != status.code) {
            return;
        }
    }
}

void FeSQLAnalyser::TransformOrder(
    SQLNode *node_ptr, const std::string &table_name,
    Status &status) {  // NOLINT (runtime/references)
}

void FeSQLAnalyser::TransformPartition(
    SQLNode *node_ptr, const std::string &table_name,
    Status &status) {  // NOLINT (runtime/references)
}

void FeSQLAnalyser::TransformExprNode(
    SQLNode *node_ptr, const std::string &table_name,
    Status &status) {  // NOLINT (runtime/references)

    if (node_ptr == nullptr) {
        status.msg = "Fail to transform null node";
        status.code = error::kExecuteErrorNullNode;
        LOG(WARNING) << status.msg;
        return;
    }
    node::ExprNode *expr = dynamic_cast<node::ExprNode *>(node_ptr);
    switch (expr->GetExprType()) {
        case node::kExprColumnRef:
            return TransformColumnRef((node::ColumnRefNode *)node_ptr,
                                      table_name, status);
        case node::kExprFunc:
            return TransformFuncNode((node::FuncNode *)node_ptr, table_name,
                                     status);
        case node::kExprPrimary:
        default: {
            status.code = error::kAnalyserErrorSQLTypeNotSupport;
            status.msg = "can not support " +
                         node::NameOfSQLNodeType(node_ptr->GetType()) +
                         " in expr";
            return;
        }
    }
}

void FeSQLAnalyser::TransformCreateNode(
    node::CreateStmt *parser_node_ptr,
    Status &status) {  // NOLINT (runtime/references)
    if (!parser_node_ptr->GetOpIfNotExist() &&
        IsTableExist(parser_node_ptr->GetTableName())) {
        status.msg = "CREATE TABLE " + parser_node_ptr->GetTableName() +
                     "ALREADY EXISTS";
        status.code = error::kAnalyserErrorTableAlreadyExist;
        return;
    }
}
void FeSQLAnalyser::TransformInsertNode(
    node::InsertStmt *node_ptr, Status &status  // NOLINT (runtime/references)
) {
    // nothing to do
}
void FeSQLAnalyser::TransformCmdNode(
    node::CmdNode *node_ptr, Status &status) {  // NOLINT (runtime/references)
    // no nothing
}

}  // namespace analyser
}  // namespace fesql