// Tests that the killCursors command is allowed in transactions.
// @tags: [uses_transactions]
(function() {
    "use strict";

    const dbName = "test";
    const collName = "kill_cursors_in_transaction";
    const testDB = db.getSiblingDB(dbName);
    const session = db.getMongo().startSession({causalConsistency: false});
    const sessionDb = session.getDatabase(dbName);
    const sessionColl = sessionDb[collName];

    sessionColl.drop();
    for (let i = 0; i < 4; ++i) {
        assert.commandWorked(sessionColl.insert({_id: i}));
    }

    jsTest.log("Test that the killCursors command is allowed in transactions.");

    session.startTransaction();
    let res = assert.commandWorked(sessionDb.runCommand({find: collName, batchSize: 2}));
    assert(res.hasOwnProperty("cursor"), tojson(res));
    assert(res.cursor.hasOwnProperty("id"), tojson(res));
    assert.commandWorked(sessionDb.runCommand({killCursors: collName, cursors: [res.cursor.id]}));
    session.commitTransaction();

    jsTest.log("Test that the killCursors can be the first operation in a transaction.");

    res = assert.commandWorked(sessionDb.runCommand({find: collName, batchSize: 2}));
    assert(res.hasOwnProperty("cursor"), tojson(res));
    assert(res.cursor.hasOwnProperty("id"), tojson(res));
    session.startTransaction();
    assert.commandWorked(sessionDb.runCommand({killCursors: collName, cursors: [res.cursor.id]}));
    session.commitTransaction();

    jsTest.log("killCursors must not block on locks held by the transaction in which it is run.");

    session.startTransaction();

    // Open a cursor on the collection.
    res = assert.commandWorked(sessionDb.runCommand({find: collName, batchSize: 2}));
    assert(res.hasOwnProperty("cursor"), tojson(res));
    assert(res.cursor.hasOwnProperty("id"), tojson(res));

    // Start a drop, which will hang.
    let awaitDrop = startParallelShell(function() {
        db.getSiblingDB("test")["kill_cursors_in_transaction"].drop();
    });

    // Wait for the drop to have a pending MODE_X lock on the database.
    assert.soon(function() {
        return testDB.runCommand({find: collName, maxTimeMS: 100}).code ===
            ErrorCodes.ExceededTimeLimit;
    });

    // killCursors does not block behind the pending MODE_X lock.
    assert.commandWorked(sessionDb.runCommand({killCursors: collName, cursors: [res.cursor.id]}));

    session.commitTransaction();

    // Once the transaction has committed, the drop can proceed.
    awaitDrop();

    session.endSession();
}());
