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

#include "bm/runner_bm_case.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "gtest/gtest.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/TargetSelect.h"

namespace fesql {
namespace bm {

using vm::Engine;
using vm::RequestRunSession;
using vm::RunnerContext;
using sqlcase::CaseDataMock;
using namespace ::llvm;  // NOLINT
static void RequestUnionRunnerCase(const std::string& sql, int runner_id,
                                   benchmark::State* state, MODE mode,
                                   int64_t limit_cnt, int64_t size) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    // prepare data into table
    auto catalog = vm::BuildOnePkTableStorage(size);
    Engine engine(catalog);
    RequestRunSession session;
    base::Status status;
    ASSERT_TRUE(engine.Get(sql, "db", session, status));
    auto table = catalog->GetTable("db", "t1");
    if (!table) {
        LOG(WARNING) << "table not exist";
        return;
    }

    std::ostringstream plan_oss;
    session.GetPhysicalPlan()->Print(plan_oss, "");
    LOG(INFO) << "physical plan:\n" << plan_oss.str() << std::endl;
    std::ostringstream runner_oss;
    session.GetClusterJob().Print(runner_oss, "");
    LOG(INFO) << "runner plan:\n" << runner_oss.str() << std::endl;
    std::unique_ptr<codec::RowView> row_view = std::unique_ptr<codec::RowView>(
        new codec::RowView(session.GetSchema()));

    auto start_runner = GetRunner(session.GetMainTask(), runner_id);
    ASSERT_TRUE(nullptr != start_runner);
    switch (mode) {
        case BENCHMARK: {
            for (auto _ : *state) {
                std::vector<std::shared_ptr<DataHandler>> res;
                benchmark::DoNotOptimize(
                    RunnerRun(&session, start_runner, table, limit_cnt, res));
            }
        }
        case TEST: {
            std::vector<std::shared_ptr<DataHandler>> res;
            ASSERT_TRUE(
                RunnerRun(&session, start_runner, table, limit_cnt, res));
            ASSERT_EQ(res.size(), static_cast<size_t>(limit_cnt));
            for (auto data : res) {
                //                ASSERT_EQ(static_cast<int64_t >(1 + size),
                //                data->GetCount());
                LOG(INFO) << "res size : " << data->GetCount();
            }
        }
    }
}

void AggRunnerCase(const std::string sql, int runner_id,
                   benchmark::State* state, MODE mode, int64_t limit_cnt,
                   int64_t size) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    // prepare data into table
    auto catalog = vm::BuildOnePkTableStorage(size);
    Engine engine(catalog);
    RequestRunSession session;
    base::Status status;
    ASSERT_TRUE(engine.Get(sql, "db", session, status));
    auto table = catalog->GetTable("db", "t1");
    if (!table) {
        LOG(WARNING) << "table not exist";
        return;
    }

    std::ostringstream plan_oss;
    session.GetPhysicalPlan()->Print(plan_oss, "");
    LOG(INFO) << "physical plan:\n" << plan_oss.str() << std::endl;
    std::ostringstream runner_oss;
    session.GetClusterJob().Print(runner_oss, "");
    LOG(INFO) << "runner plan:\n" << runner_oss.str() << std::endl;
    std::unique_ptr<codec::RowView> row_view = std::unique_ptr<codec::RowView>(
        new codec::RowView(session.GetSchema()));

    auto start_runner = GetRunner(session.GetMainTask(), runner_id);
    ASSERT_TRUE(nullptr != start_runner);
    switch (mode) {
        case BENCHMARK: {
            for (auto _ : *state) {
                std::vector<std::shared_ptr<DataHandler>> res;
                benchmark::DoNotOptimize(
                    RunnerRun(&session, start_runner, table, limit_cnt, res));
            }
        }
        case TEST: {
            std::vector<std::shared_ptr<DataHandler>> res;
            ASSERT_TRUE(
                RunnerRun(&session, start_runner, table, limit_cnt, res));
        }
    }
}

static bool RunnerRun(
    RunSession* session, Runner* runner,
    std::shared_ptr<TableHandler> table_handler, int64_t limit_cnt,
    std::vector<std::shared_ptr<DataHandler>>& result) {  // NOLINT
    auto iter = table_handler->GetIterator();
    int64_t cnt = 0;
    while (cnt < limit_cnt && iter->Valid()) {
        cnt++;
        RunnerContext ctx(&session->GetClusterJob(), iter->GetValue());
        auto data = runner->RunWithCache(ctx);
        iter->Next();
        result.push_back(data);
    }
    return true;
}

static Runner* GetRunner(Runner* root, int id) {
    if (nullptr == root) {
        return nullptr;
    }

    if (id == root->id_) {
        return root;
    } else {
        for (auto runner : root->GetProducers()) {
            auto res = GetRunner(runner, id);
            if (nullptr != res) {
                return res;
            }
        }
        return nullptr;
    }
}

void WindowSumFeature1_RequestUnion(benchmark::State* state, MODE mode,
                                    int64_t limit_cnt, int64_t size) {
    // prepare data into table
    const std::string sql =
        "SELECT "
        "sum(col4) OVER w1 as w1_col4_sum "
        "FROM t1 WINDOW w1 AS (PARTITION BY col0 ORDER BY col5 "
        "ROWS_RANGE BETWEEN "
        "30d "
        "PRECEDING AND CURRENT ROW) limit " +
        std::to_string(limit_cnt) + ";";
    RequestUnionRunnerCase(sql, 2, state, mode, limit_cnt, size);
}

void WindowSumFeature1_Aggregation(benchmark::State* state, MODE mode,
                                   int64_t limit_cnt, int64_t size) {
    // prepare data into table
    const std::string sql =
        "SELECT "
        "sum(col4) OVER w1 as w1_col4_sum "
        "FROM t1 WINDOW w1 AS (PARTITION BY col0 ORDER BY col5 "
        "ROWS_RANGE BETWEEN "
        "30d "
        "PRECEDING AND CURRENT ROW) limit " +
        std::to_string(limit_cnt) + ";";
    AggRunnerCase(sql, 3, state, mode, limit_cnt, size);
}

}  // namespace bm
}  // namespace fesql