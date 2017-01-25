#include "stdafx.h"
#include "catch.h"
#include "trie.h"

// simplified version from : http://baptiste-wicht.com/posts/2016/06/reduce-compilation-time-by-another-16-with-catch.html
// this reduce the compliation time significantly, in favor of reduced funactionality that
// I generally don't need
void evaluate_result(const char* file, std::size_t line, const char* exp, bool val) {
	Catch::ResultBuilder result("REQUIRE", { file, line }, exp, Catch::ResultDisposition::Flags::Normal);
	result.setResultType(val);
	result.endExpression();
	result.react();
}

#define VERIFY(expr)  evaluate_result(__FILE__, __LINE__, #expr, (expr));

TEST_CASE("can create trie", "[trie]") {
	trie t;

	VERIFY(t.entries_count() == 0);
	auto read = t.try_read("hello");
	VERIFY(read.first == false);
}

TEST_CASE("can add to empty trie", "[trie]") {
	trie t;
	auto result = t.write("hello", 1);

	VERIFY(result == trie::result::success);
	VERIFY(t.entries_count() == 1);

	auto read = t.try_read("hello");
	VERIFY(read.first && read.second == 1);
}


TEST_CASE("can add to entry that is has starts with previous", "[trie]") {
	trie t;
	VERIFY(t.write("oren", 1) == trie::result::success);
	VERIFY(t.entries_count() == 1);
	VERIFY(t.write("oren eini", 2) == trie::result::success);
	VERIFY(t.entries_count() == 2);

	std::pair<bool, long> read = t.try_read("oren");
	VERIFY(read.first && read.second == 1);

	read = t.try_read("oren eini");
	VERIFY(read.first && read.second == 2);
}


TEST_CASE("can add to entry with shared prefix with previous", "[trie]") {
	trie t;
	VERIFY(t.write("oren", 1) == trie::result::success);
	VERIFY(t.entries_count() == 1);

	VERIFY(t.write("orange", 2) == trie::result::success);
	VERIFY(t.entries_count() == 2);


	std::pair<bool, long> read = t.try_read("oren");
	VERIFY(read.first && read.second == 1);

	read = t.try_read("orange");
	VERIFY(read.first && read.second == 2);
}


TEST_CASE("can add to entry that is a prefix of existing one", "[trie]") {
	trie t;
	VERIFY(t.write("oren eini", 1) == trie::result::success);
	VERIFY(t.entries_count() == 1);

	VERIFY(t.write("oren", 2) == trie::result::success);
	VERIFY(t.entries_count() == 2);

	std::pair<bool, long> read = t.try_read("oren");
	VERIFY(read.first && read.second == 2);

	read = t.try_read("oren eini");
	VERIFY(read.first && read.second == 1);
}


TEST_CASE("repeatedly allocating prefixes will track garbage", "[trie]") {
	trie t;
	VERIFY(t.write("oren eini", 1) == trie::result::success);
	VERIFY(t.entries_count() == 1);

	VERIFY(t.write("oren", 2) == trie::result::success);
	VERIFY(t.entries_count() == 2);

	VERIFY(t.write("orange", 3) == trie::result::success);
	VERIFY(t.entries_count() == 3);

	VERIFY(t.available_space_before_defrag() > 0);
	VERIFY(t.wasted_space() > 0);
}


TEST_CASE("can add many urls", "[trie]") {
	std::vector<std::string>  urls = {
		"admin/activate-hotspare",
		"admin/backup",
		"admin/changedbid",
		"admin/clear-hotspare-information",
		"admin/cluster/canJoin",
		"admin/cluster/changeVotingMode",
		"admin/cluster/commands/configuration",
		"admin/cluster/commands/database/{*id}",
		"admin/cluster/create",
		"admin/cluster/initialize-new-cluster/{*id}",
		"admin/cluster/join",
		"admin/cluster/leave",
		"admin/cluster/remove-clustering",
		"admin/cluster/update",
		"admin/cluster-statistics",
		"admin/compact",
		"admin/console/{*id}",
		"admin/cs/{*counterStorageName}",
		"admin/cs/{*id}",
		"admin/cs/batch-delete",
		"admin/cs/batch-toggle-disable",
		"admin/databases/{*id}",
		"admin/databases/batch-toggle-disable",
		"admin/databases-batch-delete",
		"admin/databases-toggle-disable",
		"admin/databases-toggle-indexing",
		"admin/databases-toggle-reject-clients",
		"admin/debug/auto-tuning-info",
		"admin/debug/info-package",
		"admin/detailed-storage-breakdown",
		"admin/dump",
		"admin/fs/{*id}",
		"admin/fs-batch-delete",
		"admin/fs-batch-toggle-disable",
		"admin/fs-compact",
		"admin/fs-restore",
		"admin/fs-toggle-disable",
		"admin/gc",
		"admin/generate-oauth-certificate",
		"admin/get-hotspare-information",
		"admin/indexingStatus",
		"admin/ioTest",
		"admin/killQuery",
		"admin/license/connectivity",
		"admin/license/forceUpdate",
		"admin/logs/configure",
		"admin/logs/events",
		"admin/loh-compaction",
		"admin/low-memory-handlers-statistics",
		"admin/low-memory-notification",
		"admin/optimize",
		"admin/periodicExport/purge-tombstones",
		"admin/replication/docs-left-to-replicate",
		"admin/replication/export-docs-left-to-replicate",
		"admin/replication/purge-tombstones",
		"admin/replication/replicated-docs-by-entity-names",
		"admin/replication/topology/discover",
		"admin/replication/topology/global",
		"admin/replication/topology/view",
		"admin/replicationInfo",
		"admin/restore",
		"admin/serverSmuggling",
		"admin/startIndexing",
		"admin/startReducing",
		"admin/stats",
		"admin/stopIndexing",
		"admin/stopReducing",
		"admin/tasks",
		"admin/test-hotspare",
		"admin/transactions/rollbackAll",
		"admin/ts/{*id}",
		"admin/ts/batch-delete",
		"admin/ts/batch-toggle-disable",
		"admin/verify-principal",
		"admin/voron/tree",
		"Benchmark/EmptyMessage",
		"bulk_docs",
		"bulk_docs/{*id}",
		"bulkInsert",
		"changes/config",
		"changes/events",
		"clientaccesspolicy.xml",
		"cluster/replicationState",
		"cluster/status",
		"cluster/topology",
		"configuration/document/{*docId}",
		"configuration/global/settings",
		"configuration/periodicExportSettings",
		"configuration/replication",
		"configuration/settings",
		"configuration/versioning",
		"cs",
		"cs/{counterStorageName}/admin/backup",
		"cs/{counterStorageName}/admin/replication/topology/discover",
		"cs/{counterStorageName}/admin/replication/topology/view",
		"cs/{counterStorageName}/batch",
		"cs/{counterStorageName}/by-prefix",
		"cs/{counterStorageName}/change",
		"cs/{counterStorageName}/changes/config",
		"cs/{counterStorageName}/changes/events",
		"cs/{counterStorageName}/counters",
		"cs/{counterStorageName}/debug/",
		"cs/{counterStorageName}/debug/metrics",
		"cs/{counterStorageName}/delete",
		"cs/{counterStorageName}/delete-by-group",
		"cs/{counterStorageName}/getCounter",
		"cs/{counterStorageName}/getCounterOverallTotal",
		"cs/{counterStorageName}/groups",
		"cs/{counterStorageName}/lastEtag",
		"cs/{counterStorageName}/metrics",
		"cs/{counterStorageName}/purge-tombstones",
		"cs/{counterStorageName}/replication",
		"cs/{counterStorageName}/replication/config",
		"cs/{counterStorageName}/replication/heartbeat",
		"cs/{counterStorageName}/replications/stats",
		"cs/{counterStorageName}/reset",
		"cs/{counterStorageName}/sinceEtag",
		"cs/{counterStorageName}/singleAuthToken",
		"cs/{counterStorageName}/stats",
		"cs/{counterStorageName}/streams/groups",
		"cs/{counterStorageName}/streams/summaries",
		"cs/debug/counter-storages",
		"cs/exists",
		"c-sharp-index-definition/{*fullIndexName}",
		"database/size",
		"database/storage/sizes",
		"databases",
		"debug/auto-tuning-info",
		"debug/cache-details",
		"debug/changes",
		"debug/clear-remaining-reductions",
		"debug/config",
		"debug/currently-indexing",
		"debug/d0crefs-t0ps",
		"debug/deletion-batch-stats",
		"debug/disable-query-timing",
		"debug/docrefs",
		"debug/enable-query-timing",
		"debug/filtered-out-indexes",
		"debug/format-index",
		"debug/gc-info",
		"debug/identities",
		"debug/index-fields",
		"debug/indexing-batch-stats",
		"debug/indexing-perf-stats",
		"debug/indexing-perf-stats-with-timings",
		"debug/info-package",
		"debug/list",
		"debug/list-all",
		"debug/metrics",
		"debug/plugins",
		"debug/prefetch-status",
		"debug/queries",
		"debug/raw-doc",
		"debug/reducing-batch-stats",
		"debug/remaining-reductions",
		"debug/replication-perf-stats",
		"debug/request-tracing",
		"debug/resource-drives",
		"debug/routes",
		"debug/sl0w-d0c-c0unts",
		"debug/sl0w-lists-breakd0wn",
		"debug/slow-dump-ref-csv",
		"debug/sql-replication-perf-stats",
		"debug/sql-replication-stats",
		"debug/subscriptions",
		"debug/suggest-index-merge",
		"debug/tasks",
		"debug/tasks/summary",
		"debug/thread-pool",
		"debug/transactions",
		"debug/user-info",
		"doc-preview",
		"docs",
		"docs/{*docId}",
		"facets/{*id}",
		"facets-multisearch",
		"favicon.ico",
		"fs",
		"fs/{fileSystemName}/admin/backup",
		"fs/{fileSystemName}/admin/optimize-index",
		"fs/{fileSystemName}/admin/replication/topology/discover",
		"fs/{fileSystemName}/admin/reset-index",
		"fs/{fileSystemName}/admin/synchronization/topology/view",
		"fs/{fileSystemName}/admin-restore",
		"fs/{fileSystemName}/changes/config",
		"fs/{fileSystemName}/changes/events",
		"fs/{fileSystemName}/config",
		"fs/{fileSystemName}/config/non-generated",
		"fs/{fileSystemName}/config/search",
		"fs/{fileSystemName}/files",
		"fs/{fileSystemName}/files/{*name}",
		"fs/{fileSystemName}/files-copy/{*name}",
		"fs/{fileSystemName}/folders/Subdirectories/{*directory}",
		"fs/{fileSystemName}/operation/kill",
		"fs/{fileSystemName}/operation/status",
		"fs/{fileSystemName}/operations",
		"fs/{fileSystemName}/rdc/Manifest/{*id}",
		"fs/{fileSystemName}/rdc/Signatures/{*id}",
		"fs/{fileSystemName}/rdc/Stats",
		"fs/{fileSystemName}/search",
		"fs/{fileSystemName}/search/Terms",
		"fs/{fileSystemName}/singleAuthToken",
		"fs/{fileSystemName}/static/FavIcon",
		"fs/{fileSystemName}/static/id",
		"fs/{fileSystemName}/stats",
		"fs/{fileSystemName}/storage/cleanup",
		"fs/{fileSystemName}/storage/retryCopying",
		"fs/{fileSystemName}/storage/retryRenaming",
		"fs/{fileSystemName}/streams/Export",
		"fs/{fileSystemName}/streams/files",
		"fs/{fileSystemName}/streams/Import",
		"fs/{fileSystemName}/streams/query",
		"fs/{fileSystemName}/studio-tasks/check-sufficient-diskspace",
		"fs/{fileSystemName}/studio-tasks/exportFilesystem",
		"fs/{fileSystemName}/studio-tasks/import",
		"fs/{fileSystemName}/studio-tasks/next-operation-id",
		"fs/{fileSystemName}/synchronization",
		"fs/{fileSystemName}/synchronization/Active",
		"fs/{fileSystemName}/synchronization/applyConflict/{*fileName}",
		"fs/{fileSystemName}/synchronization/Confirm",
		"fs/{fileSystemName}/synchronization/Conflicts",
		"fs/{fileSystemName}/synchronization/Finished",
		"fs/{fileSystemName}/synchronization/Incoming",
		"fs/{fileSystemName}/synchronization/IncrementLastETag",
		"fs/{fileSystemName}/synchronization/LastSynchronization",
		"fs/{fileSystemName}/synchronization/MultipartProceed",
		"fs/{fileSystemName}/synchronization/Pending",
		"fs/{fileSystemName}/synchronization/Rename",
		"fs/{fileSystemName}/synchronization/ResolutionStrategyFromServerResolvers",
		"fs/{fileSystemName}/synchronization/ResolveConflict/{*filename}",
		"fs/{fileSystemName}/synchronization/ResolveConflicts",
		"fs/{fileSystemName}/synchronization/start/{*filename}",
		"fs/{fileSystemName}/synchronization/Status",
		"fs/{fileSystemName}/synchronization/ToDestination",
		"fs/{fileSystemName}/synchronization/ToDestinations",
		"fs/{fileSystemName}/synchronization/UpdateMetadata/{*fileName}",
		"fs/{fileSystemName}/traffic-watch/events",
		"fs/admin/backup",
		"fs/stats",
		"fs/status",
		"generate/code",
		"identity/next",
		"identity/seed",
		"identity/seed/bulk",
		"indexes",
		"indexes/{*id}",
		"indexes/last-queried",
		"indexes/set-priority/{*id}",
		"indexes/try-recover-corrupted",
		"indexes-rename/{*id}",
		"indexes-set-priority/{*id}",
		"indexes-stats",
		"license/status",
		"license/support",
		"logs/{action}",
		"logs/fs/{action}",
		"morelikethis/{*id}",
		"multi_get",
		"OAuth/API-Key",
		"operation/alert/dismiss",
		"operation/alerts",
		"operation/kill",
		"operation/status",
		"operations",
		"plugins/status",
		"queries",
		"raft/appendEntries",
		"raft/canInstallSnapshot",
		"raft/disconnectFromCluster",
		"raft/installSnapshot",
		"raft/requestVote",
		"raft/timeoutNow",
		"raven",
		"raven/{*id}",
		"reduced-database-stats",
		"replication/explain/{*docId}",
		"replication/forceConflictResolution",
		"replication/heartbeat",
		"replication/info",
		"replication/lastEtag",
		"replication/replicateAttachments",
		"replication/replicateDocs",
		"replication/replicate-indexes",
		"replication/replicate-transformers",
		"replication/side-by-side/",
		"replication/topology",
		"replication/writeAssurance",
		"side-by-side-indexes",
		"silverlight/{*id}",
		"silverlight/ensureStartup",
		"singleAuthToken",
		"smuggler/export",
		"static/",
		"static/{*filename}",
		"static/{*id}",
		"stats",
		"streams/docs",
		"streams/exploration",
		"streams/query/{*id}",
		"studio",
		"studio/{*path}",
		"studio-tasks/check-sufficient-diskspace",
		"studio-tasks/collection/counts",
		"studio-tasks/config",
		"studio-tasks/createSampleData",
		"studio-tasks/createSampleDataClass",
		"studio-tasks/exportDatabase",
		"studio-tasks/get-sql-replication-stats",
		"studio-tasks/import",
		"studio-tasks/is-base-64-key",
		"studio-tasks/latest-server-build-version",
		"studio-tasks/loadCsvFile",
		"studio-tasks/new-encryption-key",
		"studio-tasks/next-operation-id",
		"studio-tasks/replication/conflicts/resolve",
		"studio-tasks/reset-sql-replication",
		"studio-tasks/resolveMerge",
		"studio-tasks/server-configs",
		"studio-tasks/simulate-sql-replication",
		"studio-tasks/sql-replication-toggle-disable",
		"studio-tasks/test-sql-replication-connection",
		"studio-tasks/validateCustomFunctions",
		"studio-tasks/validateExportOptions",
		"subscriptions",
		"subscriptions/acknowledgeBatch",
		"subscriptions/client-alive",
		"subscriptions/close",
		"subscriptions/create",
		"subscriptions/open",
		"subscriptions/pull",
		"subscriptions/setSubscriptionAckEtag",
		"suggest/{*id}",
		"terms/{*id}",
		"traffic-watch/events",
		"transaction/commit",
		"transaction/prepare",
		"transaction/rollback",
		"transaction/status",
		"transformers",
		"transformers/{*id}",
		"ts",
		"ts/{timeSeriesName}/aggregated-points/{type}",
		"ts/{timeSeriesName}/append/{type}",
		"ts/{timeSeriesName}/batch",
		"ts/{timeSeriesName}/changes/config",
		"ts/{timeSeriesName}/changes/events",
		"ts/{timeSeriesName}/delete-key/{type}",
		"ts/{timeSeriesName}/delete-points",
		"ts/{timeSeriesName}/delete-range/{type}",
		"ts/{timeSeriesName}/key/{type}",
		"ts/{timeSeriesName}/keys/{type}",
		"ts/{timeSeriesName}/lastEtag",
		"ts/{timeSeriesName}/points/{type}",
		"ts/{timeSeriesName}/replication",
		"ts/{timeSeriesName}/replication/heartbeat",
		"ts/{timeSeriesName}/replications/get",
		"ts/{timeSeriesName}/replications/save",
		"ts/{timeSeriesName}/singleAuthToken",
		"ts/{timeSeriesName}/stats",
		"ts/{timeSeriesName}/types",
		"ts/{timeSeriesName}/types/{type}",
		"ts/debug/time-serieses",
		"ts/debug/ts/{timeSeriesName}/debug/metrics"
	};

	trie t;
	for (size_t i = 0; i < urls.size(); i++)
	{
		auto result = t.write(urls[i], (long)i);
		VERIFY(result == trie::result::success);
	}

	for (size_t i = 0; i < urls.size(); i++)
	{
		auto result = t.try_read(urls[i]);
		VERIFY(result.first && result.second == i);
	}
}


TEST_CASE("can fill up the trie", "[trie]") {

	trie t;
	size_t i = 0;
	while (true)
	{
		auto result = t.write(std::to_string(i), (long)i);
		if (result != trie::result::success)
			break;
		i++;
	}
	VERIFY(t.entries_count() == i)
	for (size_t k = 0; k < i; k++)
	{
		auto val = t.try_read(std::to_string(k));

		VERIFY(val.first && val.second == k);
	}
}


TEST_CASE("can remove from trie", "[trie]") {

	trie t;
	t.write("oren eini", 1);
	t.write("oren", 2);
	t.write("orange", 3);

	VERIFY(t.remove("or") == false);
	VERIFY(t.entries_count() == 3);

	VERIFY(t.remove("oren"));

	VERIFY(t.entries_count() == 2);

	auto read = t.try_read("oren");
	VERIFY(read.first == false);
	read = t.try_read("oren eini");
	VERIFY(read.first && read.second == 1);
}


