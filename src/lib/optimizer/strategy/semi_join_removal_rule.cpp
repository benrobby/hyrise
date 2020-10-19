#include "semi_join_removal_rule.hpp"

#include "cost_estimation/abstract_cost_estimator.hpp"
#include "expression/binary_predicate_expression.hpp"
#include "expression/expression_utils.hpp"
#include "logical_query_plan/lqp_utils.hpp"
#include "statistics/abstract_cardinality_estimator.hpp"

namespace opossum {
void SemiJoinRemovalRule::apply_to(const std::shared_ptr<AbstractLQPNode>& root) const {
  // Approach: Find a semi reduction node and the corresponding original join node and check whether the cardinality is
  // reduced between the semi join reduction and the original join. If it is not, the semi join reduction is likely not
  // helpful and should be removed.

  Assert(root->type == LQPNodeType::Root, "ExpressionReductionRule needs root to hold onto");

  // You might be able to come up with a case where this caches too much (especially when looking at nested semi joins),
  // but the chance of this is slim enough to risk it and benefit from cached results.
  const auto estimator = cost_estimator->cardinality_estimator->new_instance();
  estimator->guarantee_bottom_up_construction();

  // In some cases, semi joins are added on both sides of the join (e.g., TPC-H Q17). In the LQPTranslator, these will
  // be translated into the same operator. If we remove one of these reductions, we block the reuse of the join result.
  // To counter these cases, we track semi join reductions that should not be removed. `removal_candidates` holds the
  // nodes that can be removed unless there is a semantically identical node in `removal_blockers`.
  // This is slightly ugly, as we have to preempt the behavior of the LQPTranslator. This would be better if we had a
  // method of identifying plan reuse in the optimizer. However, when we tried this, we found that implementing reuse
  // on the LQP-level was close to impossible in the presence of self-joins.
  auto removal_candidates = std::unordered_set<std::shared_ptr<AbstractLQPNode>>{};
  auto removal_blockers = LQPNodeUnorderedSet{};

  visit_lqp(root, [&](const auto& node) {
    // Check if the current node is a semi join (not necessarily a reduction)
    if (node->type != LQPNodeType::Join) return LQPVisitation::VisitInputs;
    const auto& join_node = static_cast<const JoinNode&>(*node);
    if (join_node.join_mode != JoinMode::Semi) return LQPVisitation::VisitInputs;
    // Semi join reductions are never multi-predicate joins
    if (join_node.join_predicates().size() > 1) return LQPVisitation::VisitInputs;
    const auto semi_join_predicate = join_node.join_predicates().at(0);

    // Find an upper node that corresponds to this node
    visit_lqp_upwards(node, [&](const auto& upper_node) {
      // visit_lqp_upwards includes the current node - skip it
      if (node == upper_node) return LQPUpwardVisitation::VisitOutputs;

      // The estimation for aggregate and column/column scans is bad, so whenever one of these occurs between the semi
      // join reduction and the original join, do not remove the semi join reduction (i.e., abort the search for an
      // upper node).
      if (upper_node->type == LQPNodeType::Aggregate) {
        removal_blockers.emplace(node);
        return LQPUpwardVisitation::DoNotVisitOutputs;
      } else if (upper_node->type == LQPNodeType::Predicate) {
        const auto& upper_predicate_node = static_cast<const PredicateNode&>(*upper_node);
        const auto binary_predicate =
            std::dynamic_pointer_cast<BinaryPredicateExpression>(upper_predicate_node.predicate());
        if (binary_predicate && binary_predicate->left_operand()->type == ExpressionType::LQPColumn &&
            binary_predicate->right_operand()->type == ExpressionType::LQPColumn) {
          removal_blockers.emplace(node);
          return LQPUpwardVisitation::DoNotVisitOutputs;
        }
      }

      if (upper_node->type != LQPNodeType::Join) return LQPUpwardVisitation::VisitOutputs;
      const auto& upper_join_node = static_cast<const JoinNode&>(*upper_node);

      // In most cases, the right side of the semi join reduction is one of the inputs of the original join. In rare
      // cases, this is not true (see try_deeper_reducer_node in semi_join_reduction_rule.cpp). Those cases are not
      // covered by this removal rule yet.
      const auto semi_join_is_left_input = upper_join_node.right_input() == join_node.right_input();
      const auto semi_join_is_right_input = upper_join_node.left_input() == join_node.right_input();
      if (!semi_join_is_left_input && !semi_join_is_right_input) {
        removal_blockers.emplace(node);
        return LQPUpwardVisitation::DoNotVisitOutputs;
      }
      auto input_side = semi_join_is_left_input ? LQPInputSide::Left : LQPInputSide::Right;

      // Check whether the semi join reduction predicate matches one of the predicates of the upper join
      if (std::none_of(upper_join_node.join_predicates().begin(), upper_join_node.join_predicates().end(),
                       [&](const auto predicate) { return *predicate == *semi_join_predicate; })) {
        return LQPUpwardVisitation::VisitOutputs;
      }

      // By design, we do not distinguish between semi join reduction nodes and "regular" semi join nodes. Still,
      // as a sanity check, we want to make sure that we do not remove non-reduction nodes.
      DebugAssert(node->comment == "Semi Reduction", "Expected found semi join to be marked as reduction");

      // Estimate the output cardinality of the semi join reduction and the input cardinality of the join
      const auto semi_join_output_cardinality = estimator->estimate_cardinality(node);
      const auto join_input_cardinality = estimator->estimate_cardinality(upper_node->input(input_side));

      // Remove the semi join reduction if it does not help in properly reducing the cardinality of the original join's
      // input
      if (join_input_cardinality / semi_join_output_cardinality > MAXIMUM_SELECTIVITY) {
        removal_candidates.emplace(node);
      } else {
        removal_blockers.emplace(node);
      }

      return LQPUpwardVisitation::DoNotVisitOutputs;
    });

    return LQPVisitation::VisitInputs;
  });

  for (const auto& removal_candidate : removal_candidates) {
    if (removal_blockers.contains(removal_candidate)) continue;
    lqp_remove_node(removal_candidate, AllowRightInput::Yes);
  }
}
}  // namespace opossum
