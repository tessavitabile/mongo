/**
 *    Copyright (C) 2016 MongoDB Inc.
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

// Unit tests for MatchExpression::serialize serialization.

#include "mongo/platform/basic.h"

#include "mongo/db/json.h"
#include "mongo/db/matcher/expression.h"
#include "mongo/db/matcher/expression_parser.h"
#include "mongo/db/matcher/extensions_callback_noop.h"
#include "mongo/db/matcher/matcher.h"
#include "mongo/unittest/unittest.h"

namespace mongo {
namespace {

using std::pair;
using std::string;
using std::unique_ptr;

BSONObj serialize(MatchExpression* match) {
    BSONObjBuilder bob;
    match->serialize(&bob);
    return bob.obj();
}

TEST(SerializeBasic, AndExpressionWithOneChildSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{$and: [{x: 0}]}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$and: [{x: {$eq: 0}}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 0}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, AndExpressionWithTwoChildrenSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{$and: [{x: 1}, {x: 2}]}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$and: [{x: {$eq: 1}}, {x: {$eq: 2}}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: [1, 2]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, AndExpressionWithTwoIdenticalChildrenSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{$and: [{x: 1}, {x: 1}]}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$and: [{x: {$eq: 1}}, {x: {$eq: 1}}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: -1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionOr) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{$or: [{x: 'A'}, {x: 'B'}]}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$or: [{x: {$eq: 'A'}}, {x: {$eq: 'B'}}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 'A'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 'a'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionElemMatchObjectSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$elemMatch: {a: {$gt: 0}, b: {$gt: 0}}}}"),
                     ExtensionsCallbackNoop(),
                     collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              fromjson("{x: {$elemMatch: {$and: [{a: {$gt: 0}}, {b: {$gt: 0}}]}}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: [{a: 1, b: -1}, {a: -1, b: 1}]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: [{a: 1, b: 1}, {a: 0, b: 0}]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionElemMatchObjectWithEmptyStringSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{'': {$elemMatch: {a: {$gt: 0}, b: {$gt: 0}}}}"),
                     ExtensionsCallbackNoop(),
                     collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              fromjson("{'': {$elemMatch: {$and: [{a: {$gt: 0}}, {b: {$gt: 0}}]}}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{'': [{a: 1, b: -1}, {a: -1, b: 1}]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{'': [{a: 1, b: 1}, {a: 0, b: 0}]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionElemMatchValueSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(
        fromjson("{x: {$elemMatch: {$lt: 1, $gt: -1}}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$elemMatch: {$lt: 1, $gt: -1}}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: [{a: 1, b: -1}, {a: -1, b: 1}]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: [{a: 1, b: 1}, {a: 0, b: 0}]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: [1, 0]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionElemMatchValueWithEmptyStringSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(
        fromjson("{x: {$elemMatch: {$lt: 1, $gt: -1}}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$elemMatch: {$lt: 1, $gt: -1}}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: [{a: 1, b: -1}, {a: -1, b: 1}]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: [{a: 1, b: 1}, {a: 0, b: 0}]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: [1, 0]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionSizeSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$size: 2}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$size: 2}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: [1, 2, 3]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: [1, 2]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionAllSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$all: [1, 2]}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$and: [{x: {$eq: 1}}, {x: {$eq: 2}}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: [1, 2, 3]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: [1, 3]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionAllWithEmptyArraySerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$all: []}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$all: []}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: [1, 2, 3]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionAllWithRegex) {
    CollatorInterface* collator = nullptr;
    Matcher original(
        fromjson("{x: {$all: [/a.b.c/, /.d.e./]}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$and: [{x: /a.b.c/}, {x: /.d.e./}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 'abcde'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 'adbec'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionEqSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$eq: {a: 1}}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$eq: {a: 1}}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: {a: 1}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: {a: [1, 2]}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: {a: 2}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionNeSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$ne: {a: 1}}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$nor: [{x: {$eq: {a: 1}}}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: {a: 1}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: {a: [1, 2]}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionLtSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$lt: 3}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$lt: 3}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 3}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 2.9}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionGtSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$gt: 3}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$gt: 3}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 3}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 3.1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionGteSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$gte: 3}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$gte: 3}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 3}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 2}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionLteSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$lte: 3}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$lte: 3}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 3}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 4}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionRegexWithObjSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$regex: 'a.b'}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$regex: 'a.b'}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 'abc'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 'acb'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionRegexWithValueSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: /a.b/i}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$regex: 'a.b', $options: 'i'}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 'abc'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 'acb'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionRegexWithValueAndOptionsSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: /a.b/}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$regex: 'a.b'}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 'abc'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 'acb'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionRegexWithEqObjSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$eq: {$regex: 'a.b'}}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$eq: {$regex: 'a.b'}}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 'abc'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 'acb'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: /a.b.c/}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionModSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$mod: [2, 1]}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$mod: [2, 1]}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 2}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionExistsTrueSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$exists: true}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$exists: true}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{a: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionExistsFalseSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$exists: false}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$nor: [{x: {$exists: true}}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{a: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionInSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$in: [1, 2, 3]}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$in: [1, 2, 3]}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 4}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: [1, 2]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionInWithEmptyArraySerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$in: []}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$in: []}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionInWithRegexSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$in: [/\\d+/, /\\w+/]}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$in: [/\\d+/, /\\w+/]}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: '1234'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 'abcd'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: '1a2b'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionNinSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$nin: [1, 2, 3]}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$nor: [{x: {$in: [1, 2, 3]}}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 4}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: [1, 2]}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionBitsAllSetSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$bitsAllSet: [1, 3]}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$bitsAllSet: [1, 3]}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 2}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 10}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionBitsAllClearSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$bitsAllClear: [1, 3]}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$bitsAllClear: [1, 3]}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 2}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionBitsAnySetSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$bitsAnySet: [1, 3]}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$bitsAnySet: [1, 3]}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 2}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 4}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionBitsAnyClearSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$bitsAnyClear: [1, 3]}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$bitsAnyClear: [1, 3]}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 2}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 1}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 10}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionNotSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$not: {$eq: 3}}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$nor: [{$and: [{x: {$eq: 3}}]}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 3}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 4}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionNotWithMultipleChildrenSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$not: {$lt: 1, $gt: 3}}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              fromjson("{$nor: [{$and: [{x: {$lt: 1}}, {x: {$gt: 3}}]}]}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 2}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 4}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionNotWithBitTestSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(
        fromjson("{x: {$not: {$bitsAnySet: [1, 3]}}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$nor: [{$and: [{x: {$bitsAnySet: [1, 3]}}]}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 2}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 4}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionNotWithRegexObjSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$not: {$regex: 'a.b'}}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$nor: [{x: /a.b/}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 'abc'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 'acb'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionNotWithRegexValueSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$not: /a.b/}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$nor: [{x: /a.b/}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 'abc'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 'acb'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionNotWithRegexValueAndOptionsSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$not: /a.b/i}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$nor: [{x: /a.b/i}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 'abc'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 'acb'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionNotWithGeoSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson(
                         "{x: {$not: {$geoIntersects: {$geometry: {type: 'Polygon', "
                         "coordinates: [[[0,0], [5,0], "
                         "[5, 5], [0, 5], [0, 0]]]}}}}}"),
                     ExtensionsCallbackNoop(),
                     collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              fromjson(
                  "{$nor: [{$and: [{x: {$geoIntersects: {$geometry: {type: 'Polygon', coordinates: "
                  "[[[0,0], "
                  "[5,0], [5, 5], [0, 5], [0, 0]]]}}}}]}]}"));

    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));
    BSONObj obj =
        fromjson("{x: {type: 'Polygon', coordinates: [[4, 4], [4, 6], [6, 6], [6, 4], [4, 4]]}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson(
        "{x: {type: 'Polygon', coordinates: [[4, 4], [4, 4.5], [4.5, 4.5], [4.5, 4], [4, 4]]}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson(
        "{x: {type: 'Polygon', coordinates: [[5.5, 5.5], [5.5, 6], [6, 6], [6, 5.5], [5.5, "
        "5.5]]}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionNorSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(
        fromjson("{$nor: [{x: 3}, {x: {$lt: 1}}]}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{$nor: [{x: {$eq: 3}}, {x: {$lt: 1}}]}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 3}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 0}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: 2}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionTypeSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$type: 2}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$type: 2}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 3}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: '3'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionTypeWithNumberSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{x: {$type: 'number'}}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{x: {$type: 'number'}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 3}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: '3'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionEmptySerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: 3}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionWhereSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{$where: 'this.a == this.b'}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              BSONObjBuilder().appendCodeWScope("$where", "this.a == this.b", BSONObj()).obj());
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));
}

TEST(SerializeBasic, ExpressionWhereWithScopeSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(BSON("$where" << BSONCodeWScope("this.a == this.b", BSON("x" << 3))),
                     ExtensionsCallbackNoop(),
                     collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              BSON("$where" << BSONCodeWScope("this.a == this.b", BSON("x" << 3))));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));
}

TEST(SerializeBasic, ExpressionCommentSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{$comment: 'Hello'}"), ExtensionsCallbackNoop(), collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(), fromjson("{}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{a: 1, b: 2}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{a: 'z', b: 'z'}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionGeoWithinSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(
        fromjson(
            "{x: {$geoWithin: {$geometry: "
            "{type: 'Polygon', coordinates: [[[0, 0], [10, 0], [10, 10], [0, 10], [0, 0]]]}}}}"),
        ExtensionsCallbackNoop(),
        collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              fromjson(
                  "{x: {$geoWithin: {$geometry: {type: 'Polygon', coordinates: [[[0,0], [10,0], "
                  "[10, 10], [0, 10], [0, 0]]]}}}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj = fromjson("{x: {type: 'Point', coordinates: [5, 5]}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson("{x: {type: 'Point', coordinates: [50, 50]}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionGeoIntersectsSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(
        fromjson(
            "{x: {$geoIntersects: {$geometry: {type: 'Polygon', coordinates: [[[0,0], [5,0], [5, "
            "5], [0, 5], [0, 0]]]}}}}"),
        ExtensionsCallbackNoop(),
        collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              fromjson(
                  "{x: {$geoIntersects: {$geometry: {type: 'Polygon', coordinates: [[[0,0], [5,0], "
                  "[5, 5], [0, 5], [0, 0]]]}}}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));

    BSONObj obj =
        fromjson("{x: {type: 'Polygon', coordinates: [[4, 4], [4, 6], [6, 6], [6, 4], [4, 4]]}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson(
        "{x: {type: 'Polygon', coordinates: [[4, 4], [4, 4.5], [4.5, 4.5], [4.5, 4], [4, 4]]}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));

    obj = fromjson(
        "{x: {type: 'Polygon', coordinates: [[5.5, 5.5], [5.5, 6], [6, 6], [6, 5.5], [5.5, "
        "5.5]]}}");
    ASSERT_EQ(original.matches(obj), reserialized.matches(obj));
}

TEST(SerializeBasic, ExpressionNearSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(
        fromjson(
            "{x: {$near: {$geometry: {type: 'Point', coordinates: [0, 0]}, $maxDistance: 10, "
            "$minDistance: 1}}}"),
        ExtensionsCallbackNoop(),
        collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              fromjson(
                  "{x: {$near: {$geometry: {type: 'Point', coordinates: [0, 0]}, $maxDistance: 10, "
                  "$minDistance: 1}}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));
}

TEST(SerializeBasic, ExpressionNearSphereSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(
        fromjson(
            "{x: {$nearSphere: {$geometry: {type: 'Point', coordinates: [0, 0]}, $maxDistance: 10, "
            "$minDistance: 1}}}"),
        ExtensionsCallbackNoop(),
        collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              fromjson(
                  "{x: {$nearSphere: {$geometry: {type: 'Point', coordinates: [0, 0]}, "
                  "$maxDistance: 10, $minDistance: 1}}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));
}

TEST(SerializeBasic, ExpressionTextSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{$text: {$search: 'a', $language: 'en', $caseSensitive: true}}"),
                     ExtensionsCallbackNoop(),
                     collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              fromjson(
                  "{$text: {$search: 'a', $language: 'en', $caseSensitive: true, "
                  "$diacriticSensitive: false}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));
}

TEST(SerializeBasic, ExpressionTextWithDefaultLanguageSerializesCorrectly) {
    CollatorInterface* collator = nullptr;
    Matcher original(fromjson("{$text: {$search: 'a', $caseSensitive: false}}"),
                     ExtensionsCallbackNoop(),
                     collator);
    Matcher reserialized(
        serialize(original.getMatchExpression()), ExtensionsCallbackNoop(), collator);
    ASSERT_EQ(*reserialized.getQuery(),
              fromjson(
                  "{$text: {$search: 'a', $language: '', $caseSensitive: false, "
                  "$diacriticSensitive: false}}"));
    ASSERT_EQ(*reserialized.getQuery(), serialize(reserialized.getMatchExpression()));
}

}  // namespace
}  // namespace mongo
