// Tests for $expr in the CRUD commands.
(function() {
    "use strict";

    let coll = db.expr;

    //
    // $expr in aggregate. Included for completeness. More extensive variable testing is done
    // elsewhere.
    //

    coll.drop();
    assert.writeOK(coll.insert({a: 5}));
    assert.eq(1, coll.aggregate([{$match: {$expr: {$eq: ["$a", 5]}}}]).itcount());

    //
    // $expr in count.
    //

    coll.drop();
    assert.writeOK(coll.insert({a: 5}));
    assert.eq(1, coll.find({$expr: {$eq: ["$a", 5]}}).count());

    //
    // $expr in distinct.
    //

    coll.drop();
    assert.writeOK(coll.insert({a: 5}));
    assert.eq(1, coll.distinct("a", {$expr: {$eq: ["$a", 5]}}).length);

    //
    // $expr in find.
    //

    // $expr is allowed in query.
    coll.drop();
    assert.writeOK(coll.insert({a: 5}));
    assert.eq(1, coll.find({$expr: {$eq: ["$a", 5]}}).itcount());

    // $expr is allowed in find with explain.
    assert.commandWorked(coll.find({$expr: {$eq: ["$a", 5]}}).explain());

    // $expr is not allowed in $elemMatch projection.
    coll.drop();
    assert.writeOK(coll.insert({a: [{b: 5}]}));
    assert.throws(function() {
        coll.find({}, {a: {$elemMatch: {$expr: {$eq: ["$b", 5]}}}}).itcount();
    });

    //
    // $expr in findAndModify.
    //

    // $expr is allowed in the query when upsert=false.
    coll.drop();
    assert.writeOK(coll.insert({_id: 0, a: 5}));
    assert.eq({_id: 0, a: 5, b: 6},
              coll.findAndModify(
                  {query: {_id: 0, $expr: {$eq: ["$a", 5]}}, update: {$set: {b: 6}}, new: true}));

    // $expr is not allowed in the query when upsert=true.
    coll.drop();
    assert.writeOK(coll.insert({_id: 0, a: 5}));
    assert.throws(function() {
        coll.findAndModify(
            {query: {_id: 0, $expr: {$eq: ["$a", 5]}}, update: {$set: {b: 6}}, upsert: true});
    });

    // $expr is not allowed in $pull filter.
    coll.drop();
    assert.writeOK(coll.insert({_id: 0, a: [{b: 5}]}));
    assert.throws(function() {
        coll.findAndModify({query: {_id: 0}, update: {$pull: {a: {$expr: {$eq: ["$b", 5]}}}}});
    });

    // $expr is not allowed in arrayFilters.
    if (db.getMongo().writeMode() === "commands") {
        coll.drop();
        assert.writeOK(coll.insert({_id: 0, a: [{b: 5}]}));
        assert.throws(function() {
            coll.findAndModify({
                query: {_id: 0},
                update: {$set: {"a.$[i].b": 6}},
                arrayFilters: [{"i.b": 5, $expr: {$eq: ["$i.b", 5]}}]
            });
        });
    }

    //
    // $expr in geoNear.
    //

    coll.drop();
    assert.writeOK(coll.insert({geo: {type: "Point", coordinates: [0, 0]}, a: 5}));
    assert.commandWorked(coll.ensureIndex({geo: "2dsphere"}));
    assert.eq(1,
              assert
                  .commandWorked(db.runCommand({
                      geoNear: coll.getName(),
                      near: {type: "Point", coordinates: [0, 0]},
                      spherical: true,
                      query: {$expr: {$eq: ["$a", 5]}}
                  }))
                  .results.length);

    //
    // $expr in group.
    //

    coll.drop();
    assert.writeOK(coll.insert({a: 5}));
    assert.eq([{a: 5, count: 1}], coll.group({
        cond: {$expr: {$eq: ["$a", 5]}},
        key: {a: 1},
        initial: {count: 0},
        reduce: function(curr, result) {
            result.count += 1;
        }
    }));

    //
    // $expr in mapReduce.
    //

    coll.drop();
    assert.writeOK(coll.insert({a: 5}));
    let mapReduceOut = coll.mapReduce(
        function() {
            emit(this.str, 1);
        },
        function(key, values) {
            return Array.sum(values);
        },
        {out: {inline: 1}, query: {$expr: {$eq: ["$a", 5]}}});
    assert.commandWorked(mapReduceOut);
    assert.eq(mapReduceOut.results.length, 1, tojson(mapReduceOut));

    //
    // $expr in remove.
    //

    coll.drop();
    assert.writeOK(coll.insert({_id: 0, a: 5}));
    let writeRes = coll.remove({_id: 0, $expr: {$eq: ["$a", 5]}});
    assert.writeOK(writeRes);
    assert.eq(1, writeRes.nRemoved);

    //
    // $expr in update.
    //

    // $expr is allowed in the query when upsert=false.
    coll.drop();
    assert.writeOK(coll.insert({_id: 0, a: 5}));
    writeRes = coll.update({_id: 0, $expr: {$eq: ["$a", 5]}}, {$set: {b: 6}});
    assert.writeOK(writeRes);
    assert.eq(1, writeRes.nModified);

    // $expr is not allowed in the query when upsert=true.
    coll.drop();
    assert.writeOK(coll.insert({_id: 0, a: 5}));
    assert.writeError(
        coll.update({_id: 0, $expr: {$eq: ["$a", 5]}}, {$set: {b: 6}}, {upsert: true}));

    // $expr is not allowed in $pull filter.
    coll.drop();
    assert.writeOK(coll.insert({_id: 0, a: [{b: 5}]}));
    assert.writeError(coll.update({_id: 0}, {$pull: {a: {$expr: {$eq: ["$b", 5]}}}}));

    // $expr is not allowed in arrayFilters.
    if (db.getMongo().writeMode() === "commands") {
        coll.drop();
        assert.writeOK(coll.insert({_id: 0, a: [{b: 5}]}));
        assert.writeError(coll.update({_id: 0},
                                      {$set: {"a.$[i].b": 6}},
                                      {arrayFilters: [{"i.b": 5, $expr: {$eq: ["$i.b", 5]}}]}));
    }
})();
