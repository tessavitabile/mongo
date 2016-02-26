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

#pragma once

#include <string>

namespace mongo {

/**
 * A CollationSpec is a parsed representation of a user-provided collation BSONObj. Can be
 * re-serialized to BSON using the CollationSpecSerializer.
 */
struct CollationSpec {
    // Field name constants.
    static const char* kLocaleField;
    static const char* kCaseSensitiveField;
    static const char* kCaseOrderField;
    static const char* kStrengthField;
    static const char* kNumericCollationField;
    static const char* kIgnoreAlternateCharactersField;
    static const char* kAlternateCharactersField;
    static const char* kCheckNormalizationField;
    static const char* kFrenchField;

    // A string such as "en_US", identifying the language, country, or other attributes of the
    // locale for this collation.
    // Required.
    std::string localeID;

    // Ignore case sensitivity in comparisons.
    // Default: false.
    bool caseSensitive;

    // Uppercase or lowercase first.
    // Possible values: "uppercaseFirst", "lowercaseFirst", "off".
    // Default: "off".
    std::string caseOrder;

    // Prioritize the comparison properties.
    // Possible values: 1-5.
    // Default: 1.
    unsigned strength;

    // Order numbers based on numerical order and not collation order.
    // Default: false.
    bool numericCollation;

    // Spaces and punctuation.
    // Default: false.
    bool ignoreAlternateCharacters;

    // Used in combination with ignoreAlternateCharacters.
    // Possible values: "all", "space", "punct".
    // Default: "all".
    std::string alternateCharacters;

    // Any language that uses multiple combining characters such as Arabic, ancient Greek, Hebrew,
    // Hindi, Thai or Vietnamese either requires Normalization Checking to be on, or the text to go
    // through a normalization process before collation.
    // Default: false.
    bool checkNormalization;

    // Causes secondary differences to be considered in reverse order, as it is done in the French
    // language.
    // Default: false.
    bool french;
};

/**
 * Returns whether 'left' and 'right' are logically equivalent collations.
 */
inline bool operator==(const CollationSpec& left, const CollationSpec& right) {
    return ((left.localeID == right.localeID) && (left.caseSensitive == right.caseSensitive) &&
            (left.caseOrder == right.caseOrder) && (left.strength == right.strength) &&
            (left.numericCollation == right.numericCollation) &&
            (left.ignoreAlternateCharacters == right.ignoreAlternateCharacters) &&
            (left.alternateCharacters == right.alternateCharacters) &&
            (left.checkNormalization == right.checkNormalization) && (left.french == right.french));
}

inline bool operator!=(const CollationSpec& left, const CollationSpec& right) {
    return !(left == right);
}

}  // namespace mongo
