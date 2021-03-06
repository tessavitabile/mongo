// Test to make sure that tag ranges get split
(function() {

var s = new ShardingTest({ name: "tag_auto_split",
                           shards: 2,
                           mongos: 1,
                           other: { enableBalancer : true } });

db = s.getDB( "test" );

s.adminCommand( { enablesharding : "test" } );
s.ensurePrimaryShard('test', 'shard0001');
s.adminCommand( { shardcollection : "test.foo" , key : { _id : 1 } } );

assert.eq( 1, s.config.chunks.count() );

sh.addShardTag( "shard0000" , "a" );

sh.addTagRange( "test.foo" , { _id : 5 } , { _id : 10 } , "a" );
sh.addTagRange( "test.foo" , { _id : 10 } , { _id : 15 } , "b" );

assert.soon( function() {
    return s.config.chunks.count() == 3;
}, "things didn't get split", 1000 * 60 * 10, 1000 );

s.printShardingStatus();

s.stop();

//test without full shard key on tags
s = new ShardingTest({ name: "tag_auto_split2",
                       shards: 2,
                       mongos: 1,
                       other: { enableBalancer : true } });

db = s.getDB( "test" );

s.adminCommand( { enablesharding : "test" } );
s.ensurePrimaryShard('test', 'shard0001');
s.adminCommand( { shardcollection : "test.foo" , key : { _id : 1, a : 1 } } );

assert.eq( 1, s.config.chunks.count() );

sh.addShardTag( "shard0000" , "a" );

sh.addTagRange( "test.foo" , { _id : 5 } , { _id : 10 } , "a" );
sh.addTagRange( "test.foo" , { _id : 10 } , { _id : 15 } , "b" );

assert.soon( function() {
    return s.config.chunks.count() == 3;
}, "things didn't get split", 1000 * 60 * 10, 1000 );

s.config.chunks.find().forEach(
    function(chunk){
        var numFields = 0;
        for ( var x in chunk.min ) {
            numFields++;
            assert( x == "_id" || x == "a", tojson(chunk) );
        }
        assert.eq( 2, numFields,tojson(chunk) );
    }
);

// check chunk mins correspond exactly to tag range boundaries, extended to match shard key
assert.eq( 1, s.config.chunks.find( {min : {_id : 5 , a : MinKey} } ).count(),
           "bad chunk range boundary" );
assert.eq( 1, s.config.chunks.find( {min : {_id : 10 , a : MinKey} } ).count(),
           "bad chunk range boundary" );

s.stop();

})();
