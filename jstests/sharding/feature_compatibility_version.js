(function() {
    'use strict';

    var res;

    var st = new ShardingTest({shards: {rs0: {nodes: 2}}});

    var configPrimary = st.configRS.getPrimary().getDB("admin");
    var configSecondary = st.configRS.getSecondary().getDB("admin");
    var shardPrimary = st.rs0.getPrimary().getDB("admin");
    var shardSecondary = st.rs0.getSecondary().getDB("admin");

    // Test setFeatureCompatibilityVersion against a replica set.

    // Initially featureCompatibilityVersion is 3.4.
    res = shardPrimary.runCommand({getParameter: 1, featureCompatibilityVersion: 1});
    assert.commandWorked(res);
    assert.eq(res.featureCompatibilityVersion, "3.4");

    // featureCompatibilityVersion cannot be set to invalid value.
    assert.commandFailed(shardPrimary.runCommand({setFeatureCompatibilityVersion: 5}));
    assert.commandFailed(shardPrimary.runCommand({setFeatureCompatibilityVersion: "3.0"}));

    // featureCompatibilityVersion cannot be set via setParameter.
    assert.commandFailed(shardPrimary.runCommand({setParameter: 1, featureCompatibilityVersion: "3.2"}));

    // featureCompatibilityVersion can be set to 3.2.
    assert.commandWorked(shardPrimary.runCommand({setFeatureCompatibilityVersion: "3.2"}));
    res = shardPrimary.runCommand({getParameter: 1, featureCompatibilityVersion: 1});
    assert.commandWorked(res);
    assert.eq(res.featureCompatibilityVersion, "3.2");

    // featureCompatibilityVersion propagated to secondary.
    st.rs0.awaitReplication();
    res = shardSecondary.runCommand({getParameter: 1, featureCompatibilityVersion: 1});
    assert.commandWorked(res);
    assert.eq(res.featureCompatibilityVersion, "3.2");

    // featureCompatibilityVersion can be set to 3.4.
    assert.commandWorked(shardPrimary.runCommand({setFeatureCompatibilityVersion: "3.4"}));
    res = shardPrimary.runCommand({getParameter: 1, featureCompatibilityVersion: 1});
    assert.commandWorked(res);
    assert.eq(res.featureCompatibilityVersion, "3.4");

    // Test setFeatureCompatibilityVersion against a sharded cluster.

    // mongos rejects bad values for featureCompatibilityVersion.

    st.stop();
})();
