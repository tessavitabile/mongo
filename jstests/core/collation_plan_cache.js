// Integration testing for the plan cache and index filter commands with collation.
(function() {
    'use strict';

    var coll = db.collation_plan_cache;
    coll.drop();

    var res;

    assert.writeOK(coll.insert({a: 'foo', b: 5}));

    // We need two indexes that the query can use so that the MultiPlanRunner is executed.
    assert.commandWorked(coll.createIndex({a: 1}, {collation: {locale: 'en_US'}}));
    assert.commandWorked(coll.createIndex({a: 1, b: 1}, {collation: {locale: 'en_US'}}));

    // We need an index with a different collation, so that string locations affect the query shape.
    assert.commandWorked(coll.createIndex({b: 1}, {collation: {locale: 'fr_CA'}}));

    // listQueryShapes().

    // Run a query so that an entry is inserted into the cache.
    assert.commandWorked(
        coll.runCommand("find", {filter: {a: 'foo', b: 5}, collation: {locale: "en_US"}}),
        'find command failed');

    // The query shape should have been added.
    res = coll.getPlanCache().listQueryShapes();
    assert.eq(1, res.length, 'unexpected cache size after running query');
    assert.eq(res[0],
              {
                query: {a: 'foo', b: 5},
                sort: {},
                projection: {},
                collation: {
                    locale: 'en_US',
                    caseLevel: false,
                    caseFirst: 'off',
                    strength: 3,
                    numericOrdering: false,
                    alternate: 'non-ignorable',
                    maxVariable: 'punct',
                    normalization: false,
                    backwards: false,
                    version: '57.1'
                }
              },
              'unexpected query shape returned from listQueryShapes()');

    coll.getPlanCache().clear();

    // getPlansByQuery().

    // Run a query so that an entry is inserted into the cache.
    assert.commandWorked(
        coll.runCommand("find", {filter: {a: 'foo', b: 5}, collation: {locale: "en_US"}}),
        'find command failed');

    // The query should have cached plans.
    assert.lt(
        0,
        coll.getPlanCache()
            .getPlansByQuery(
                {query: {a: 'foo', b: 5}, sort: {}, projection: {}, collation: {locale: 'en_US'}})
            .length,
        'unexpected number of cached plans for query');

    // A query with no collation should have no cached plans.
    assert.eq(
        0,
        coll.getPlanCache()
            .getPlansByQuery({query: {a: 'foo', b: 5}, sort: {}, projection: {}, collation: {}})
            .length,
        'unexpected number of cached plans for query');

    // A query with a different collation should have no cached plans.
    assert.eq(
        0,
        coll.getPlanCache()
            .getPlansByQuery(
                {query: {a: 'foo', b: 5}, sort: {}, projection: {}, collation: {locale: 'fr_CA'}})
            .length,
        'unexpected number of cached plans for query');

    // A query with different string locations should have no cached plans.
    assert.eq(0,
              coll.getPlanCache()
                  .getPlansByQuery({
                      query: {a: 'foo', b: 'bar'},
                      sort: {},
                      projection: {},
                      collation: {locale: 'en_US'}
                  })
                  .length,
              'unexpected number of cached plans for query');

    coll.getPlanCache().clear();

    // clearPlansByQuery().

    // Run a query so that an entry is inserted into the cache.
    assert.commandWorked(
        coll.runCommand("find", {filter: {a: 'foo', b: 5}, collation: {locale: "en_US"}}),
        'find command failed');
    assert.eq(1,
              coll.getPlanCache().listQueryShapes().length,
              'unexpected cache size after running query');

    // Dropping a query shape with no collation should have no effect.
    coll.getPlanCache().clearPlansByQuery(
        {query: {a: 'foo', b: 5}, sort: {}, projection: {}, collation: {}});
    assert.eq(1,
              coll.getPlanCache().listQueryShapes().length,
              'unexpected cache size after dropping uncached query shape');

    // Dropping a query shape with a different collation should have no effect.
    coll.getPlanCache().clearPlansByQuery(
        {query: {a: 'foo', b: 5}, sort: {}, projection: {}, collation: {locale: 'fr_CA'}});
    assert.eq(1,
              coll.getPlanCache().listQueryShapes().length,
              'unexpected cache size after dropping uncached query shape');

    // Dropping a query shape with different string locations should have no effect.
    coll.getPlanCache().clearPlansByQuery(
        {query: {a: 'foo', b: 'bar'}, sort: {}, projection: {}, collation: {locale: 'en_US'}});
    assert.eq(1,
              coll.getPlanCache().listQueryShapes().length,
              'unexpected cache size after dropping uncached query shape');

    // Dropping query shape.
    coll.getPlanCache().clearPlansByQuery(
        {query: {a: 'foo', b: 5}, sort: {}, projection: {}, collation: {locale: 'en_US'}});
    assert.eq(0,
              coll.getPlanCache().listQueryShapes().length,
              'unexpected cache size after dropping query shapes');

    // Index filter commands.

    // Set a plan cache filter.
    assert.commandWorked(
        coll.runCommand(
            'planCacheSetFilter',
            {query: {a: 'foo', b: 5}, collation: {locale: 'en_US'}, indexes: [{a: 1, b: 1}]}),
        'planCacheSetFilter failed');

    // Check the plan cache filter was added.
    res = coll.runCommand('planCacheListFilters');
    assert.commandWorked(res, 'planCacheListFilters failed');
    assert.eq(1, res.filters.length, 'unexpected number of plan cache filters');
    assert.eq(res.filters[0],
              {
                query: {a: 'foo', b: 5},
                sort: {},
                projection: {},
                collation: {locale: 'en_US'},
                indexes: [{a: 1, b: 1}]
              },
              'unexpected plan cache filter');

    // Clearing a plan cache filter with no collation should have no effect.
    assert.commandWorked(coll.runCommand('planCacheClearFilters', {query: {a: 'foo', b: 5}}));
    assert.eq(1,
              coll.runCommand('planCacheListFilters').filters.length,
              'unexpected number of plan cache filters');

    // Clearing a plan cache filter with a different collation should have no effect.
    assert.commandWorked(coll.runCommand('planCacheClearFilters',
                                         {query: {a: 'foo', b: 5}, collation: {locale: 'fr_CA'}}));
    assert.eq(1,
              coll.runCommand('planCacheListFilters').filters.length,
              'unexpected number of plan cache filters');

    // Clearing a plan cache filter with different string locations should have no effect.
    assert.commandWorked(coll.runCommand(
        'planCacheClearFilters', {query: {a: 'foo', b: 'bar', collation: {locale: 'en_US'}}}));
    assert.eq(1,
              coll.runCommand('planCacheListFilters').filters.length,
              'unexpected number of plan cache filters');

    // Clear plan cache filter.
    assert.commandWorked(coll.runCommand('planCacheClearFilters',
                                         {query: {a: 'foo', b: 5}, collation: {locale: 'en_US'}}));
    assert.eq(0,
              coll.runCommand('planCacheListFilters').filters.length,
              'unexpected number of plan cache filters');
})();