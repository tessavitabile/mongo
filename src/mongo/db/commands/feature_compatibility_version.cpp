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

#include "mongo/platform/basic.h"

#include "mongo/db/commands/feature_compatibility_version.h"

#include "mongo/base/status.h"
#include "mongo/db/dbdirectclient.h"
#include "mongo/db/server_options.h"
#include "mongo/db/server_parameters.h"

namespace mongo {

constexpr StringData FeatureCompatibilityVersion::kCollection;
constexpr StringData FeatureCompatibilityVersion::kCommandName;
constexpr StringData FeatureCompatibilityVersion::kParameterName;
constexpr StringData FeatureCompatibilityVersion::kVersionField;
constexpr StringData FeatureCompatibilityVersion::kVersion34;
constexpr StringData FeatureCompatibilityVersion::kVersion32;

void FeatureCompatibilityVersion::set(OperationContext* txn, StringData version) {
    uassert(ErrorCodes::BadValue,
            "featureCompatibilityVersion must be '3.4' or '3.2'",
            version == FeatureCompatibilityVersion::kVersion34 ||
                version == FeatureCompatibilityVersion::kVersion32);

    // Update admin.system.version.
    DBDirectClient client(txn);
    const bool upsert = true;
    client.update(FeatureCompatibilityVersion::kCollection.toString(),
                  BSON("_id" << FeatureCompatibilityVersion::kParameterName),
                  BSON("$set" << BSON(FeatureCompatibilityVersion::kVersionField << version)),
                  upsert);

    // Update server parameter.
    if (version == FeatureCompatibilityVersion::kVersion34) {
        serverGlobalParams.featureCompatibilityVersion.store(
            ServerGlobalParams::FeatureCompatibilityVersion_34);
    } else if (version == FeatureCompatibilityVersion::kVersion32) {
        serverGlobalParams.featureCompatibilityVersion.store(
            ServerGlobalParams::FeatureCompatibilityVersion_32);
    }
}

void FeatureCompatibilityVersion::onInsertOrUpdate(const BSONObj& doc) {
    auto idElement = doc["_id"];
    if (idElement.type() != BSONType::String ||
        idElement.String() != FeatureCompatibilityVersion::kParameterName) {
        return;
    }
    for (auto&& elem : doc) {
        auto fieldName = elem.fieldNameStringData();
        if (fieldName == "_id") {
            continue;
        } else if (fieldName == FeatureCompatibilityVersion::kVersionField) {
            uassert(ErrorCodes::TypeMismatch,
                    str::stream() << FeatureCompatibilityVersion::kVersionField
                                  << " must be of type String, but was of type "
                                  << typeName(elem.type())
                                  << ". Contents of "
                                  << FeatureCompatibilityVersion::kParameterName
                                  << " document in "
                                  << FeatureCompatibilityVersion::kCollection
                                  << ": "
                                  << doc,
                    elem.type() == BSONType::String);
            std::string version = elem.String();
            if (version == FeatureCompatibilityVersion::kVersion34) {
                serverGlobalParams.featureCompatibilityVersion.store(
                    ServerGlobalParams::FeatureCompatibilityVersion_34);
            } else if (version == FeatureCompatibilityVersion::kVersion32) {
                serverGlobalParams.featureCompatibilityVersion.store(
                    ServerGlobalParams::FeatureCompatibilityVersion_32);
            } else {
                uasserted(ErrorCodes::BadValue,
                          str::stream() << "Invalid value for "
                                        << FeatureCompatibilityVersion::kVersionField
                                        << ", found "
                                        << version
                                        << ", expected '"
                                        << FeatureCompatibilityVersion::kVersion34
                                        << "' or '"
                                        << FeatureCompatibilityVersion::kVersion32
                                        << "'. Contents of "
                                        << FeatureCompatibilityVersion::kParameterName
                                        << " document in "
                                        << FeatureCompatibilityVersion::kCollection
                                        << ": "
                                        << doc);
            }
        } else {
            uasserted(ErrorCodes::BadValue,
                      str::stream() << "Unrecognized field '" << elem.fieldName()
                                    << "''. Contents of "
                                    << FeatureCompatibilityVersion::kParameterName
                                    << " document in "
                                    << FeatureCompatibilityVersion::kCollection
                                    << ": "
                                    << doc);
        }
    }
}

void FeatureCompatibilityVersion::onDelete(const BSONObj& doc) {
    auto idElement = doc["_id"];
    if (idElement.type() != BSONType::String ||
        idElement.String() != FeatureCompatibilityVersion::kParameterName) {
        return;
    }
    serverGlobalParams.featureCompatibilityVersion.store(
        ServerGlobalParams::FeatureCompatibilityVersion_32);
}

/**
 * Read-only server parameter for featureCompatibilityVersion.
 */
class FeatureCompatibilityVersionParameter : public ServerParameter {
public:
    FeatureCompatibilityVersionParameter()
        : ServerParameter(ServerParameterSet::getGlobal(),
                          FeatureCompatibilityVersion::kParameterName.toString(),
                          false,  // allowedToChangeAtStartup
                          false   // allowedToChangeAtRuntime
                          ) {}

    StringData featureCompatibilityVersionStr() {
        switch (serverGlobalParams.featureCompatibilityVersion.load()) {
            case ServerGlobalParams::FeatureCompatibilityVersion_34:
                return FeatureCompatibilityVersion::kVersion34;
            case ServerGlobalParams::FeatureCompatibilityVersion_32:
            default:
                return FeatureCompatibilityVersion::kVersion32;
        }
    }

    virtual void append(OperationContext* txn, BSONObjBuilder& b, const std::string& name) {
        b.append(name, featureCompatibilityVersionStr());
    }

    virtual Status set(const BSONElement& newValueElement) {
        return Status(ErrorCodes::IllegalOperation,
                      str::stream() << FeatureCompatibilityVersion::kParameterName
                                    << " cannot be set via setParameter");
    }

    virtual Status setFromString(const std::string& str) {
        return Status(ErrorCodes::IllegalOperation,
                      str::stream() << FeatureCompatibilityVersion::kParameterName
                                    << " cannot be set via setParameter");
    }
} featureCompatibilityVersionParameter;

}  // namespace mongo
