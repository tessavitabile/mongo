// Test commands that are not allowed in multi-document transactions.
// @tags: [uses_transactions]
(function() {
    "use strict";

    const dbName = "test";
    const collName = "commands_not_allowed_in_txn";
    const testDB = db.getSiblingDB(dbName);
    const testColl = testDB[collName];

    testColl.drop();

    let txnNumber = 0;

    const sessionOptions = {causalConsistency: false};
    const session = db.getMongo().startSession(sessionOptions);
    const sessionDb = session.getDatabase(dbName);

    assert.commandWorked(
        testDB.createCollection(testColl.getName(), {writeConcern: {w: "majority"}}));
    assert.commandWorked(testDB.runCommand({
        createIndexes: collName,
        indexes: [
            {name: "geo_2d", key: {geo: "2d"}},
            {key: {haystack: "geoHaystack", a: 1}, name: "haystack_geo", bucketSize: 1}
        ],
        writeConcern: {w: "majority"}
    }));

    function setup() {
        testColl.dropIndex({a: 1});
        testDB.create_collection.drop();
        testDB.drop_collection.drop();
        assert.commandWorked(
            testDB.createCollection("drop_collection", {writeConcern: {w: "majority"}}));
    }

    function testCommand(command) {
        jsTest.log("Testing command: " + tojson(command));

        // Check that the command runs successfully outside transactions.
        setup();
        assert.commandWorked(sessionDb.runCommand(command));

        // Check that the command cannot be used to start a transaction.
        setup();
        assert.commandFailedWithCode(sessionDb.runCommand(Object.extend(command, {
            readConcern: {level: "snapshot"},
            txnNumber: NumberLong(++txnNumber),
            autocommit: false
        })),
                                     ErrorCodes.CommandFailed);
        assert.commandFailedWithCode(
            sessionDb.runCommand({commitTransaction: 1, txnNumber: NumberLong(txnNumber)}),
            ErrorCodes.NoSuchTransaction);

        // Check that the command fails inside a transaction, but does not abort the transaction.
        setup();
        assert.commandWorked(sessionDb.runCommand({
            insert: collName,
            documents: [{}],
            readConcern: {level: "snapshot"},
            txnNumber: NumberLong(++txnNumber),
            autocommit: false
        }));
        assert.commandFailedWithCode(
            sessionDb.runCommand(
                Object.extend(command, {txnNumber: NumberLong(txnNumber), autocommit: false})),
            ErrorCodes.CommandFailed);
        assert.commandWorked(
            sessionDb.runCommand({commitTransaction: 1, txnNumber: NumberLong(txnNumber)}));
    }

    //
    // Test commands that check out the session but are not allowed in multi-document transactions.
    //

    const sessionCommands = [
        {applyOps: [{op: "u", ns: testColl.getFullName(), o2: {_id: 0}, o: {$set: {a: 5}}}]},
        {count: collName},
        {explain: {find: collName}},
        {eval: "function() {return 1;}"},
        {"$eval": "function() {return 1;}"},
        {distinct: collName, key: "x"},
        {filemd5: 1, root: "fs"},
        {geoNear: collName, near: [0, 0]},
        {geoSearch: collName, near: [0, 0], maxDistance: 1, search: {a: 1}},
        {group: {ns: collName, key: {_id: 1}, $reduce: function(curr, result) {}, initial: {}}},
        {mapReduce: collName, map: function() {}, reduce: function(key, vals) {}, out: "out"},
        {parallelCollectionScan: collName, numCursors: 1},
        {refreshLogicalSessionCacheNow: 1}
    ];

    sessionCommands.forEach(testCommand);

    //
    // Test a selection of commands that do not check out the session. It is illegal to provide a
    // 'txnNumber' on these commands.
    //

    const nonSessionCommands = [
        {create: "create_collection", writeConcern: {w: "majority"}},
        {drop: "drop_collection", writeConcern: {w: "majority"}},
        {
          createIndexes: collName,
          indexes: [{name: "a_1", key: {a: 1}}],
          writeConcern: {w: "majority"}
        }
    ];

    nonSessionCommands.forEach(testCommand);

    nonSessionCommands.forEach(function(command) {
        setup();
        assert.commandFailedWithCode(
            sessionDb.runCommand(Object.extend(command, {txnNumber: NumberLong(++txnNumber)})),
            ErrorCodes.CommandFailed);
    });

    //
    // Test that doTxn is not allowed at positions after the first in transactions.
    //

    assert.commandWorked(sessionDb.runCommand({
        find: collName,
        readConcern: {level: "snapshot"},
        txnNumber: NumberLong(++txnNumber),
        autocommit: false
    }));
    assert.commandFailedWithCode(sessionDb.runCommand({
        doTxn: [{op: "u", ns: testColl.getFullName(), o2: {_id: 0}, o: {$set: {a: 5}}}],
        txnNumber: NumberLong(txnNumber)
    }),
                                 ErrorCodes.IllegalOperation);

    // It is still possible to commit the transaction. The rejected command does not abort the
    // transaction.
    assert.commandWorked(
        sessionDb.runCommand({commitTransaction: 1, txnNumber: NumberLong(txnNumber)}));

    session.endSession();
}());
