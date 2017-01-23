/**
 *    Copyright (C) 2013 10gen Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#include "mongo/db/query/index_tag.h"

#include "mongo/db/matcher/expression_array.h"
#include "mongo/db/matcher/expression_tree.h"
#include "mongo/db/query/indexability.h"

#include <algorithm>
#include <limits>

namespace mongo {

namespace {

// Attaches 'node' to 'target'. If 'target' is an AND, adds 'node' as a child of 'target'. Otherwise, creates an AND that is a child of 'targetParent' at position 'targetPosition', and adds 'target' and 'node' as its children. Tags 'node' with 'tagData'.
void attachNode(MatchExpression* node,
                MatchExpression* target,
                OrMatchExpression* targetParent,
                size_t targetPosition,
                std::unique_ptr<MatchExpression::TagData> tagData) {
    auto clone = node->shallowClone();
    if (clone->matchType() == MatchExpression::NOT) {
        IndexTag* indexTag = static_cast<IndexTag*>(tagData.get());
        clone->setTag(new IndexTag(indexTag->index));
        clone->getChild(0)->setTag(tagData.release());
    } else {
        clone->setTag(tagData.release());
    }

    if (MatchExpression::AND == target->matchType()) {
        AndMatchExpression* andNode = static_cast<AndMatchExpression*>(target);
        andNode->add(clone.release());
    } else {
        std::unique_ptr<AndMatchExpression> andNode = stdx::make_unique<AndMatchExpression>();
        IndexTag* indexTag = static_cast<IndexTag*>(clone->getTag());
        andNode->setTag(new IndexTag(indexTag->index));
        andNode->add(target);
        andNode->add(clone.release());
        targetParent->getChildVector()->operator[](targetPosition) = andNode.release();
    }
}

// Finds all tags in 'moveNodeTags' whose path starts with 'position', remove them from 'moveNodeTags', and add them to the output vector with the starting 'position' removed.
std::vector<MatchExpression::MoveNodeTag> getChildMoveNodeTags(
    std::vector<MatchExpression::MoveNodeTag>& moveNodeTags, size_t position) {
    std::vector<MatchExpression::MoveNodeTag> childTags;
    auto iter = moveNodeTags.begin();
    while (iter != moveNodeTags.end()) {
        invariant(!(*iter).path.empty());
        if ((*iter).path[0] == position) {
            MatchExpression::MoveNodeTag tag = std::move(*iter);
            iter = moveNodeTags.erase(iter);
            tag.path.erase(tag.path.begin());
            childTags.push_back(std::move(tag));
        } else {
            ++iter;
        }
    }
    return childTags;
}

// Finds the child of 'tree' that is an indexed OR, if one exists.
MatchExpression* getIndexedOrChild(MatchExpression* tree) {
    auto children = tree->getChildVector();
    if (!children) {
        return nullptr;
    }
    for (auto child : *children) {
        if (MatchExpression::OR == child->matchType() && child->getTag() != nullptr) {
            return child;
        }
    }
    return nullptr;
}

// Moves 'node' along the paths in 'target' specified in 'moveNodeTags'. Each value in path is the index of a child in an indexed OR. Returns true if 'node' is moved to every indexed descendant of 'target'.
bool moveNode(MatchExpression* node,
              MatchExpression* target,
              std::vector<MatchExpression::MoveNodeTag> moveNodeTags) {
    if (MatchExpression::OR == target->matchType()) {
        OrMatchExpression* orNode = static_cast<OrMatchExpression*>(target);
        bool moveToAllChildren = true;
        for (size_t i = 0; i < orNode->numChildren(); ++i) {

            // Find all tags in 'moveNodeTags' whose path starts with 'i', remove them from 'moveNodeTags', and add them to 'childTags' with the starting 'i' removed.
            auto childTags = getChildMoveNodeTags(moveNodeTags, i);
            if (childTags.empty()) {

                // This child was not specified by any path in moveNodeTags.
                moveToAllChildren = false;
            } else if (childTags.size() == 1 && childTags[0].path.empty()) {

                // We have reached the empty path {}. Attach the node to this child.
                attachNode(node, orNode->getChild(i), orNode, i, std::move(childTags[0].tagData));
            } else if (orNode->getChild(i)->matchType() == MatchExpression::NOT &&
                       childTags.size() == 1 && childTags[0].path.size() == 1 &&
                       childTags[0].path[0] == 0) {

                // We have reached the path {0} and the child is a NOT. Attach the node to this child.
                attachNode(node, orNode->getChild(i), orNode, i, std::move(childTags[0].tagData));
            } else {

                // childTags contains non-trivial paths, so we recur.
                moveToAllChildren =
                    moveToAllChildren && moveNode(node, orNode->getChild(i), std::move(childTags));
            }
        }
        invariant(moveNodeTags.empty());
        return moveToAllChildren;
    }

    if (MatchExpression::AND == target->matchType()) {
        auto indexedOr = getIndexedOrChild(target);
        invariant(indexedOr);
        return moveNode(node, indexedOr, std::move(moveNodeTags));
    }

    invariant(0);
    return false;
}

// Populates 'out' with all descendants of 'node' that have MoveNodeTags, assuming the initial input is an ELEM_MATCH_OBJECT.
void getElemMatchMoveNodeDescendants(MatchExpression* node, std::vector<MatchExpression*>& out) {
    if (!node->getMoveNodeTags().empty()) {
        out.push_back(node);
    } else if (node->matchType() == MatchExpression::ELEM_MATCH_OBJECT ||
               node->matchType() == MatchExpression::AND) {
        for (size_t i = 0; i < node->numChildren(); ++i) {
            getElemMatchMoveNodeDescendants(node->getChild(i), out);
        }
    }
}

}  // namespace

// TODO: Move out of the enumerator and into the planner.

const size_t IndexTag::kNoIndex = std::numeric_limits<size_t>::max();

void tagForSort(MatchExpression* tree) {
    if (!Indexability::nodeCanUseIndexOnOwnField(tree)) {
        size_t myTagValue = IndexTag::kNoIndex;
        for (size_t i = 0; i < tree->numChildren(); ++i) {
            MatchExpression* child = tree->getChild(i);
            tagForSort(child);
            IndexTag* childTag = static_cast<IndexTag*>(child->getTag());
            if (NULL != childTag) {
                myTagValue = std::min(myTagValue, childTag->index);
            }
        }
        if (myTagValue != IndexTag::kNoIndex) {
            tree->setTag(new IndexTag(myTagValue));
        }
    }
}

bool TagComparison(const MatchExpression* lhs, const MatchExpression* rhs) {
    IndexTag* lhsTag = static_cast<IndexTag*>(lhs->getTag());
    size_t lhsValue = (NULL == lhsTag) ? IndexTag::kNoIndex : lhsTag->index;
    size_t lhsPos = (NULL == lhsTag) ? IndexTag::kNoIndex : lhsTag->pos;

    IndexTag* rhsTag = static_cast<IndexTag*>(rhs->getTag());
    size_t rhsValue = (NULL == rhsTag) ? IndexTag::kNoIndex : rhsTag->index;
    size_t rhsPos = (NULL == rhsTag) ? IndexTag::kNoIndex : rhsTag->pos;

    // First, order on indices.
    if (lhsValue != rhsValue) {
        // This relies on kNoIndex being larger than every other possible index.
        return lhsValue < rhsValue;
    }

    // Next, order so that if there's a GEO_NEAR it's first.
    if (MatchExpression::GEO_NEAR == lhs->matchType()) {
        return true;
    } else if (MatchExpression::GEO_NEAR == rhs->matchType()) {
        return false;
    }

    // Ditto text.
    if (MatchExpression::TEXT == lhs->matchType()) {
        return true;
    } else if (MatchExpression::TEXT == rhs->matchType()) {
        return false;
    }

    // Next, order so that the first field of a compound index appears first.
    if (lhsPos != rhsPos) {
        return lhsPos < rhsPos;
    }

    // Next, order on fields.
    int cmp = lhs->path().compare(rhs->path());
    if (0 != cmp) {
        return 0;
    }

    // Finally, order on expression type.
    return lhs->matchType() < rhs->matchType();
}

void sortUsingTags(MatchExpression* tree) {
    for (size_t i = 0; i < tree->numChildren(); ++i) {
        sortUsingTags(tree->getChild(i));
    }
    std::vector<MatchExpression*>* children = tree->getChildVector();
    if (NULL != children) {
        std::sort(children->begin(), children->end(), TagComparison);
    }
}

void resolveMoveNodeTags(MatchExpression* tree) {
    if (MatchExpression::AND != tree->matchType() && MatchExpression::OR != tree->matchType()) {
        return;
    }
    if (MatchExpression::AND == tree->matchType()) {
        AndMatchExpression* andNode = static_cast<AndMatchExpression*>(tree);
        MatchExpression* indexedOr = getIndexedOrChild(andNode);

        // Iterate through the children backward, since we may remove some.
        for (int64_t i = andNode->numChildren() - 1; i >= 0; --i) {
            auto child = andNode->getChild(i);
            if (!child->getMoveNodeTags().empty()) {
                auto moveNodeTags = child->releaseMoveNodeTags();
                if (moveNode(child, indexedOr, std::move(moveNodeTags))) {

                    // indexedOr can completely satisfy the predicate specified in child, so we can remove it.
                    auto ownedChild = andNode->removeChild(i);
                }
            } else if (child->matchType() == MatchExpression::NOT &&
                       !child->getChild(0)->getMoveNodeTags().empty()) {
                auto moveNodeTags = child->getChild(0)->releaseMoveNodeTags();

                // Move the NOT and its child.
                if (moveNode(child, indexedOr, std::move(moveNodeTags))) {

                    // indexedOr can completely satisfy the predicate specified in child, so we can remove it.
                    auto ownedChild = andNode->removeChild(i);
                }
            } else if (child->matchType() == MatchExpression::ELEM_MATCH_OBJECT) {

                // Move all descendants of child with MoveNodeTags.
                std::vector<MatchExpression*> moveNodeDescendants;
                getElemMatchMoveNodeDescendants(child, moveNodeDescendants);
                for (auto descendant : moveNodeDescendants) {
                    auto moveNodeTags = descendant->releaseMoveNodeTags();
                    moveNode(descendant, indexedOr, std::move(moveNodeTags));

                    // We cannot remove descendants of an $elemMatch object, since the filter must be applied in its entirety.
                }
            }
        }
    }
    for (size_t i = 0; i < tree->numChildren(); ++i) {
        resolveMoveNodeTags(tree->getChild(i));
    }
}

}  // namespace mongo
