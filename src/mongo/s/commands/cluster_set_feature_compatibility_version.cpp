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

#include "mongo/bson/util/bson_extract.h"
#include "mongo/db/commands.h"
#include "mongo/s/client/shard.h"
#include "mongo/s/client/shard_connection.h"
#include "mongo/s/client/shard_registry.h"
#include "mongo/s/grid.h"

namespace mongo {

namespace {

class SetFeatureCompatibilityVersion : public Command {
public:
    SetFeatureCompatibilityVersion() : Command("setFeatureCompatibilityVersion") {}

    virtual bool slaveOk() const {
        return false;
    }

    virtual bool supportsWriteConcern(const BSONObj& cmd) const override {
        return false;
    }

    virtual void help(std::stringstream& help) const {
        help << "set the mininum version present in the cluster, to determine what features are "
                "allowed";
    }

    bool run(OperationContext* txn,
             const std::string& dbname,
             BSONObj& cmdObj,
             int options,
             std::string& errmsg,
             BSONObjBuilder& result) {

        // Validate command.
        BSONElement versionElement;
        auto status = bsonExtractTypedField(cmdObj, "setFeatureCompatibilityVersion", BSONType::String, &versionElement);
        if (!status.isOK()) {
            return appendCommandStatus(result, status);
        }
        std::string version = versionElement.String();
        if (version != "3.4" && version != "3.2") {
            return appendCommandStatus(result, Status(ErrorCodes::BadValue, str::stream() << "invalid value for featureCompatibilityVersion: " << version << ", expected '3.2' or '3.4'"));
        }

        // Forward to all shards (including config).
        std::vector<ShardId> shardIds;
        grid.shardRegistry()->getAllShardIds(&shardIds);
        shardIds.emplace_back("config");
        for (const ShardId& shardId : shardIds) {
            const auto shard = grid.shardRegistry()->getShard(txn, shardId);
            if (!shard) {
                continue;
            }

            ShardConnection conn(shard->getConnString(), "");
            BSONObj res;
            bool ok = conn->runCommand(dbname, cmdObj, res, options);
            conn.done();

            if (!ok) {
                result.appendElements(res);
                return false;
            }
        }

        return true;
    }
} clusterSetFeatureCompatibilityVersion;

}  // namespace
}  // namespace mongo
