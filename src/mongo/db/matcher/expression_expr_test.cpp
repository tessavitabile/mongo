/**
 *    Copyright (C) 2017 MongoDB Inc.
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
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/db/matcher/expression.h"
#include "mongo/db/matcher/expression_expr.h"
#include "mongo/db/matcher/matcher.h"
#include "mongo/db/pipeline/expression_context_for_test.h"
#include "mongo/db/query/collation/collator_interface_mock.h"
#include "mongo/unittest/unittest.h"

namespace mongo {

namespace {

using unittest::assertGet;

TEST(ExprMatchExpression, ComparisonToConstantMatchesCorrectly) {
    boost::intrusive_ptr<ExpressionContextForTest> expCtx(new ExpressionContextForTest());
    auto match = BSON("a" << 5);
    auto notMatch = BSON("a" << 6);

    auto expression1 = BSON("$expr" << BSON("$eq" << BSON_ARRAY("$a" << 5)));
    Matcher matcher1(expression1,
                     expCtx,
                     ExtensionsCallbackNoop(),
                     MatchExpressionParser::kAllowAllSpecialFeatures);
    ASSERT_TRUE(matcher1.matches(match));
    ASSERT_FALSE(matcher1.matches(notMatch));

    auto varId = expCtx->variablesParseState.defineVariable("var");
    expCtx->variables.setValue(varId, Value(5));
    auto expression2 = BSON("$expr" << BSON("$eq" << BSON_ARRAY("$a"
                                                                << "$$var")));
    Matcher matcher2(expression2,
                     expCtx,
                     ExtensionsCallbackNoop(),
                     MatchExpressionParser::kAllowAllSpecialFeatures);
    ASSERT_TRUE(matcher2.matches(match));
    ASSERT_FALSE(matcher2.matches(notMatch));
}

TEST(ExprMatchExpression, ComparisonBetweenTwoFieldPathsMatchesCorrectly) {
    boost::intrusive_ptr<ExpressionContextForTest> expCtx(new ExpressionContextForTest());

    auto expression = BSON("$expr" << BSON("$gt" << BSON_ARRAY("$a"
                                                               << "$b")));
    auto match = BSON("a" << 10 << "b" << 2);
    auto notMatch = BSON("a" << 2 << "b" << 10);

    Matcher matcher(expression,
                    std::move(expCtx),
                    ExtensionsCallbackNoop(),
                    MatchExpressionParser::kAllowAllSpecialFeatures);

    ASSERT_TRUE(matcher.matches(match));
    ASSERT_FALSE(matcher.matches(notMatch));
}

TEST(ExprMatchExpression, ComparisonThrowsWithUnboundVariable) {
    boost::intrusive_ptr<ExpressionContextForTest> expCtx(new ExpressionContextForTest());
    auto expression = BSON("$expr" << BSON("$eq" << BSON_ARRAY("$a"
                                                               << "$$var")));
    ASSERT_THROWS(ExprMatchExpression pipelineExpr(expression.firstElement(), std::move(expCtx)),
                  DBException);
}

// TODO SERVER-30991: Uncomment once MatchExpression::optimize() is in place and handles
// optimization of the Expression held by ExprMatchExpression. Also add a second expression,
// BSON("$expr" << "$$var"), with $$var bound to 4 to confirm it optimizes to {$const: 4} as
// well.
/*
TEST(ExprMatchExpression, IdenticalPostOptimizedExpressionsAreEquivalent) {
    BSONObj expression = BSON("$expr" << BSON("$multiply" << BSON_ARRAY(2 << 2)));
    BSONObj expressionEquiv = BSON("$expr" << BSON("$const" << 4));
    BSONObj expressionNotEquiv = BSON("$expr" << BSON("$const" << 10));

    boost::intrusive_ptr<ExpressionContextForTest> expCtx(new ExpressionContextForTest());
    ExprMatchExpression pipelineExpr(expression.firstElement(), expCtx);
    pipelineExpr::optimize();
    ASSERT_TRUE(pipelineExpr.equivalent(&pipelineExpr));

    ExprMatchExpression pipelineExprEquiv(expressionEquiv.firstElement(), expCtx);
    ASSERT_TRUE(pipelineExpr.equivalent(&pipelineExprEquiv));

    ExprMatchExpression pipelineExprNotEquiv(expressionNotEquiv.firstElement(), expCtx);
    ASSERT_FALSE(pipelineExpr.equivalent(&pipelineExprNotEquiv));
}
*/

TEST(ExprMatchExpression, ShallowClonedExpressionIsEquivalentToOriginal) {
    BSONObj expression = BSON("$expr" << BSON("$eq" << BSON_ARRAY("$a" << 5)));

    boost::intrusive_ptr<ExpressionContextForTest> expCtx(new ExpressionContextForTest());
    ExprMatchExpression pipelineExpr(expression.firstElement(), std::move(expCtx));
    auto shallowClone = pipelineExpr.shallowClone();
    ASSERT_TRUE(pipelineExpr.equivalent(shallowClone.get()));
}

TEST(ExprMatchExpression, SetCollatorChangesCollationUsedForComparisons) {
    boost::intrusive_ptr<ExpressionContextForTest> expCtx(new ExpressionContextForTest());
    auto match = BSON("a"
                      << "abc");
    auto notMatch = BSON("a"
                         << "ABC");

    auto expression = BSON("$expr" << BSON("$eq" << BSON_ARRAY("$a"
                                                               << "abc")));
    auto matchExpression = assertGet(MatchExpressionParser::parse(expression, expCtx));
    ASSERT_TRUE(matchExpression->matchesBSON(match));
    ASSERT_FALSE(matchExpression->matchesBSON(notMatch));

    CollatorInterfaceMock collator(CollatorInterfaceMock::MockType::kAlwaysEqual);
    matchExpression->setCollator(&collator);

    ASSERT_TRUE(matchExpression->matchesBSON(match));
    ASSERT_TRUE(matchExpression->matchesBSON(notMatch));
}

}  // namespace
}  // namespace mongo
