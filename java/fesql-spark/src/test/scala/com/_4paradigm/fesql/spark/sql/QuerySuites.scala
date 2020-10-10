package com._4paradigm.fesql.spark.sql

class QuerySuites extends SQLBaseSuite {

  testCases("cases/query/simple_query.yaml")
  testCases("cases/query/window_query.yaml")
  testCases("cases/query/last_join_query.yaml")
  testCases("cases/query/last_join_window_query.yaml")
  testCases("cases/query/window_with_union_query.yaml")
  testCases("cases/integration/v1/test_sub_select.yaml")
  testCases("cases/integration/v1/test_select_sample.yaml")
  testCases("cases/integration/v1/test_expression.yaml")
  testCases("cases/integration/v1/test_udaf_function.yaml")
  testCases("cases/integration/v1/test_window_row.yaml")
  testCases("cases/integration/v1/test_window_row_range.yaml")
  testCases("cases/integration/v1/test_window_union.yaml")
  testCases("cases/integration/v1/test_last_join.yaml")
  testCases("cases/query/group_query.yaml")

  // TODO: fix at(0)
  // testCases("cases/query/udaf_query.yaml")

}