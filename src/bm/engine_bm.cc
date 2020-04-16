/*
 * engine_mk.cc
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

#include "benchmark/benchmark.h"
#include "bm/engine_bm_case.h"
#include "llvm/Transforms/Scalar.h"

namespace fesql {
namespace bm {
using namespace ::llvm;  // NOLINT

static void BM_EngineRequestSimpleSelectVarchar(
    benchmark::State& state) {  // NOLINT
    EngineRequestSimpleSelectVarchar(&state, BENCHMARK);
}
static void BM_EngineRequestSimpleSelectDouble(
    benchmark::State& state) {  // NOLINT

    EngineRequestSimpleSelectDouble(&state, BENCHMARK);
}
static void BM_EngineRequestSimpleSelectInt32(
    benchmark::State& state) {  // NOLINT
    EngineRequestSimpleSelectInt32(&state, BENCHMARK);
}
static void BM_EngineRequestSimpleUDF(benchmark::State& state) {  // NOLINT
    EngineRequestSimpleUDF(&state, BENCHMARK);
}

static void BM_EngineWindowSumFeature1(benchmark::State& state) {  // NOLINT
    EngineWindowSumFeature1(&state, BENCHMARK, state.range(0), state.range(1));
}

static void BM_EngineWindowSumFeature5(benchmark::State& state) {  // NOLINT
    EngineWindowSumFeature5(&state, BENCHMARK, state.range(0), state.range(1));
}

static void BM_EngineSimpleSelectVarchar(benchmark::State& state) {  // NOLINT
    EngineSimpleSelectVarchar(&state, BENCHMARK);
}
static void BM_EngineSimpleSelectDouble(benchmark::State& state) {  // NOLINT
    EngineSimpleSelectDouble(&state, BENCHMARK);
}
static void BM_EngineSimpleSelectInt32(benchmark::State& state) {  // NOLINT
    EngineSimpleSelectInt32(&state, BENCHMARK);
}
static void BM_EngineSimpleUDF(benchmark::State& state) {  // NOLINT
    EngineSimpleUDF(&state, BENCHMARK);
}

static void BM_EngineRunBatchWindowSumFeature1(
    benchmark::State& state) {  // NOLINT
    EngineRunBatchWindowSumFeature1(&state, BENCHMARK, state.range(0),
                                    state.range(1));
}
static void BM_EngineRunBatchWindowSumFeature5(
    benchmark::State& state) {  // NOLINT
    EngineRunBatchWindowSumFeature5(&state, BENCHMARK, state.range(0),
                                    state.range(1));
}
// request engine simple bm
BENCHMARK(BM_EngineRequestSimpleSelectVarchar);
BENCHMARK(BM_EngineRequestSimpleSelectDouble);
BENCHMARK(BM_EngineRequestSimpleSelectInt32);
BENCHMARK(BM_EngineRequestSimpleUDF);

// batch engine simple bm
BENCHMARK(BM_EngineSimpleSelectVarchar);
BENCHMARK(BM_EngineSimpleSelectDouble);
BENCHMARK(BM_EngineSimpleSelectInt32);
BENCHMARK(BM_EngineSimpleUDF);

// request engine window bm
BENCHMARK(BM_EngineWindowSumFeature1)
    ->Args({1, 2})
    ->Args({1, 10})
    ->Args({1, 100})
    ->Args({1, 1000})
    ->Args({1, 10000})
    ->Args({100, 100})
    ->Args({1000, 1000})
    ->Args({10000, 10000});
BENCHMARK(BM_EngineWindowSumFeature5)
    ->Args({1, 2})
    ->Args({1, 2})
    ->Args({1, 10})
    ->Args({1, 100})
    ->Args({1, 1000})
    ->Args({1, 10000})
    ->Args({100, 100})
    ->Args({1000, 1000})
    ->Args({10000, 10000});

// batch engine window bm
BENCHMARK(BM_EngineRunBatchWindowSumFeature1)
    ->Args({1, 2})
    ->Args({1, 2})
    ->Args({1, 10})
    ->Args({1, 100})
    ->Args({1, 1000})
    ->Args({1, 10000})
    ->Args({100, 100})
    ->Args({1000, 1000})
    ->Args({10000, 10000});
BENCHMARK(BM_EngineRunBatchWindowSumFeature5)
    ->Args({1, 2})
    ->Args({1, 2})
    ->Args({1, 10})
    ->Args({1, 100})
    ->Args({1, 1000})
    ->Args({1, 10000})
    ->Args({100, 100})
    ->Args({1000, 1000})
    ->Args({10000, 10000});

}  // namespace bm
}  // namespace fesql

BENCHMARK_MAIN();