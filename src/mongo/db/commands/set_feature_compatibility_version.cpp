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

#include "mongo/bson/util/bson_extract.h"
#include "mongo/db/commands.h"
#include "mongo/db/db_raii.h"
#include "mongo/db/dbdirectclient.h"
#include "mongo/db/op_observer.h"
#include "mongo/db/ops/update.h"
#include "mongo/db/server_parameters.h"

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
        /*
        const ServerParameter::Map& parameterMap = ServerParameterSet::getGlobal()->getMap();
        auto parameter = parameterMap.find("featureCompatibilityVersion");
        auto status = parameter->second->set(cmdObj["setFeatureCompatibilityVersion"]);
        */

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

        DBDirectClient client(txn);
        client.update("admin.system.version", BSON("_id" << "featureCompatibilityVersion"), BSON("$set" << BSON("version" << version)), true);
        return true;
    }
} setFeatureCompatibilityVersion;

}  // namespace
}  // namespace mongo
