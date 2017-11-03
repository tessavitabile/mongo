/**
 * Test that it is not possible to move a chunk from an upgrade featureCompatibilityVersion node to
 * a downgrade binary version node.
 *
 * We restart mongod during the test and expect it to have the same data after restarting.
 * @tags: [requires_persistence]
 */
(function() {
    "use strict";

    const downgradeVersion = "3.4";
    const dbpath =
        MongoRunner.dataPath + "migration_between_mixed_FCV_mixed_version_mongods_shard1";

    // Set up a sharded cluster with two shards.
    let st = new ShardingTest(
        {shards: [{binVersion: "latest"}, {binVersion: "latest", dbpath: dbpath}]});

    let admin = st.s.getDB("admin");
    let testDB = st.s.getDB("test");

    // Create a sharded collection with primary shard 0.
    assert.commandWorked(admin.runCommand({enableSharding: testDB.getName()}));
    st.ensurePrimaryShard(testDB.getName(), "shard0000");
    assert.commandWorked(
        admin.runCommand({shardCollection: testDB.coll.getFullName(), key: {a: 1}}));

    // Downgrade shard 1 to the downgrade binary version. Note that this requires running
    // setFeatureCompatibilityVersion on an individual shard, which is not advised.
    assert.commandWorked(
        st.shard1.adminCommand({setFeatureCompatibilityVersion: downgradeVersion}));
    MongoRunner.stopMongod(st.shard1);
    let conn =
        MongoRunner.runMongod({dbpath: dbpath, binVersion: downgradeVersion, noCleanData: true});
    assert.neq(null, conn, "failed to restart shard with lower binary version");

    // It should not be possible to migrate chunks to the downgrade shard.
    assert.commandFailed(
        admin.runCommand({moveChunk: testDB.coll.getName(), find: {a: 1}, to: "shard0001"}));

    st.stop();
})();
