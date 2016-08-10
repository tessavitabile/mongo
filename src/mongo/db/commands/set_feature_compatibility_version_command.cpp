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

#include "mongo/db/commands.h"
#include "mongo/db/commands/feature_compatibility_version.h"

namespace mongo {

namespace {

/**
 * Sets the minimum allowed version for the cluster. If it is 3.2, then the node should not use 3.4
 * features.
 *
 * Format:
 * {
 *   setFeatureCompatibilityVersion: <string version>
 * }
 */
class SetFeatureCompatibilityVersionCommand : public Command {
public:
    SetFeatureCompatibilityVersionCommand() : Command(FeatureCompatibilityVersion::kCommandName) {}

    virtual bool slaveOk() const {
        return false;
    }

    virtual bool adminOnly() const {
        return true;
    }

    virtual bool supportsWriteConcern(const BSONObj& cmd) const override {
        return true;
    }

    virtual void help(std::stringstream& help) const {
        help << "set the minimum allowed version in the cluster, which determines what features "
                "are available";
    }

    virtual Status checkAuthForCommand(ClientBasic* client,
                                       const std::string& dbname,
                                       const BSONObj& cmdObj) {
        // TODO: What should this be?
        return Status::OK();
    }

    bool run(OperationContext* txn,
             const std::string& dbname,
             BSONObj& cmdObj,
             int options,
             std::string& errmsg,
             BSONObjBuilder& result) {

        // Validate command.
        std::string version;
        for (auto&& elem : cmdObj) {
            if (elem.fieldNameStringData() == FeatureCompatibilityVersion::kCommandName) {
                if (elem.type() != BSONType::String) {
                    return appendCommandStatus(
                        result,
                        Status(ErrorCodes::TypeMismatch,
                               str::stream() << FeatureCompatibilityVersion::kCommandName
                                             << " must be a string, not a "
                                             << typeName(elem.type())));
                }
                version = elem.String();
            } else {
                return appendCommandStatus(
                    result,
                    Status(ErrorCodes::FailedToParse,
                           str::stream() << "unrecognized field '" << elem.fieldName() << "'"));
            }
        }

        if (version != FeatureCompatibilityVersion::kVersion34 &&
            version != FeatureCompatibilityVersion::kVersion32) {
            return appendCommandStatus(result,
                                       Status(ErrorCodes::BadValue,
                                              str::stream()
                                                  << "invalid value for "
                                                  << FeatureCompatibilityVersion::kCommandName
                                                  << ": "
                                                  << version
                                                  << ", expected '"
                                                  << FeatureCompatibilityVersion::kVersion34
                                                  << "' or '"
                                                  << FeatureCompatibilityVersion::kVersion32
                                                  << "'"));
        }

        // Set featureCompatibilityVersion.
        auto status = FeatureCompatibilityVersion::set(txn, version);
        if (!status.isOK()) {
            return appendCommandStatus(result, status);
        }

        return true;
    }
} setFeatureCompatibilityVersionCommand;

}  // namespace
}  // namespace mongo
