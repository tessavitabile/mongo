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

#include "mongo/db/auth/action_type.h"
#include "mongo/db/auth/authorization_session.h"
#include "mongo/db/commands.h"
#include "mongo/db/commands/feature_compatibility_version.h"
#include "mongo/s/client/shard.h"
#include "mongo/s/client/shard_registry.h"
#include "mongo/s/grid.h"

namespace mongo {

namespace {

/**
 * Internal sharding command run on config servers to set featureCompatibilityVersion on all shards.
 *
 * Format:
 * {
 *   _configsvrSetFeatureCompatibilityVersion: <string version>
 * }
 */
class ConfigSvrSetFeatureCompatibilityVersionCommand : public Command {
public:
    ConfigSvrSetFeatureCompatibilityVersionCommand()
        : Command("_configsvrSetFeatureCompatibilityVersion") {}

    void help(std::stringstream& help) const override {
        help << "Internal command, which is exported by the sharding config server. Do not call "
                "directly. Sets featureCompatibilityVersion on all shards.";
    }

    bool slaveOk() const override {
        return false;
    }

    bool adminOnly() const override {
        return true;
    }

    bool supportsWriteConcern(const BSONObj& cmd) const override {
        return true;
    }

    Status checkAuthForCommand(ClientBasic* client,
                               const std::string& dbname,
                               const BSONObj& cmdObj) override {
        if (!AuthorizationSession::get(client)->isAuthorizedForActionsOnResource(
                ResourcePattern::forClusterResource(), ActionType::internal)) {
            return Status(ErrorCodes::Unauthorized, "Unauthorized");
        }
        return Status::OK();
    }

    bool run(OperationContext* txn,
             const std::string& unusedDbName,
             BSONObj& cmdObj,
             int options,
             std::string& errmsg,
             BSONObjBuilder& result) override {
        if (serverGlobalParams.clusterRole != ClusterRole::ConfigServer) {
            return appendCommandStatus(
                result,
                Status(
                    ErrorCodes::IllegalOperation,
                    "_configsvrSetFeatureCompatibilityVersion can only be run on config servers"));
        }

        // Validate command.
        std::string version;
        for (auto&& elem : cmdObj) {
            if (elem.fieldNameStringData() == "_configsvrSetFeatureCompatibilityVersion") {
                if (elem.type() != BSONType::String) {
                    return appendCommandStatus(result,
                                               Status(ErrorCodes::TypeMismatch,
                                                      str::stream()
                                                          << "_configsvrSetFeatureCompatibilityVers"
                                                             "ion must be a string, not a "
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
            return appendCommandStatus(
                result,
                Status(ErrorCodes::BadValue,
                       str::stream()
                           << "invalid value for _configsvrSetFeatureCompatibilityVersion: "
                           << version
                           << ", expected '"
                           << FeatureCompatibilityVersion::kVersion34
                           << "' or '"
                           << FeatureCompatibilityVersion::kVersion32
                           << "'"));
        }

        // Set featureCompatibilityVersion on self.
        auto status = FeatureCompatibilityVersion::set(txn, version);
        if (!status.isOK()) {
            return appendCommandStatus(result, status);
        }

        // Forward to all shards.
        // TODO: Prevent shards from being added during this process.
        std::vector<ShardId> shardIds;
        grid.shardRegistry()->getAllShardIds(&shardIds);
        for (const ShardId& shardId : shardIds) {
            const auto shard = grid.shardRegistry()->getShard(txn, shardId);
            if (!shard) {
                continue;
            }

            auto response =
                shard->runCommand(txn,
                                  ReadPreferenceSetting{ReadPreference::PrimaryOnly},
                                  "admin",
                                  BSON(FeatureCompatibilityVersion::kCommandName << version),
                                  Shard::RetryPolicy::kIdempotent);

            if (!response.isOK()) {
                return appendCommandStatus(result, response.getStatus());
            }

            if (!response.getValue().commandStatus.isOK()) {
                return appendCommandStatus(result, response.getValue().commandStatus);
            }
        }

        return true;
    }
} configsvrSetFeatureCompatibilityVersionCmd;

}  // namespace
}  // namespace mongo
