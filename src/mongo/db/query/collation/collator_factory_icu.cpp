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

#include "mongo/db/query/collation/collator_factory_icu.h"

#include <unicode/errorcode.h>

#include "mongo/bson/bsonobj.h"
#include "mongo/db/query/collation/collator_interface_icu.h"
#include "mongo/stdx/memory.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

namespace {

// Extracts the collation options from 'spec' and performs basic validation.
//
// Validation or normalization requiring the ICU library is done later.
StatusWith<CollationSpec> parseToCollationSpec(const BSONObj& spec) {
    CollationSpec parsedSpec;

    // Set defaults.
    parsedSpec.caseSensitive = false;
    parsedSpec.caseOrder = "off";
    parsedSpec.strength = 1;
    parsedSpec.numericCollation = false;
    parsedSpec.ignoreAlternateCharacters = false;
    parsedSpec.alternateCharacters = "all";
    parsedSpec.checkNormalization = false;
    parsedSpec.french = false;

    // Parse fields from spec and validate individual fields.
    for (auto elem : spec) {
        if (str::equals(CollationSpec::kLocaleField, elem.fieldName())) {
            if (elem.type() != BSONType::String) {
                return {ErrorCodes::FailedToParse,
                        str::stream() << "Field '" << CollationSpec::kLocaleField
                                      << "' must be of type string in: " << spec};
            }

            parsedSpec.localeID = elem.String();
        } else if (str::equals(CollationSpec::kCaseSensitiveField, elem.fieldName())) {
            if (elem.type() != BSONType::Bool) {
                return {ErrorCodes::FailedToParse,
                        str::stream() << "Field '" << CollationSpec::kCaseSensitiveField
                                      << "' must be of type bool in: " << spec};
            }

            parsedSpec.caseSensitive = elem.Bool();
        } else if (str::equals(CollationSpec::kCaseOrderField, elem.fieldName())) {
            if (elem.type() != BSONType::String) {
                return {ErrorCodes::FailedToParse,
                    str::stream() << "Field '" << CollationSpec::kCaseOrderField
                              << "' must be 'uppercaseFirst', 'lowercaseFirst', or 'off' in: " << spec};
            }

            if (!getCaseOrderAttribute(elem.String())) {
                return {ErrorCodes::FailedToParse,
                    str::stream() << "Field '" << CollationSpec::kCaseOrderField
                              << "' must be 'uppercaseFirst', 'lowercaseFirst', or 'off' in: " << spec};
            }

            parsedSpec.caseOrder = elem.String();
        } else if (str::equals(CollationSpec::kStrengthField, elem.fieldName())) {
            switch (elem.type()) {
                case NumberInt:
                    break;
                case NumberLong:
                    if (elem.numberInt() != elem.numberLong()) {
                        return {ErrorCodes::FailedToParse,
                                str::stream()
                                    << "Field '" << CollationSpec::kStrengthField
                                    << "' must be an integer 1 through 5 in: " << spec};
                    }
                    break;
                case NumberDouble:
                    if (elem.numberInt() != elem.numberDouble()) {
                        return {ErrorCodes::FailedToParse,
                                str::stream()
                                    << "Field '" << CollationSpec::kStrengthField
                                    << "' must be an integer 1 through 5 in: " << spec};
                    }
                    break;
                default:
                    return {ErrorCodes::FailedToParse,
                            str::stream() << "Field '" << CollationSpec::kStrengthField
                                          << "' must be an integer 1 through 5 in: " << spec};
            }

            if (!getStrengthAttribute(elem.numberInt())) {
                return {ErrorCodes::FailedToParse,
                            str::stream() << "Field '" << CollationSpec::kStrengthField
                                          << "' must be an integer 1 through 5 in: " << spec};
            }

            parsedSpec.strength = elem.numberInt();
        } else if (str::equals(CollationSpec::kNumericCollationField, elem.fieldName())) {
            if (elem.type() != BSONType::Bool) {
                return {ErrorCodes::FailedToParse,
                        str::stream() << "Field '" << CollationSpec::kNumericCollationField
                                      << "' must be of type bool in: " << spec};
            }

            parsedSpec.numericCollation = elem.Bool();
        } else if (str::equals(CollationSpec::kIgnoreAlternateCharactersField, elem.fieldName())) {
            if (elem.type() != BSONType::Bool) {
                return {ErrorCodes::FailedToParse,
                        str::stream() << "Field '" << CollationSpec::kIgnoreAlternateCharactersField
                                      << "' must be of type bool in: " << spec};
            }

            parsedSpec.ignoreAlternateCharacters = elem.Bool();
        } else if (str::equals(CollationSpec::kAlternateCharactersField, elem.fieldName())) {
            if (elem.type() != BSONType::String) {
                return {ErrorCodes::FailedToParse,
                        str::stream() << "Field '" << CollationSpec::kAlternateCharactersField
                                      << "' must be of type string in: " << spec};
            }

            if (!getAlternateCharactersAttribute(elem.String())) {
                return {ErrorCodes::FailedToParse,
                    str::stream() << "Field '" << CollationSpec::kCaseOrderField
                              << "' must be 'all', 'space', or 'punct' in: " << spec};
            }

            parsedSpec.alternateCharacters = elem.String();
        } else if (str::equals(CollationSpec::kCheckNormalizationField, elem.fieldName())) {
            if (elem.type() != BSONType::Bool) {
                return {ErrorCodes::FailedToParse,
                        str::stream() << "Field '" << CollationSpec::kCheckNormalizationField
                                      << "' must be of type bool in: " << spec};
            }

            parsedSpec.checkNormalization = elem.Bool();
        } else if (str::equals(CollationSpec::kFrenchField, elem.fieldName())) {
            if (elem.type() != BSONType::Bool) {
                return {ErrorCodes::FailedToParse,
                        str::stream() << "Field '" << CollationSpec::kFrenchField
                                      << "' must be of type bool in: " << spec};
            }

            parsedSpec.french = elem.Bool();
        } else {
            return {ErrorCodes::FailedToParse,
                    str::stream() << "Unknown collation spec field: " << elem.fieldName()};
        }
    }

    // Ensure localeID is present.
    if (parsedSpec.localeID.empty()) {
        return {ErrorCodes::FailedToParse, str::stream() << "Missing locale string"};
    }

    // Validate consistency of fields.
    if (!parsedSpec.caseSensitive && parsedSpec.caseOrder != "off") {
        return {ErrorCodes::FailedToParse, str::stream() << "Must have " << CollationSpec::kCaseOrderField
        << "='off' with " << CollationSpec::kCaseSensitiveField << "=false."};
    }

    if (!parsedSpec.ignoreAlternateCharacters && parsedSpec.alternateCharacters != "all") {
        return {ErrorCodes::FailedToParse, str::stream() << "Must have " << CollationSpec::kAlternateCharactersField
        << "='all' with " << CollationSpec::kIgnoreAlternateCharactersField << "=false."};
    }

    return parsedSpec;
}


UColAttribute getCaseSensitiveAttribute(const bool value) {
    return value ? UCOL_ON : UCOL_OFF;
}

boost::optional<UColAttribute> getCaseOrderAttribute(const std::string value) {
    switch (value) {
        case "uppercaseFirst":
            return UCOL_UPPER_FIRST;
        case "lowercaseFirst":
            return UCOL_LOWER_FIRST;
        case "off":
            return UCOL_OFF;
        default:
            return boost::none;
    }
}

boost::optional<UColAttribute> getStrengthAttribute(const unsigned value) {
    switch (value) {
        case 1:
            return UCOL_PRIMARY;
        case 2:
            return UCOL_SECONDARY;
        case 3:
            return UCOL_TERTIARY;
        case 4:
            return UCOL_QUATERNARY;
        case 5:
            return UCOL_IDENTICAL;
        default:
            return boost::none;
    }
}

UColAttribute getNumericCollationAttribute(const bool value) {
    return value ? UCOL_ON : UCOL_OFF;
}

boost::optional<UColAttribute> getAlternateCharactersAttribute(const std::string value) {
    switch (value) {
        case "all":
            return UCOL_DEFAULT;
        case "space":
            return UCOL_SHIFTED;
        case "punct":
            return UCOL_NON_IGNORABLE;
        default:
            return boost::none;
    }
}

UColAttribute getCheckNormalizationAttribute(const bool value) {
    return value ? UCOL_ON : UCOL_OFF;
}

UColAttribute getFrenchAttribute(const bool value) {
    return value ? UCOL_ON : UCOL_OFF;
}

// Check if localeID is recognized by ICU.
bool isValidLocale(const std::string localeID) {
    UErrorCode status = U_ZERO_ERROR;
    const size_t bufferSize = 100;
    UChar buffer[bufferSize];
    uloc_getDisplayName(localeID.c_str(), NULL, &buffer[0], bufferSize, &status);
    if (U_FAILURE(status) || U_USING_DEFAULT_WARNING == status) {
        return false;
    }
    return true;
}

}  // namespace

StatusWith<std::unique_ptr<CollatorInterface>> CollatorFactoryICU::makeFromBSON(
    const BSONObj& spec) {
    auto parsedSpec = parseToCollationSpec(spec);
    if (!parsedSpec.isOK()) {
        return parsedSpec.getStatus();
    }


    if (!isValidLocale(parsedSpec.getValue().localeID)) {
        return {ErrorCodes::BadValue,
                    str::stream() << "Field '" << CollationSpec::kLocaleField
                              << "' is not a valid ICU locale in: " << spec};
    }

    auto locale = icu::Locale::createFromName(parsedSpec.getValue().localeID.c_str());

    // Set localeID in the spec to the canonicalized locale name.
    parsedSpec.getValue().localeID = locale.getName();

    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::Collator> icuCollator(icu::Collator::createInstance(locale, status));
    if (U_FAILURE(status)) {
        icu::ErrorCode icuError;
        icuError.set(status);
        return {ErrorCodes::OperationFailed,
                str::stream() << "Failed to create collator: " << icuError.errorName()
                              << ". Collation spec: " << spec};
    }

    // Set case sensitive attribute.
    status = U_ZERO_ERROR;
    icuCollator->setAttribute(UCOL_CASE_LEVEL, getCaseSensitiveAttribute(parsedSpec.getValue().caseSensitive), status);
    if (U_FAILURE(status)) {
        icu::ErrorCode icuError;
        icuError.set(status);
        return {ErrorCodes::OperationFailed,
                str::stream() << "Failed to set '" << << CollationSpec::kCaseSensitiveField 
                              << "' attribute: " << icuError.errorName()
                              << ". Collation spec: " << spec};
    }

    // Set case order attribute.
    status = U_ZERO_ERROR;
    boost::optional<UColAttribute> caseOrderAttribute = getCaseOrderAttribute(parsedSpec.getValue().caseOrder);
    invariant(caseOrderAttribute);
    icuCollator->setAttribute(UCOL_CASE_FIRST, *caseOrderAttribute, status);
    if (U_FAILURE(status)) {
        icu::ErrorCode icuError;
        icuError.set(status);
        return {ErrorCodes::OperationFailed,
                str::stream() << "Failed to set '" << << CollationSpec::kCaseOrderField 
                              << "' attribute: " << icuError.errorName()
                              << ". Collation spec: " << spec};
    }

    // Set strength attribute.
    status = U_ZERO_ERROR;
    boost::optional<UColAttribute> strengthAttribute = getStrengthAttribute(parsedSpec.getValue().strength);
    invariant(strengthAttribute);
    icuCollator->setAttribute(UCOL_STRENGTH, *strengthAttribute, status);
    if (U_FAILURE(status)) {
        icu::ErrorCode icuError;
        icuError.set(status);
        return {ErrorCodes::OperationFailed,
                str::stream() << "Failed to set '" << << CollationSpec::kStrengthField 
                              << "' attribute: " << icuError.errorName()
                              << ". Collation spec: " << spec};
    }

    // Set numeric collation attribute.
    status = U_ZERO_ERROR;
    icuCollator->setAttribute(UCOL_NUMERIC_COLLATION, getNumericCollationAttribute(parsedSpec.getValue().numericCollation), status);
    if (U_FAILURE(status)) {
        icu::ErrorCode icuError;
        icuError.set(status);
        return {ErrorCodes::OperationFailed,
                str::stream() << "Failed to set '" << << CollationSpec::kNumericCollationField 
                              << "' attribute: " << icuError.errorName()
                              << ". Collation spec: " << spec};
    }

    // Set alternate characters attribute.
    status = U_ZERO_ERROR;
    boost::optional<UColAttribute> alternateCharactersAttribute = getAlternateCharactersAttribute(parsedSpec.getValue().alternateCharacters);
    invariant(alternateCharacters);
    icuCollator->setAttribute(UCOL_ALTERNATE_HANDLING, *alternateCharacters, status);
    if (U_FAILURE(status)) {
        icu::ErrorCode icuError;
        icuError.set(status);
        return {ErrorCodes::OperationFailed,
                str::stream() << "Failed to set '" << << CollationSpec::kAlternateCharactersField 
                              << "' attribute: " << icuError.errorName()
                              << ". Collation spec: " << spec};
    }

    // Set check normalization attribute.
    status = U_ZERO_ERROR;
    icuCollator->setAttribute(UCOL_NORMALIZATION_MODE, getCheckNormalizationAttribute(parsedSpec.getValue().checkNormalization), status);
    if (U_FAILURE(status)) {
        icu::ErrorCode icuError;
        icuError.set(status);
        return {ErrorCodes::OperationFailed,
                str::stream() << "Failed to set '" << << CollationSpec::kCheckNormalizationField 
                              << "' attribute: " << icuError.errorName()
                              << ". Collation spec: " << spec};
    }

    // Set French attribute.
    status = U_ZERO_ERROR;
    icuCollator->setAttribute(UCOL_FRENCH_COLLATION, getFrenchAttribute(parsedSpec.getValue().french), status);
    if (U_FAILURE(status)) {
        icu::ErrorCode icuError;
        icuError.set(status);
        return {ErrorCodes::OperationFailed,
                str::stream() << "Failed to set '" << << CollationSpec::kFrenchField 
                              << "' attribute: " << icuError.errorName()
                              << ". Collation spec: " << spec};
    }

    auto mongoCollator = stdx::make_unique<CollatorInterfaceICU>(std::move(parsedSpec.getValue()),
                                                                 std::move(icuCollator));
    return {std::move(mongoCollator)};
}

}  // namespace mongo
