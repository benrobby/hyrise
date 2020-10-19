#pragma once

#include "abstract_rule.hpp"

namespace opossum {

class AbstractLQPNode;
class PredicateNode;

/**
 * The SemiJoinREDUCTIONRule adds semi joins to reduce the cardinality of expensive operations early. These semi join
 * reductions should be moved down the plan by the PredicatePlacementRule. If this does not work, the reductions are
 * pointless. The purpose of THIS rule, the SemiJoinREMOVALRule is to remove those semi join reductions that do not
 * sufficiently reduce the cardinality.
 *
 * Example:
 * Original plan:
 * [ part p1 ] -> [ Predicate p_container IN (...) ------------> [ Semi Join p1.p_container = p2.p_container
 *                                                              /             AND p1.p_size > AVG(p2.p_size) ]
 * [ part p2 ] -> [ Validate (selectivity 95%) ] ---------------
 *
 * Plan with semi join reduction:
 * [ part p1 ] -> [ Predicate p_container IN (...) ] -----------------------------------------------------------------> ...  // NOLINT
 *                                                   \                                                               /
 * [ part p2 ] --------------------------------------> [ Semi Join p1.p_container = p2.p_container ] -> [ Validate ]
 *
 * As the selectivity of the Validate node is below MAXIMUM_SELECTIVITY, the semi join reduction is considered useless.
 */

class SemiJoinRemovalRule : public AbstractRule {
 public:
  void apply_to(const std::shared_ptr<AbstractLQPNode>& root) const override;

  constexpr static auto MAXIMUM_SELECTIVITY = .25;
};

}  // namespace opossum
