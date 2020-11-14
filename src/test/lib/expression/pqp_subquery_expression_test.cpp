#include <regex>

#include "base_test.hpp"

#include "expression/expression_functional.hpp"
#include "expression/expression_utils.hpp"
#include "expression/pqp_column_expression.hpp"
#include "expression/pqp_subquery_expression.hpp"
#include "hyrise.hpp"
#include "logical_query_plan/dummy_table_node.hpp"
#include "operators/get_table.hpp"
#include "operators/limit.hpp"
#include "operators/projection.hpp"
#include "operators/table_scan.hpp"
#include "utils/load_table.hpp"

using namespace std::string_literals;            // NOLINT
using namespace opossum::expression_functional;  // NOLINT

namespace opossum {

class PQPSubqueryExpressionTest : public BaseTest {
 public:
  void SetUp() override {
    table_a = load_table("resources/test_data/tbl/int_float.tbl");
    Hyrise::get().storage_manager.add_table("int_float", table_a);
    a_a = PQPColumnExpression::from_table(*table_a, "a");
    a_b = PQPColumnExpression::from_table(*table_a, "b");

    // Build a Subquery returning a SINGLE NON-NULLABLE VALUE and taking ONE PARAMETER
    const auto parameter_a = placeholder_(ParameterID{2});
    const auto get_table_a = std::make_shared<GetTable>("int_float");
    const auto projection_a = std::make_shared<Projection>(get_table_a, expression_vector(add_(a_a, parameter_a)));
    const auto limit_a = std::make_shared<Limit>(projection_a, value_(1));
    pqp_single_value_one_parameter = limit_a;
    parameters_a = {std::make_pair(ParameterID{2}, ColumnID{3})};
    subquery_single_value_one_parameter =
        std::make_shared<PQPSubqueryExpression>(pqp_single_value_one_parameter, DataType::Int, false, parameters_a);

    // Build a Subquery returning a TABLE and taking NO PARAMETERS
    const auto get_table_b = std::make_shared<GetTable>("int_float");
    const auto table_scan_b = std::make_shared<TableScan>(get_table_b, greater_than_(a_a, 5));
    pqp_table = table_scan_b;
    subquery_table = std::make_shared<PQPSubqueryExpression>(pqp_table);
  }

  std::shared_ptr<Table> table_a;
  std::shared_ptr<PQPColumnExpression> a_a, a_b;
  PQPSubqueryExpression::Parameters parameters_a;
  std::shared_ptr<PQPSubqueryExpression> subquery_single_value_one_parameter;
  std::shared_ptr<PQPSubqueryExpression> subquery_table;
  std::shared_ptr<AbstractOperator> pqp_single_value_one_parameter;
  std::shared_ptr<AbstractOperator> pqp_table;
};

TEST_F(PQPSubqueryExpressionTest, DeepEquals) {
  EXPECT_EQ(*subquery_single_value_one_parameter, *subquery_single_value_one_parameter);

  // different parameters:
  const auto parameters_b = PQPSubqueryExpression::Parameters{std::make_pair(ParameterID{2}, ColumnID{2})};
  const auto subquery_different_parameter =
      std::make_shared<PQPSubqueryExpression>(pqp_single_value_one_parameter, DataType::Int, false, parameters_b);
  EXPECT_NE(*subquery_table, *subquery_different_parameter);

  // different PQP:
  const auto pqp_without_limit = pqp_single_value_one_parameter->mutable_left_input();
  const auto subquery_different_lqp =
      std::make_shared<PQPSubqueryExpression>(pqp_without_limit, DataType::Int, false, parameters_a);
  EXPECT_NE(*subquery_table, *subquery_different_lqp);
}

TEST_F(PQPSubqueryExpressionTest, DeepCopy) {
  const auto subquery_single_value_one_parameter_copy =
      std::dynamic_pointer_cast<PQPSubqueryExpression>(subquery_single_value_one_parameter->deep_copy());
  ASSERT_TRUE(subquery_single_value_one_parameter_copy);

  ASSERT_EQ(subquery_single_value_one_parameter_copy->parameters.size(), 1u);
  EXPECT_EQ(subquery_single_value_one_parameter_copy->parameters[0].first, ParameterID{2});
  EXPECT_EQ(subquery_single_value_one_parameter_copy->parameters[0].second, ColumnID{3});
  EXPECT_NE(subquery_single_value_one_parameter_copy->pqp, subquery_single_value_one_parameter->pqp);
  EXPECT_EQ(subquery_single_value_one_parameter_copy->pqp->type(), OperatorType::Limit);

  const auto subquery_table_copy = std::dynamic_pointer_cast<PQPSubqueryExpression>(subquery_table->deep_copy());
  ASSERT_TRUE(subquery_table_copy);

  ASSERT_EQ(subquery_table_copy->parameters.size(), 0u);
  EXPECT_NE(subquery_table_copy->pqp, subquery_single_value_one_parameter->pqp);
  EXPECT_EQ(subquery_table_copy->pqp->type(), OperatorType::TableScan);
}

TEST_F(PQPSubqueryExpressionTest, DeepCopyDiamondShape) {
  auto scan_a = create_table_scan(_table_wrapper_a, ColumnID{0}, PredicateCondition::GreaterThanEquals, 1234);
  scan_a->execute();

  auto scan_b = create_table_scan(scan_a, ColumnID{1}, PredicateCondition::LessThan, 1000);
  auto scan_c = create_table_scan(scan_a, ColumnID{1}, PredicateCondition::GreaterThan, 2000);
  auto union_positions = std::make_shared<UnionPositions>(scan_b, scan_c);

  auto copied_pqp = union_positions->deep_copy();

  EXPECT_EQ(copied_pqp->left_input()->left_input(), copied_pqp->right_input()->left_input());
}

TEST_F(PQPSubqueryExpressionTest, RequiresCalculation) {
  EXPECT_TRUE(subquery_single_value_one_parameter->requires_computation());
  EXPECT_TRUE(subquery_table->requires_computation());
}

TEST_F(PQPSubqueryExpressionTest, DataType) {
  // Subquerie returning tables don't have a data type
  EXPECT_ANY_THROW(subquery_table->data_type());
  EXPECT_EQ(subquery_single_value_one_parameter->data_type(), DataType::Int);
  const auto subquery_float =
      std::make_shared<PQPSubqueryExpression>(pqp_single_value_one_parameter, DataType::Float, true, parameters_a);
  EXPECT_EQ(subquery_float->data_type(), DataType::Float);
}

TEST_F(PQPSubqueryExpressionTest, IsNullable) {
  // Cannot query nullability of PQP expressions
  EXPECT_ANY_THROW(subquery_table->is_nullable_on_lqp(*DummyTableNode::make()));
}

TEST_F(PQPSubqueryExpressionTest, AsColumnName) {
  EXPECT_TRUE(std::regex_search(subquery_table->as_column_name(), std::regex{"SUBQUERY \\(PQP, 0x[0-9a-f]+\\)"}));
  EXPECT_TRUE(std::regex_search(subquery_single_value_one_parameter->as_column_name(),
                                std::regex{"SUBQUERY \\(PQP, 0x[0-9a-f]+\\)"}));
}

}  // namespace opossum
