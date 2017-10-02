/**
 * Test that mongod will not allow creation of collection validators containing $expr when the
 * feature compatibility version is older than 3.6.
 *
 * We restart mongod during the test and expect it to have the same data after restarting.
 * @tags: [requires_persistence]
 */

(function() {
    "use strict";

    const testName = "expr_feature_compatibility_on_startup";
    let dbpath = MongoRunner.dataPath + testName;
    resetDbpath(dbpath);

    let conn = MongoRunner.runMongod({dbpath: dbpath, binVersion: "latest"});
    assert.neq(null, conn, "mongod was unable to start up");

    let testDB = conn.getDB(testName);
    assert.commandWorked(testDB.dropDatabase());

    let adminDB = conn.getDB("admin");

    // Explicitly set feature compatibility version 3.6.
    assert.commandWorked(adminDB.runCommand({setFeatureCompatibilityVersion: "3.6"}));

    // Create a collection with validator containing $expr.
    assert.commandWorked(
        testDB.createCollection("coll", {validator: {$expr: {$eq: ["$a", "good"]}}}));
    let coll = testDB.coll;

    // The validator should cause this insert to fail.
    assert.writeError(coll.insert({a: "bad"}), ErrorCodes.DocumentValidationFailure);

    // Set a validator containing $expr on an existing collection.
    assert.commandWorked(
        testDB.runCommand({collMod: "coll", validator: {$expr: {$eq: ["$a", "good"]}}}));

    // Another failing update.
    assert.writeError(coll.insert({a: "bad"}), ErrorCodes.DocumentValidationFailure);

    // Set the feature compatibility version to 3.4.
    assert.commandWorked(adminDB.runCommand({setFeatureCompatibilityVersion: "3.4"}));

    // The validator is already in place, so it should still cause this insert to fail.
    assert.writeError(coll.insert({a: "bad"}), ErrorCodes.DocumentValidationFailure);

    // Trying to create a new collection with a JSON Schema validator should fail while feature
    // compatibility version is 3.4.
    let res = testDB.createCollection("coll2", {validator: {$expr: {$eq: ["$a", "good"]}}});
    assert.commandFailedWithCode(res, ErrorCodes.ExprNotAllowed);
    assert(
        res.errmsg.match(/featureCompatibilityVersion/),
        "Expected error message from createCollection referencing 'featureCompatibilityVersion' but instead got: " +
            res.errmsg);

    // Trying to update a collection with a collection validator containing $expr should also fail.
    res = testDB.runCommand({collMod: "coll", validator: {$expr: {$eq: ["$a", "good"]}}});
    assert.commandFailedWithCode(res, ErrorCodes.ExprNotAllowed);
    assert(
        res.errmsg.match(/featureCompatibilityVersion/),
        "Expected error message from collMod referencing 'featureCompatibilityVersion' but instead got: " +
            res.errmsg);

    MongoRunner.stopMongod(conn);

    // If we try to start up a 3.4 mongod, it will fail, because it will not be able to parse the
    // validator.
    conn = MongoRunner.runMongod({dbpath: dbpath, binVersion: "3.4", noCleanData: true});
    assert.eq(null,
              conn,
              "mongod 3.4 started, even with a collection validator containing $expr in place.");

    // Starting up a 3.6 mongod, however, should succeed, even though the feature compatibility
    // version is still set to 3.4.
    conn = MongoRunner.runMongod({dbpath: dbpath, binVersion: "latest", noCleanData: true});
    assert.neq(null, conn, "mongod was unable to start up");

    adminDB = conn.getDB("admin");
    testDB = conn.getDB(testName);
    coll = testDB.coll;

    // And the validator should still work.
    assert.writeError(coll.insert({a: "bad"}), ErrorCodes.DocumentValidationFailure);

    // Remove the validator.
    assert.commandWorked(testDB.runCommand({collMod: "coll", validator: {}}));

    MongoRunner.stopMongod(conn);

    // Now, we should be able to start up a 3.4 mongod.
    conn = MongoRunner.runMongod({dbpath: dbpath, binVersion: "3.4", noCleanData: true});
    assert.neq(null, conn, "mongod 3.4 failed to start, even after we removed the validator");

    MongoRunner.stopMongod(conn);

    // The rest of the test uses mongod 3.6.
    conn = MongoRunner.runMongod({dbpath: dbpath, binVersion: "latest", noCleanData: true});
    assert.neq(null, conn, "mongod was unable to start up");

    adminDB = conn.getDB("admin");
    testDB = conn.getDB(testName);
    coll = testDB.coll;

    // Set the feature compatibility version back to 3.6.
    assert.commandWorked(adminDB.runCommand({setFeatureCompatibilityVersion: "3.6"}));

    // Now we should be able to create a collection with a validator containing $expr again.
    assert.commandWorked(
        testDB.createCollection("coll2", {validator: {$expr: {$eq: ["$a", "good"]}}}));

    // And we should be able to modify a collection to have a validator containing $expr.
    assert.commandWorked(
        testDB.runCommand({collMod: "coll", validator: {$expr: {$eq: ["$a", "good"]}}}));

    // Set the feature compatibility version to 3.4 and then restart with
    // internalValidateFeaturesAsMaster=false.
    assert.commandWorked(adminDB.runCommand({setFeatureCompatibilityVersion: "3.4"}));
    MongoRunner.stopMongod(conn);
    conn = MongoRunner.runMongod({
        dbpath: dbpath,
        binVersion: "latest",
        noCleanData: true,
        setParameter: "internalValidateFeaturesAsMaster=false"
    });
    assert.neq(null, conn, "mongod was unable to start up");

    testDB = conn.getDB(testName);

    // Even though the feature compatibility version is 3.4, we should still be able to add a
    // validator containing $expr, because internalValidateFeaturesAsMaster is false.
    assert.commandWorked(
        testDB.createCollection("coll3", {validator: {$expr: {$eq: ["$a", "good"]}}}));

    // We should also be able to modify a collection to have a $jsonSchema validator.
    assert.commandWorked(
        testDB.runCommand({collMod: "coll3", validator: {$expr: {$eq: ["$a", "good"]}}}));

    MongoRunner.stopMongod(conn);
}());
