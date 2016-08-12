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
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#pragma once

#include "mongo/base/string_data.h"

namespace mongo {

class BSONObj;
class OperationContext;

class FeatureCompatibilityVersion {
public:
    static constexpr StringData kCollection = "admin.system.version"_sd;
    static constexpr StringData kCommandName = "setFeatureCompatibilityVersion"_sd;
    static constexpr StringData kParameterName = "featureCompatibilityVersion"_sd;
    static constexpr StringData kVersionField = "version"_sd;
    static constexpr StringData kVersion34 = "3.4"_sd;
    static constexpr StringData kVersion32 = "3.2"_sd;

    /**
     * Sets the minimum allowed version in the cluster, which determines what features are
     * available.
     * 'version' should be '3.4' or '3.2'.
     */
    static void set(OperationContext* txn, StringData version);

    /**
     * Examines a document inserted into admin.system.version. If it is the
     * featureCompatibilityVersion document, validates the document and updates the server
     * parameter.
     */
    static void onInsertOrUpdate(const BSONObj& doc);

    /**
     * Examines a document inserted into admin.system.version. If it is the
     * featureCompatibilityVersion document, resets the server parameter to its default value (3.2).
     */
    static void onDelete(const BSONObj& doc);
};

}  // namespace mongo
