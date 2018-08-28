/**
 * Tests that rebuilding multiple indexes at startup succeeds. Getting into this state requires
 * replication, but the code can execute in any mode.
 *
 * @tags: [requires_persistence, requires_replication]
 */
(function() {
    "use strict";

    load("jstests/libs/check_log.js");

    const rst = new ReplSetTest({
        name: "doNotRebuildIndexesIfNeedInitialSync",
        nodes: 1,
        nodeOptions:
            {setParameter: {logComponentVerbosity: tojsononeline({storage: {recovery: 2}})}}
    });
    const nodes = rst.startSet();
    rst.initiate();

    let coll = rst.getPrimary().getDB("indexRebuild")["coll"];
    assert.commandWorked(coll.createIndexes([{a: 1}, {b: 1}], {}, {writeConcern: {w: "majority"}}));
    assert.eq(3, coll.getIndexes().length);
    for (var i = 0; i < 200; i++) {
        assert.commandWorked(coll.insert({a: 1, b: 1, i: 1}));
    }

    // Add a SECONDARY node.
    jsTestLog("Adding secondary node.");
    rst.add();

    var secondary = rst.getSecondary();
    var secondaryDBPath = secondary.dbpath;
    var collectionClonerFailPoint = "initialSyncHangDuringCollectionClone";

    // Make the collection cloner pause after its initial 'find' response on the capped collection.
    var nss = "indexRebuild.coll";
    jsTestLog("Enabling collection cloner fail point for " + nss);
    assert.commandWorked(secondary.adminCommand(
        {configureFailPoint: collectionClonerFailPoint, mode: 'alwaysOn', data: {namespace: nss, numDocsToClone: 100}}));

    // Let the SECONDARY begin initial sync.
    jsTestLog("Re-initiating replica set with new secondary.");
    rst.reInitiate();

    jsTestLog("Waiting for the initial 'find' response of collection cloner to complete.");
    checkLog.contains(
        secondary,
        "initial sync - initialSyncHangDuringCollectionClone fail point enabled");

    // Restart the secondary.
    jsTestLog("Stopping secondary");
    rst.stop(1);

    jsTestLog("Restarting secondary as standalone");
    let mongod = MongoRunner.runMongod({dbpath: secondaryDBPath, noCleanData: true});

    /*assert.commandWorked(mongod.adminCommand(
            {configureFailPoint: "hangBeforeIndexBuildOf", mode: "alwaysOn", data: {i: 1}}));


    var shell = startParallelShell(function() {assert.commandFailed(db.getSiblingDB("indexRebuild")["coll"].createIndex({i: 1}))}, mongod.port);

    jsTestLog("Waiting to hang before index build of i=1");
    checkLog.contains(mongod, "Hanging before index build of i=1");*/


    assert.commandWorked(
        mongod.adminCommand({configureFailPoint: 'slowBackgroundIndexBuild', mode: 'alwaysOn'}));

    var shell = startParallelShell(function() {assert.commandFailed(db.getSiblingDB("indexRebuild")["coll"].createIndex({i: 1}))}, mongod.port);

    //assert.commandWorked(mongod.getDB("indexRebuild")["coll"].createIndex({i: 1}, {background: true}));
    /*jsTestLog("Disable snapshotting");
    assert.commandWorked(mongod.adminCommand(
                      {configureFailPoint: "disableSnapshotting", mode: "alwaysOn"}));

    jsTestLog("Dropping indexes");
    assert.commandWorked(mongod.getDB("indexRebuild")["coll"].dropIndexes());

    jsTestLog("Killing standalone");
    MongoRunner.stopMongod(mongod, 9, {allowedExitCode: MongoRunner.EXIT_SIGKILL});*/

    jsTestLog("Stopping standalone");
    MongoRunner.stopMongod(mongod);
    //shell.join();

    jsTestLog("Restarting secondary");
    rst.start(1, {startClean: false}, true);

    rst.waitForState(secondary, ReplSetTest.State.SECONDARY);

    assert.eq(3, secondary.getDB("indexRebuild")["coll"].getIndexes().length);
    rst.stopSet();
})();
