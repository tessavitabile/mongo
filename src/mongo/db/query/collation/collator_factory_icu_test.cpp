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

#include "mongo/platform/basic.h"
#include <unicode/errorcode.h>
#include <unicode/coll.h>
#include "mongo/db/query/collation/collator_factory_icu.h"

#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/unittest/unittest.h"

namespace {

using namespace mongo;

TEST(CollatorFactoryICUTest, LocaleStringParsesSuccessfully) {
    CollatorFactoryICU factory;
    auto collator = factory.makeFromBSON(BSON("locale"
                                              << "en_US"));
    ASSERT_OK(collator.getStatus());
    ASSERT_EQ("en_US", collator.getValue()->getSpec().localeID);
}

TEST(CollatorFactoryICUTest, LocaleStringCanonicalizesSuccessfully) {
    CollatorFactoryICU factory;
    auto collator = factory.makeFromBSON(BSON("locale"
                                              << "EN_US"));
    ASSERT_OK(collator.getStatus());
    ASSERT_EQ("en_US", collator.getValue()->getSpec().localeID);
}

TEST(CollatorFactoryICUTest, LocaleFieldNotAStringFailsToParse) {
    CollatorFactoryICU factory;
    auto collator = factory.makeFromBSON(BSON("locale" << 3));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, MissingLocaleStringFailsToParse) {
    CollatorFactoryICU factory;
    auto collator = factory.makeFromBSON(BSONObj());
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, UnknownSpecFieldFailsToParse) {
    BSONObj spec = BSON("locale"
                        << "en_US"
                        << "unknown"
                        << "field");
    CollatorFactoryICU factory;
    auto collator = factory.makeFromBSON(spec);
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, InvalidLocaleFieldFails) {
    CollatorFactoryICU factory;
    auto collator = factory.makeFromBSON(BSON("locale"
                                              << "garbage"));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::BadValue);
}

TEST(CollatorFactoryICUTest, AllFieldsParseSuccessfully) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength" << 1 << "numericCollation" << false
                                  << "ignoreAlternateCharacters" << true << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french" << true));
    ASSERT_OK(collator.getStatus());
    ASSERT_EQ("en_US", collator.getValue()->getSpec().localeID);
}

TEST(CollatorFactoryICUTest, LongStrengthFieldParsesSuccessfully) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength" << (long long)1 << "numericCollation" << false
                                  << "ignoreAlternateCharacters" << true << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french" << true));
    ASSERT_OK(collator.getStatus());
    ASSERT_EQ("en_US", collator.getValue()->getSpec().localeID);
}

TEST(CollatorFactoryICUTest, DoubleStrengthFieldParsesSuccessfully) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength" << 1.0 << "numericCollation" << false
                                  << "ignoreAlternateCharacters" << true << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french" << true));
    ASSERT_OK(collator.getStatus());
    ASSERT_EQ("en_US", collator.getValue()->getSpec().localeID);
}

TEST(CollatorFactoryICUTest, NonBooleanCaseSensitiveFieldFailsToParse) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive"
                                  << "garbage"
                                  << "caseOrder"
                                  << "off"
                                  << "strength" << 1 << "numericCollation" << false
                                  << "ignoreAlternateCharacters" << true << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french" << true));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, NonStringCaseOrderFieldFailsToParse) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder" << 1 << "strength" << 1
                                  << "numericCollation" << false << "ignoreAlternateCharacters"
                                  << true << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french" << true));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, NonNumberStrengthFieldFailsToParse) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength"
                                  << "garbage"
                                  << "numericCollation" << false << "ignoreAlternateCharacters"
                                  << true << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french" << true));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, TooLargeStrengthFieldFailsToParse) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength" << (long long)2147483648 << "numericCollation"
                                  << false << "ignoreAlternateCharacters" << true
                                  << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french" << true));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, FractionalStrengthFieldFailsToParse) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength" << 0.5 << "numericCollation" << false
                                  << "ignoreAlternateCharacters" << true << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french" << true));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, NegativeStrengthFieldFailsToParse) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength" << -1 << "numericCollation" << false
                                  << "ignoreAlternateCharacters" << true << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french" << true));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, NonBoolNumericCollationFieldFailsToParse) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength" << 1 << "numericCollation"
                                  << "garbage"
                                  << "ignoreAlternateCharacters" << true << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french" << true));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, NonBoolIgnoreAlternateCharactersFieldFailsToParse) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength" << 1 << "numericCollation" << false
                                  << "ignoreAlternateCharacters"
                                  << "garbage"
                                  << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french" << true));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, NonStringAlternateCharactersFieldFailsToParse) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength" << 1 << "numericCollation" << false
                                  << "ignoreAlternateCharacters" << true << "alternateCharacters"
                                  << 1 << "checkNormalization" << false << "french" << true));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, NonBoolCheckNormalizationFieldFailsToParse) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength" << 1 << "numericCollation" << false
                                  << "ignoreAlternateCharacters" << true << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization"
                                  << "garbage"
                                  << "french" << true));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, NonBoolFrenchFieldFailsToParse) {
    CollatorFactoryICU factory;
    auto collator =
        factory.makeFromBSON(BSON("locale"
                                  << "en_US"
                                  << "caseSensitive" << true << "caseOrder"
                                  << "off"
                                  << "strength" << 1 << "numericCollation" << false
                                  << "ignoreAlternateCharacters" << true << "alternateCharacters"
                                  << "all"
                                  << "checkNormalization" << false << "french"
                                  << "garbage"));
    ASSERT_NOT_OK(collator.getStatus());
    ASSERT_EQ(collator.getStatus(), ErrorCodes::FailedToParse);
}

TEST(CollatorFactoryICUTest, FactoryMadeCollatorComparesStringsCorrectlyEnUS) {
    CollatorFactoryICU factory;
    auto collator = factory.makeFromBSON(BSON("locale"
                                              << "en_US"));
    ASSERT_OK(collator.getStatus());

    ASSERT_LT(collator.getValue()->compare("ab", "ba"), 0);
    ASSERT_GT(collator.getValue()->compare("ba", "ab"), 0);
    ASSERT_EQ(collator.getValue()->compare("ab", "ab"), 0);
}

}  // namespace
