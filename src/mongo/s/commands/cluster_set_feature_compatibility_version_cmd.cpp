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

#include "mongo/db/commands.h"
#include "mongo/s/client/shard.h"
#include "mongo/s/client/shard_registry.h"
#include "mongo/s/grid.h"

namespace mongo {

namespace {

/**
 * Sets the minimum allowed version for the cluster. If it is 3.2, then shards should not use 3.4
 * features.
 *
 * Format:
 * {
 *   setFeatureCompatibilityVersion: <string version>
 * }
 */
class SetFeatureCompatibilityVersionCmd : public Command {
public:
    SetFeatureCompatibilityVersionCmd() : Command("setFeatureCompatibilityVersion") {}

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
        help << "set the minimum allowed version for the cluster, which determines what features "
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
            if (elem.fieldNameStringData() == "setFeatureCompatibilityVersion") {
                if (elem.type() != BSONType::String) {
                    return appendCommandStatus(
                        result,
                        Status(ErrorCodes::TypeMismatch,
                               str::stream()
                                   << "setFeatureCompatibilityVersion must be a string, not a "
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

        if (version != "3.4" && version != "3.2") {
            return appendCommandStatus(
                result,
                Status(ErrorCodes::BadValue,
                       str::stream() << "invalid value for setFeatureCompatibilityVersion: "
                                     << version
                                     << ", expected '3.2' or '3.4'"));
        }

        // Forward to config shard, which will forward to all shards.
        auto configShard = Grid::get(txn)->shardRegistry()->getConfigShard();
        auto response =
            configShard->runCommand(txn,
                                    ReadPreferenceSetting{ReadPreference::PrimaryOnly},
                                    dbname,
                                    BSON("_configsvrSetFeatureCompatibilityVersion" << version),
                                    Shard::RetryPolicy::kIdempotent);
        if (!response.isOK()) {
            return appendCommandStatus(result, response.getStatus());
        }
        if (!response.getValue().commandStatus.isOK()) {
            return appendCommandStatus(result, response.getValue().commandStatus);
        }

        return true;
    }
} clusterSetFeatureCompatibilityVersionCmd;

}  // namespace
}  // namespace mongo
