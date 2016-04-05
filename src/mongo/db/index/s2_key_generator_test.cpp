/**
 *    Copyright (C) 2014 MongoDB Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kIndex

#include "mongo/platform/basic.h"

#include "mongo/db/index/expression_keys_private.h"

#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/db/index/s2_common.h"
#include "mongo/db/index/expression_params.h"
#include "mongo/db/json.h"
#include "mongo/db/query/collation/collator_interface_mock.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/log.h"

using namespace mongo;

namespace {

const long long originCellId(1152921504606846977);

//
// Helper functions
//

std::string dumpKeyset(const BSONObjSet& objs) {
    std::stringstream ss;
    ss << "[ ";
    for (BSONObjSet::iterator i = objs.begin(); i != objs.end(); ++i) {
        ss << i->toString() << " ";
    }
    ss << "]";

    return ss.str();
}

bool assertKeysetsEqual(const BSONObjSet& expectedKeys, const BSONObjSet& actualKeys) {
    if (expectedKeys != actualKeys) {
        log() << "Expected: " << dumpKeyset(expectedKeys) << ", "
              << "Actual: " << dumpKeyset(actualKeys);
        return false;
    }
    return true;
}

//
// Unit tests
//

TEST(S2KeyGeneratorTest, CollationAppliedToNonGeoStringFields) {
    BSONObj obj = fromjson("{a: {type: 'Point', coordinates: [0, 0]}, b: 'string'}");
    BSONObj keyPattern = fromjson("{a: '2dsphere', b: 1}");
    BSONObj infoObj = fromjson("{key: {a: '2dsphere', b: 1}, '2dsphereIndexVersion': 3}");
    S2IndexingParams params;
    ExpressionParams::parse2dsphereParams(infoObj, &params);
    BSONObjSet actualKeys;
    CollatorInterfaceMock collator(CollatorInterfaceMock::MockType::kReverseString);
    ExpressionKeysPrivate::getS2Keys(obj, keyPattern, params, &actualKeys, &collator);

    BSONObjSet expectedKeys;
    expectedKeys.insert(BSON("" << originCellId << ""
                                << "gnirts"));

    ASSERT(assertKeysetsEqual(expectedKeys, actualKeys));
}

TEST(S2KeyGeneratorTest, CollationAppliedToStringsInArray) {
    BSONObj obj = fromjson("{a: {type: 'Point', coordinates: [0, 0]}, b: ['string', 'string2']}");
    BSONObj keyPattern = fromjson("{a: '2dsphere', b: 1}");
    BSONObj infoObj = fromjson("{key: {a: '2dsphere', b: 1}, '2dsphereIndexVersion': 3}");
    S2IndexingParams params;
    ExpressionParams::parse2dsphereParams(infoObj, &params);
    BSONObjSet actualKeys;
    CollatorInterfaceMock collator(CollatorInterfaceMock::MockType::kReverseString);
    ExpressionKeysPrivate::getS2Keys(obj, keyPattern, params, &actualKeys, &collator);

    BSONObjSet expectedKeys;
    expectedKeys.insert(BSON("" << originCellId << ""
                                << "gnirts"));
    expectedKeys.insert(BSON("" << originCellId << ""
                                << "2gnirts"));

    ASSERT(assertKeysetsEqual(expectedKeys, actualKeys));
}

TEST(S2KeyGeneratorTest, CollationDoesNotAffectNonStringFields) {
    BSONObj obj = fromjson("{a: {type: 'Point', coordinates: [0, 0]}, b: 5}");
    BSONObj keyPattern = fromjson("{a: '2dsphere', b: 1}");
    BSONObj infoObj = fromjson("{key: {a: '2dsphere', b: 1}, '2dsphereIndexVersion': 3}");
    S2IndexingParams params;
    ExpressionParams::parse2dsphereParams(infoObj, &params);
    BSONObjSet actualKeys;
    CollatorInterfaceMock collator(CollatorInterfaceMock::MockType::kReverseString);
    ExpressionKeysPrivate::getS2Keys(obj, keyPattern, params, &actualKeys, &collator);

    BSONObjSet expectedKeys;
    expectedKeys.insert(BSON("" << originCellId << "" << 5));

    ASSERT(assertKeysetsEqual(expectedKeys, actualKeys));
}

// TODO SERVER-23172: remove test
TEST(S2KeyGeneratorTest, CollationDoesNotAffectStringsInEmbeddedDocuments) {
    BSONObj obj = fromjson("{a: {type: 'Point', coordinates: [0, 0]}, b: {c: 'string'}}");
    BSONObj keyPattern = fromjson("{a: '2dsphere', b: 1}");
    BSONObj infoObj = fromjson("{key: {a: '2dsphere', b: 1}, '2dsphereIndexVersion': 3}");
    S2IndexingParams params;
    ExpressionParams::parse2dsphereParams(infoObj, &params);
    BSONObjSet actualKeys;
    CollatorInterfaceMock collator(CollatorInterfaceMock::MockType::kReverseString);
    ExpressionKeysPrivate::getS2Keys(obj, keyPattern, params, &actualKeys, &collator);

    BSONObjSet expectedKeys;
    expectedKeys.insert(BSON("" << originCellId << "" << BSON("c"
                                                              << "string")));

    ASSERT(assertKeysetsEqual(expectedKeys, actualKeys));
}

TEST(S2KeyGeneratorTest, NoCollation) {
    BSONObj obj = fromjson("{a: {type: 'Point', coordinates: [0, 0]}, b: 'string'}");
    BSONObj keyPattern = fromjson("{a: '2dsphere', b: 1}");
    BSONObj infoObj = fromjson("{key: {a: '2dsphere', b: 1}, '2dsphereIndexVersion': 3}");
    S2IndexingParams params;
    ExpressionParams::parse2dsphereParams(infoObj, &params);
    BSONObjSet actualKeys;
    ExpressionKeysPrivate::getS2Keys(obj, keyPattern, params, &actualKeys, nullptr);

    BSONObjSet expectedKeys;
    expectedKeys.insert(BSON("" << originCellId << ""
                                << "string"));

    ASSERT(assertKeysetsEqual(expectedKeys, actualKeys));
}

}  // namespace
