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

#include "mongo/db/query/collation/collation_spec.h"

#include "mongo/unittest/unittest.h"

namespace {

using namespace mongo;

TEST(CollationSpecTest, SpecsWithNonEqualLocaleStringsAreNotEqual) {
    CollationSpec collationSpec1;
    collationSpec1.localeID = "fr";
    collationSpec1.caseSensitive = true;
    collationSpec1.caseOrder = "uppercaseFirst";
    collationSpec1.strength = 1;
    collationSpec1.numericCollation = false;
    collationSpec1.ignoreAlternateCharacters = true;
    collationSpec1.alternateCharacters = "all";
    collationSpec1.checkNormalization = false;
    collationSpec1.french = false;

    CollationSpec collationSpec2;
    collationSpec2.localeID = "de";
    collationSpec2.caseSensitive = true;
    collationSpec2.caseOrder = "uppercaseFirst";
    collationSpec2.strength = 1;
    collationSpec2.numericCollation = false;
    collationSpec2.ignoreAlternateCharacters = true;
    collationSpec2.alternateCharacters = "all";
    collationSpec2.checkNormalization = false;
    collationSpec2.french = false;

    ASSERT_FALSE(collationSpec1 == collationSpec2);
    ASSERT_TRUE(collationSpec1 != collationSpec2);
}

TEST(CollationSpecTest, SpecsWithNonEqualCaseSensitiveValuesAreNotEqual) {
    CollationSpec collationSpec1;
    collationSpec1.localeID = "fr";
    collationSpec1.caseSensitive = true;
    collationSpec1.caseOrder = "uppercaseFirst";
    collationSpec1.strength = 1;
    collationSpec1.numericCollation = false;
    collationSpec1.ignoreAlternateCharacters = true;
    collationSpec1.alternateCharacters = "all";
    collationSpec1.checkNormalization = false;
    collationSpec1.french = false;

    CollationSpec collationSpec2;
    collationSpec2.localeID = "fr";
    collationSpec2.caseSensitive = false;
    collationSpec2.caseOrder = "uppercaseFirst";
    collationSpec2.strength = 1;
    collationSpec2.numericCollation = false;
    collationSpec2.ignoreAlternateCharacters = true;
    collationSpec2.alternateCharacters = "all";
    collationSpec2.checkNormalization = false;
    collationSpec2.french = false;

    ASSERT_FALSE(collationSpec1 == collationSpec2);
    ASSERT_TRUE(collationSpec1 != collationSpec2);
}

TEST(CollationSpecTest, SpecsWithNonEqualCaseOrderStringsAreNotEqual) {
    CollationSpec collationSpec1;
    collationSpec1.localeID = "fr";
    collationSpec1.caseSensitive = true;
    collationSpec1.caseOrder = "uppercaseFirst";
    collationSpec1.strength = 1;
    collationSpec1.numericCollation = false;
    collationSpec1.ignoreAlternateCharacters = true;
    collationSpec1.alternateCharacters = "all";
    collationSpec1.checkNormalization = false;
    collationSpec1.french = false;

    CollationSpec collationSpec2;
    collationSpec2.localeID = "fr";
    collationSpec2.caseSensitive = true;
    collationSpec2.caseOrder = "off";
    collationSpec2.strength = 1;
    collationSpec2.numericCollation = false;
    collationSpec2.ignoreAlternateCharacters = true;
    collationSpec2.alternateCharacters = "all";
    collationSpec2.checkNormalization = false;
    collationSpec2.french = false;

    ASSERT_FALSE(collationSpec1 == collationSpec2);
    ASSERT_TRUE(collationSpec1 != collationSpec2);
}

TEST(CollationSpecTest, SpecsWithNonEqualStrengthsAreNotEqual) {
    CollationSpec collationSpec1;
    collationSpec1.localeID = "fr";
    collationSpec1.caseSensitive = true;
    collationSpec1.caseOrder = "uppercaseFirst";
    collationSpec1.strength = 1;
    collationSpec1.numericCollation = false;
    collationSpec1.ignoreAlternateCharacters = true;
    collationSpec1.alternateCharacters = "all";
    collationSpec1.checkNormalization = false;
    collationSpec1.french = false;

    CollationSpec collationSpec2;
    collationSpec2.localeID = "fr";
    collationSpec2.caseSensitive = true;
    collationSpec2.caseOrder = "uppercaseFirst";
    collationSpec2.strength = 2;
    collationSpec2.numericCollation = false;
    collationSpec2.ignoreAlternateCharacters = true;
    collationSpec2.alternateCharacters = "all";
    collationSpec2.checkNormalization = false;
    collationSpec2.french = false;

    ASSERT_FALSE(collationSpec1 == collationSpec2);
    ASSERT_TRUE(collationSpec1 != collationSpec2);
}

TEST(CollationSpecTest, SpecsWithNonEqualNumericCollationValuesAreNotEqual) {
    CollationSpec collationSpec1;
    collationSpec1.localeID = "fr";
    collationSpec1.caseSensitive = true;
    collationSpec1.caseOrder = "uppercaseFirst";
    collationSpec1.strength = 1;
    collationSpec1.numericCollation = false;
    collationSpec1.ignoreAlternateCharacters = true;
    collationSpec1.alternateCharacters = "all";
    collationSpec1.checkNormalization = false;
    collationSpec1.french = false;

    CollationSpec collationSpec2;
    collationSpec2.localeID = "fr";
    collationSpec2.caseSensitive = true;
    collationSpec2.caseOrder = "uppercaseFirst";
    collationSpec2.strength = 1;
    collationSpec2.numericCollation = true;
    collationSpec2.ignoreAlternateCharacters = true;
    collationSpec2.alternateCharacters = "all";
    collationSpec2.checkNormalization = false;
    collationSpec2.french = false;

    ASSERT_FALSE(collationSpec1 == collationSpec2);
    ASSERT_TRUE(collationSpec1 != collationSpec2);
}

TEST(CollationSpecTest, SpecsWithNonEqualIgnoreAlternateCharactersValuesAreNotEqual) {
    CollationSpec collationSpec1;
    collationSpec1.localeID = "fr";
    collationSpec1.caseSensitive = true;
    collationSpec1.caseOrder = "uppercaseFirst";
    collationSpec1.strength = 1;
    collationSpec1.numericCollation = false;
    collationSpec1.ignoreAlternateCharacters = true;
    collationSpec1.alternateCharacters = "all";
    collationSpec1.checkNormalization = false;
    collationSpec1.french = false;

    CollationSpec collationSpec2;
    collationSpec2.localeID = "fr";
    collationSpec2.caseSensitive = true;
    collationSpec2.caseOrder = "uppercaseFirst";
    collationSpec2.strength = 1;
    collationSpec2.numericCollation = false;
    collationSpec2.ignoreAlternateCharacters = false;
    collationSpec2.alternateCharacters = "all";
    collationSpec2.checkNormalization = false;
    collationSpec2.french = false;

    ASSERT_FALSE(collationSpec1 == collationSpec2);
    ASSERT_TRUE(collationSpec1 != collationSpec2);
}

TEST(CollationSpecTest, SpecsWithNonEqualAlternateCharactersStringsAreNotEqual) {
    CollationSpec collationSpec1;
    collationSpec1.localeID = "fr";
    collationSpec1.caseSensitive = true;
    collationSpec1.caseOrder = "uppercaseFirst";
    collationSpec1.strength = 1;
    collationSpec1.numericCollation = false;
    collationSpec1.ignoreAlternateCharacters = true;
    collationSpec1.alternateCharacters = "all";
    collationSpec1.checkNormalization = false;
    collationSpec1.french = false;

    CollationSpec collationSpec2;
    collationSpec2.localeID = "fr";
    collationSpec2.caseSensitive = true;
    collationSpec2.caseOrder = "uppercaseFirst";
    collationSpec2.strength = 1;
    collationSpec2.numericCollation = false;
    collationSpec2.ignoreAlternateCharacters = true;
    collationSpec2.alternateCharacters = "space";
    collationSpec2.checkNormalization = false;
    collationSpec2.french = false;

    ASSERT_FALSE(collationSpec1 == collationSpec2);
    ASSERT_TRUE(collationSpec1 != collationSpec2);
}

TEST(CollationSpecTest, SpecsWithNonEqualCheckNormalizationValuesAreNotEqual) {
    CollationSpec collationSpec1;
    collationSpec1.localeID = "fr";
    collationSpec1.caseSensitive = true;
    collationSpec1.caseOrder = "uppercaseFirst";
    collationSpec1.strength = 1;
    collationSpec1.numericCollation = false;
    collationSpec1.ignoreAlternateCharacters = true;
    collationSpec1.alternateCharacters = "all";
    collationSpec1.checkNormalization = false;
    collationSpec1.french = false;

    CollationSpec collationSpec2;
    collationSpec2.localeID = "fr";
    collationSpec2.caseSensitive = true;
    collationSpec2.caseOrder = "uppercaseFirst";
    collationSpec2.strength = 1;
    collationSpec2.numericCollation = false;
    collationSpec2.ignoreAlternateCharacters = true;
    collationSpec2.alternateCharacters = "all";
    collationSpec2.checkNormalization = true;
    collationSpec2.french = false;

    ASSERT_FALSE(collationSpec1 == collationSpec2);
    ASSERT_TRUE(collationSpec1 != collationSpec2);
}

TEST(CollationSpecTest, SpecsWithNonEqualFrenchValuesAreNotEqual) {
    CollationSpec collationSpec1;
    collationSpec1.localeID = "fr";
    collationSpec1.caseSensitive = true;
    collationSpec1.caseOrder = "uppercaseFirst";
    collationSpec1.strength = 1;
    collationSpec1.numericCollation = false;
    collationSpec1.ignoreAlternateCharacters = true;
    collationSpec1.alternateCharacters = "all";
    collationSpec1.checkNormalization = false;
    collationSpec1.french = false;

    CollationSpec collationSpec2;
    collationSpec2.localeID = "fr";
    collationSpec2.caseSensitive = true;
    collationSpec2.caseOrder = "uppercaseFirst";
    collationSpec2.strength = 1;
    collationSpec2.numericCollation = false;
    collationSpec2.ignoreAlternateCharacters = true;
    collationSpec2.alternateCharacters = "all";
    collationSpec2.checkNormalization = false;
    collationSpec2.french = true;

    ASSERT_FALSE(collationSpec1 == collationSpec2);
    ASSERT_TRUE(collationSpec1 != collationSpec2);
}

TEST(CollationSpecTest, EqualSpecs) {
    CollationSpec collationSpec1;
    collationSpec1.localeID = "fr";
    collationSpec1.caseSensitive = true;
    collationSpec1.caseOrder = "uppercaseFirst";
    collationSpec1.strength = 1;
    collationSpec1.numericCollation = false;
    collationSpec1.ignoreAlternateCharacters = true;
    collationSpec1.alternateCharacters = "all";
    collationSpec1.checkNormalization = false;
    collationSpec1.french = false;

    CollationSpec collationSpec2;
    collationSpec2.localeID = "fr";
    collationSpec2.caseSensitive = true;
    collationSpec2.caseOrder = "uppercaseFirst";
    collationSpec2.strength = 1;
    collationSpec2.numericCollation = false;
    collationSpec2.ignoreAlternateCharacters = true;
    collationSpec2.alternateCharacters = "all";
    collationSpec2.checkNormalization = false;
    collationSpec2.french = false;

    ASSERT_TRUE(collationSpec1 == collationSpec2);
    ASSERT_FALSE(collationSpec1 != collationSpec2);
}

}  // namespace
