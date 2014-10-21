#include "jobs.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include "sockopt.h"
#include "logger.h"
#include "client_global.h"
#include "fdfs_global.h"
#include "fdfs_client.h"

#include "common_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "config.h"

struct configItem g_config_list[] = {
	{"mysql_server", ""},
	{"db_name", ""},
	{"db_user", ""},
	{"db_pwd", ""},
	{"log_name", ""}
};

int g_log_fd;

#define DB_SERVER		g_config_list[0].value
#define DB_NAME			g_config_list[1].value
#define DB_USER			g_config_list[2].value
#define DB_PWD			g_config_list[3].value
#define LOG_NAME		g_config_list[4].value

static ConnectionInfo *pTrackerServer;

static int list_all_groups(const char *group_name,char *output_str);

static int list_storages(FDFSGroupStat *pGroupStat,char *output_str)
{
	int result;
	int storage_count;
	FDFSStorageInfo storage_infos[FDFS_MAX_SERVERS_EACH_GROUP];
	FDFSStorageInfo *p;
	FDFSStorageInfo *pStorage;
	FDFSStorageInfo *pStorageEnd;
	FDFSStorageStat *pStorageStat;
	char szJoinTime[32];
	char szUpTime[32];
	char szLastHeartBeatTime[32];
	char szSrcUpdTime[32];
	char szSyncUpdTime[32];
	char szSyncedTimestamp[32];
	char szSyncedDelaySeconds[128];
	char szHostname[128];
	char szHostnamePrompt[128+8];
	int k;
	int max_last_source_update;

	sprintf(output_str, "%sgroup name = %s\n" \
		"disk total space = "INT64_PRINTF_FORMAT" MB\n" \
		"disk free space = "INT64_PRINTF_FORMAT" MB\n" \
		"trunk free space = "INT64_PRINTF_FORMAT" MB\n" \
		"storage server count = %d\n" \
		"active server count = %d\n" \
		"storage server port = %d\n" \
		"storage HTTP port = %d\n" \
		"store path count = %d\n" \
		"subdir count per path = %d\n" \
		"current write server index = %d\n" \
		"current trunk file id = %d\n\n", \
		output_str,
		pGroupStat->group_name, \
		pGroupStat->total_mb, \
		pGroupStat->free_mb, \
		pGroupStat->trunk_free_mb, \
		pGroupStat->count, \
		pGroupStat->active_count, \
		pGroupStat->storage_port, \
		pGroupStat->storage_http_port, \
		pGroupStat->store_path_count, \
		pGroupStat->subdir_count_per_path, \
		pGroupStat->current_write_server, \
		pGroupStat->current_trunk_file_id
	);

	result = tracker_list_servers(pTrackerServer, \
		pGroupStat->group_name, NULL, \
		storage_infos, FDFS_MAX_SERVERS_EACH_GROUP, \
		&storage_count);
	if (result != 0)
	{
		return result;
	}

	k = 0;
	pStorageEnd = storage_infos + storage_count;
	for (pStorage=storage_infos; pStorage<pStorageEnd; \
		pStorage++)
	{
		max_last_source_update = 0;
		for (p=storage_infos; p<pStorageEnd; p++)
		{
			if (p != pStorage && p->stat.last_source_update
				> max_last_source_update)
			{
				max_last_source_update = \
					p->stat.last_source_update;
			}
		}

		pStorageStat = &(pStorage->stat);
		if (max_last_source_update == 0)
		{
			*szSyncedDelaySeconds = '\0';
		}
		else
		{
			if (pStorageStat->last_synced_timestamp == 0)
			{
				strcpy(szSyncedDelaySeconds, "(never synced)");
			}
			else
			{
			int delay_seconds;
			int remain_seconds;
			int day;
			int hour;
			int minute;
			int second;
			char szDelayTime[64];
			
			delay_seconds = (int)(max_last_source_update - \
				pStorageStat->last_synced_timestamp);
			day = delay_seconds / (24 * 3600);
			remain_seconds = delay_seconds % (24 * 3600);
			hour = remain_seconds / 3600;
			remain_seconds %= 3600;
			minute = remain_seconds / 60;
			second = remain_seconds % 60;

			if (day != 0)
			{
				sprintf(szDelayTime, "%d days " \
					"%02dh:%02dm:%02ds", \
					day, hour, minute, second);
			}
			else if (hour != 0)
			{
				sprintf(szDelayTime, "%02dh:%02dm:%02ds", \
					hour, minute, second);
			}
			else if (minute != 0)
			{
				sprintf(szDelayTime, "%02dm:%02ds", minute, second);
			}
			else
			{
				sprintf(szDelayTime, "%ds", second);
			}

			sprintf(szSyncedDelaySeconds, "(%s delay)", szDelayTime);
			}
		}

		getHostnameByIp(pStorage->ip_addr, szHostname, sizeof(szHostname));
		if (*szHostname != '\0')
		{
			sprintf(szHostnamePrompt, " (%s)", szHostname);
		}
		else
		{
			*szHostnamePrompt = '\0';
		}

		if (pStorage->up_time != 0)
		{
			formatDatetime(pStorage->up_time, \
				"%Y-%m-%d %H:%M:%S", \
				szUpTime, sizeof(szUpTime));
		}
		else
		{
			*szUpTime = '\0';
		}

		sprintf(output_str, "%s\tStorage %d:\n" \
			"\t\tid = %s\n" \
			"\t\tip_addr = %s%s  %s\n" \
			"\t\thttp domain = %s\n" \
			"\t\tversion = %s\n" \
			"\t\tjoin time = %s\n" \
			"\t\tup time = %s\n" \
			"\t\ttotal storage = %d MB\n" \
			"\t\tfree storage = %d MB\n" \
			"\t\tupload priority = %d\n" \
			"\t\tstore_path_count = %d\n" \
			"\t\tsubdir_count_per_path = %d\n" \
			"\t\tstorage_port = %d\n" \
			"\t\tstorage_http_port = %d\n" \
			"\t\tcurrent_write_path = %d\n" \
			"\t\tsource storage id= %s\n" \
			"\t\tif_trunk_server= %d\n" \
			"\t\ttotal_upload_count = "INT64_PRINTF_FORMAT"\n"   \
			"\t\tsuccess_upload_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_append_count = "INT64_PRINTF_FORMAT"\n"   \
			"\t\tsuccess_append_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_modify_count = "INT64_PRINTF_FORMAT"\n"   \
			"\t\tsuccess_modify_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_truncate_count = "INT64_PRINTF_FORMAT"\n"   \
			"\t\tsuccess_truncate_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_set_meta_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_set_meta_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_delete_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_delete_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_download_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_download_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_get_meta_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_get_meta_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_create_link_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_create_link_count = "INT64_PRINTF_FORMAT"\n"\
			"\t\ttotal_delete_link_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_delete_link_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_upload_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_upload_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_append_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_append_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_modify_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_modify_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\tstotal_download_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_download_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_sync_in_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_sync_in_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_sync_out_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_sync_out_bytes = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_file_open_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_file_open_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_file_read_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_file_read_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\ttotal_file_write_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\tsuccess_file_write_count = "INT64_PRINTF_FORMAT"\n" \
			"\t\tlast_heart_beat_time = %s\n" \
			"\t\tlast_source_update = %s\n" \
			"\t\tlast_sync_update = %s\n"   \
			"\t\tlast_synced_timestamp = %s %s\n",  \
			output_str,\
			++k, pStorage->id, pStorage->ip_addr, \
			szHostnamePrompt, get_storage_status_caption( \
			    pStorage->status), pStorage->domain_name, \
			pStorage->version,  \
			formatDatetime(pStorage->join_time, \
				"%Y-%m-%d %H:%M:%S", \
				szJoinTime, sizeof(szJoinTime)), \
			szUpTime, pStorage->total_mb, \
			pStorage->free_mb,  \
			pStorage->upload_priority,  \
			pStorage->store_path_count,  \
			pStorage->subdir_count_per_path,  \
			pStorage->storage_port,  \
			pStorage->storage_http_port,  \
			pStorage->current_write_path,  \
			pStorage->src_id,  \
			pStorage->if_trunk_server,  \
			pStorageStat->total_upload_count, \
			pStorageStat->success_upload_count, \
			pStorageStat->total_append_count, \
			pStorageStat->success_append_count, \
			pStorageStat->total_modify_count, \
			pStorageStat->success_modify_count, \
			pStorageStat->total_truncate_count, \
			pStorageStat->success_truncate_count, \
			pStorageStat->total_set_meta_count, \
			pStorageStat->success_set_meta_count, \
			pStorageStat->total_delete_count, \
			pStorageStat->success_delete_count, \
			pStorageStat->total_download_count, \
			pStorageStat->success_download_count, \
			pStorageStat->total_get_meta_count, \
			pStorageStat->success_get_meta_count, \
			pStorageStat->total_create_link_count, \
			pStorageStat->success_create_link_count, \
			pStorageStat->total_delete_link_count, \
			pStorageStat->success_delete_link_count, \
			pStorageStat->total_upload_bytes, \
			pStorageStat->success_upload_bytes, \
			pStorageStat->total_append_bytes, \
			pStorageStat->success_append_bytes, \
			pStorageStat->total_modify_bytes, \
			pStorageStat->success_modify_bytes, \
			pStorageStat->total_download_bytes, \
			pStorageStat->success_download_bytes, \
			pStorageStat->total_sync_in_bytes, \
			pStorageStat->success_sync_in_bytes, \
			pStorageStat->total_sync_out_bytes, \
			pStorageStat->success_sync_out_bytes, \
			pStorageStat->total_file_open_count, \
			pStorageStat->success_file_open_count, \
			pStorageStat->total_file_read_count, \
			pStorageStat->success_file_read_count, \
			pStorageStat->total_file_write_count, \
			pStorageStat->success_file_write_count, \
			formatDatetime(pStorageStat->last_heart_beat_time, \
				"%Y-%m-%d %H:%M:%S", \
				szLastHeartBeatTime, sizeof(szLastHeartBeatTime)), \
			formatDatetime(pStorageStat->last_source_update, \
				"%Y-%m-%d %H:%M:%S", \
				szSrcUpdTime, sizeof(szSrcUpdTime)), \
			formatDatetime(pStorageStat->last_sync_update, \
				"%Y-%m-%d %H:%M:%S", \
				szSyncUpdTime, sizeof(szSyncUpdTime)), \
			formatDatetime(pStorageStat->last_synced_timestamp, \
				"%Y-%m-%d %H:%M:%S", \
				szSyncedTimestamp, sizeof(szSyncedTimestamp)),\
			szSyncedDelaySeconds);
	}

	return 0;
}

int tracker_list_groups(ConnectionInfo *pTrackerServer, \
		FDFSGroupStat *group_stats, const int max_groups, \
		int *group_count)
{
	bool new_connection;
	TrackerHeader header;
	TrackerGroupStat stats[FDFS_MAX_GROUPS];
	char *pInBuff;
	ConnectionInfo *conn;
	TrackerGroupStat *pSrc;
	TrackerGroupStat *pEnd;
	FDFSGroupStat *pDest;
	int result;
	int64_t in_bytes;

	CHECK_CONNECTION(pTrackerServer, conn, result, new_connection);

	memset(&header, 0, sizeof(header));
	header.cmd = TRACKER_PROTO_CMD_SERVER_LIST_ALL_GROUPS;
	header.status = 0;
	if ((result=tcpsenddata_nb(conn->sock, &header, \
			sizeof(header), g_fdfs_network_timeout)) != 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"send data to tracker server %s:%d fail, " \
			"errno: %d, error info: %s", __LINE__, \
			pTrackerServer->ip_addr, \
			pTrackerServer->port, \
			result, STRERROR(result));
	}
	else
	{
		pInBuff = (char *)stats;
		result = fdfs_recv_response(conn, \
			&pInBuff, sizeof(stats), &in_bytes);
	}

	if (new_connection)
	{
		tracker_disconnect_server_ex(conn, result != 0);
	}

	if (result != 0)
	{
		*group_count = 0;
		return result;
	}

	if (in_bytes % sizeof(TrackerGroupStat) != 0)
	{
		logError("file: "__FILE__", line: %d, " \
			"tracker server %s:%d response data " \
			"length: "INT64_PRINTF_FORMAT" is invalid", \
			__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, in_bytes);
		*group_count = 0;
		return EINVAL;
	}

	*group_count = in_bytes / sizeof(TrackerGroupStat);
	if (*group_count > max_groups)
	{
		logError("file: "__FILE__", line: %d, " \
			"tracker server %s:%d insufficent space, " \
			"max group count: %d, expect count: %d", \
			__LINE__, pTrackerServer->ip_addr, \
			pTrackerServer->port, max_groups, *group_count);
		*group_count = 0;
		return ENOSPC;
	}

	memset(group_stats, 0, sizeof(FDFSGroupStat) * max_groups);
	pDest = group_stats;
	pEnd = stats + (*group_count);
	for (pSrc=stats; pSrc<pEnd; pSrc++)
	{
		memcpy(pDest->group_name, pSrc->group_name, \
				FDFS_GROUP_NAME_MAX_LEN);
		pDest->total_mb = buff2long(pSrc->sz_total_mb);
		pDest->free_mb = buff2long(pSrc->sz_free_mb);
		pDest->trunk_free_mb = buff2long(pSrc->sz_trunk_free_mb);
		pDest->count= buff2long(pSrc->sz_count);
		pDest->storage_port= buff2long(pSrc->sz_storage_port);
		pDest->storage_http_port= buff2long(pSrc->sz_storage_http_port);
		pDest->active_count = buff2long(pSrc->sz_active_count);
		pDest->current_write_server = buff2long( \
				pSrc->sz_current_write_server);
		pDest->store_path_count = buff2long( \
				pSrc->sz_store_path_count);
		pDest->subdir_count_per_path = buff2long( \
				pSrc->sz_subdir_count_per_path);
		pDest->current_trunk_file_id = buff2long( \
				pSrc->sz_current_trunk_file_id);

		pDest++;
	}

	return 0;
}

static int list_all_groups(const char *group_name,char *output_str)
{
	int result;
	int group_count;
	FDFSGroupStat group_stats[FDFS_MAX_GROUPS];
	FDFSGroupStat *pGroupStat;
	FDFSGroupStat *pGroupEnd;
	int i;

	result = tracker_list_groups(pTrackerServer, \
		group_stats, FDFS_MAX_GROUPS, \
		&group_count);
	if (result != 0)
	{
		tracker_close_all_connections();
		fdfs_client_destroy();
		return result;
	}

	pGroupEnd = group_stats + group_count;
	if (group_name == NULL)
	{
		sprintf(output_str, "%sgroup count: %d\n", output_str, group_count);
		i = 0;
		for (pGroupStat=group_stats; pGroupStat<pGroupEnd; \
			pGroupStat++)
		{
			sprintf(output_str, "%s\nGroup %d:\n",output_str, ++i);
			list_storages(pGroupStat,output_str);
			break;
		}
	}
	else
	{
		for (pGroupStat=group_stats; pGroupStat<pGroupEnd; \
			pGroupStat++)
		{
			if (strcmp(pGroupStat->group_name, group_name) == 0)
			{
				list_storages(pGroupStat,output_str);
				break;
			}
		}
	}

	return 0;
}

#ifdef USE_LEVEL_DB
#include <leveldb/c.h>
static int save_db(char* key,char* value)
{
	char *err = NULL;
	leveldb_t *db = NULL;
	leveldb_options_t *options = NULL;
    leveldb_writeoptions_t* woptions = NULL;

	options = leveldb_options_create();

	leveldb_options_set_create_if_missing(options,1);

	//leveldb_options_set_error_if_exists(options,1);

	woptions = leveldb_writeoptions_create();

	db = leveldb_open(options,"ldb_fdfs_monitor",&err);

	if(err != NULL){
		logError("%s",err);
		leveldb_free(err);
		goto ERROR;
	}

	leveldb_put(db, woptions, key, strlen(key), value, strlen(value), &err);

	if(err != NULL){
		logError("%s",err);
		leveldb_free(err);
		goto ERROR;
	}

	leveldb_writeoptions_destroy(woptions);
	leveldb_options_destroy(options);
	leveldb_close(db);
	return 0;

ERROR:
	if(woptions != NULL)
		leveldb_writeoptions_destroy(woptions);
	if(options != NULL)
		leveldb_options_destroy(options);
	if(db != NULL)
		leveldb_close(db);
	return -1;
}
#endif

#ifdef USE_MYSQL
#include <mysql.h>

/*
drop table raw_data;
create table raw_data(
id bigint AUTO_INCREMENT,
datatime VARCHAR(32) not null,
value VARCHAR(409600) not null,
PRIMARY KEY(id)
)DEFAULT charset=utf8;
drop table user;
create table user(
username varchar(16) primary key,
password varchar(16) not null
);
drop table storage;
create table storage(
id bigint AUTO_INCREMENT primary key,
time varchar(32)not null,
groupId int not null,
serverId int not null,
ip varchar(256),
total int,
free int,
threshold int
);
drop table groupStorage;
create table groupStorage(
groupId int primary key,
groupThreshold int
);
*/

static int save_mysql(MYSQL *db,char *query_str)
{
	int query_error;
	MYSQL_RES *query_result = NULL;
	
	query_error = mysql_query(db, query_str);

	if(query_error != 0){
		logError(mysql_error(db));
		goto ERROR;
	}

	query_result = mysql_store_result(db);

	if(query_result == NULL)
		return 0;
	MYSQL_FIELD *fileds;
	MYSQL_ROW row;
	unsigned int i, num_fields;

	num_fields = mysql_num_fields(query_result);
	fileds = mysql_fetch_fields(query_result);

	while((row = mysql_fetch_row(query_result)) != NULL){
		for(i = 0;i != num_fields;++i){
			logError("%s \t", row[i]);
		}
	}

	return 0;
ERROR:
	return -1;
}

static int save_db(char* key,char* value)
{
	MYSQL *db = NULL,mysql;

	mysql_init(&mysql);
	db = mysql_real_connect(&mysql, DB_SERVER, DB_USER, DB_PWD, DB_NAME,0,0,0);

	if(db == NULL){
		logError(mysql_error(&mysql));
		goto ERROR;
	}

	char query_string[4096 * 100];
	memset(query_string, 0, 4096 * 100);
	snprintf(query_string, 4096 * 100, "INSERT INTO raw_data(datatime,value)VALUES('%s','%s')" ,key ,value);
	logDebug("%s",query_string);
	if(save_mysql(db,query_string) != 0){
		goto ERROR;
	}
	
	char *line_start = value;
	char *line_end = NULL;
	char *equal_sign = NULL;
	int group_id = 1,prev_group = 1;
	int storage_id = 1,prev_storage = 1;
	char *ip = NULL;
	int total = 0;
	int free = 0;

	while(*line_start != 0)
	{
		line_end = NULL;
		equal_sign = NULL;

		line_end = strchr(line_start, '\n');

		if(strncmp(line_start, "Group ",strlen("Group")) == 0){
			prev_group = group_id;
			group_id = strtol(line_start + 6,NULL,10);
		}
		if(strncmp(line_start, "\tStorage ",strlen("\tStorage ")) == 0){
			prev_storage = storage_id;
			storage_id = strtol(line_start + 9,NULL,10);
			if(!(group_id == 1 && storage_id == 1)){
				snprintf(query_string, 4096 * 100, "INSERT INTO storage(time,groupId,serverId,ip,total,free)VALUES('%s',%d,%d,'%s',%d,%d)" ,key , prev_group, prev_storage, ip, total, free);
				logInfo("%s",query_string);
				save_mysql(db,query_string);
			}
		}
		if(strncmp(line_start, "\t\tip_addr ",strlen("\t\tip_addr ")) == 0){
			ip = line_start + 12;
			*line_end = '\0';
		}
		if(strncmp(line_start, "\t\ttotal storage ",strlen("\t\ttotal storage ")) == 0){
			total = strtol(line_start + 18,NULL,10);
		}
		if(strncmp(line_start, "\t\tfree storage ",strlen("\t\tfree storage ")) == 0){
			free = strtol(line_start + 17,NULL,10);
		}
		line_start = line_end + 1;
	}
	snprintf(query_string, 4096 * 100, "INSERT INTO storage(time,groupId,serverId,ip,total,free)VALUES('%s',%d,%d,'%s',%d,%d)" ,key , group_id, storage_id, ip, total, free);
	logInfo("%s",query_string);
	save_mysql(db,query_string);
	
	mysql_close(db);
	return 0;
ERROR:
	if(db != NULL)
		mysql_close(db);
	return -1;
}
#endif

static void* save(time_t job_time,void *arg) {
	char output_str[4096*100];
	char key[20];
	int result;
	memset(output_str,0,4096*100);
	memset(key,0,20);

	timetostr(&job_time,key);
	logInfo("%s\n",key);
	if ((result=fdfs_client_init("/etc/fdfs/client.conf")) != 0)
	{
		return NULL;
	}
	if (g_tracker_group.server_count > 1)
	{
		srand(time(NULL));
		rand();  //discard the first
		g_tracker_group.server_index = (int)( \
			(g_tracker_group.server_count * (double)rand()) \
			/ (double)RAND_MAX);
	}

	sprintf(output_str,"%sserver_count=%d, server_index=%d\n", output_str, g_tracker_group.server_count, g_tracker_group.server_index);

	pTrackerServer = tracker_get_connection();
	if (pTrackerServer == NULL)
	{
		fdfs_client_destroy();
		return NULL;
	}
	sprintf(output_str,"%s\ntracker server is %s:%d\n\n", output_str, pTrackerServer->ip_addr, pTrackerServer->port);
	
	list_all_groups(NULL,output_str);

	tracker_disconnect_server_ex(pTrackerServer, true);
	fdfs_client_destroy();
	
	if(save_db(key,output_str) != 0){
		return NULL;
	}
	return NULL;
}

static void daemonize()
{
	int fd0, fd1, fd2;
	pid_t pid;
	struct rlimit rlimit;


	if(getrlimit(RLIMIT_NOFILE, &rlimit) < 0){
		fprintf(stderr, "can not get file limit.\n");
		exit(1);
	}

	if ((pid = fork()) < 0){
		fprintf(stderr, "can not fork.\n");
		exit(1);
	} else if (pid != 0){  /* parent process */
		exit(0);
	}
	setsid();
	if((pid = fork()) != 0) 
		exit(0);
	else if(pid < 0) 
		exit(1);

	/*      
	 * if (chdir("/") < 0){
	 *      fprintf(stderr, ": can not change directory to /\n");
	 *      exit(1);
	 * }
	 */

	if (rlimit.rlim_max == RLIM_INFINITY)
		rlimit.rlim_max = 1024;
	int i;
	for (i = 0; i < rlimit.rlim_max; i ++){
		close(i);
	}

	umask(0);

	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);
	if (fd0 != 0 || fd1 != 1 || fd2 != 2){
		fprintf(stderr, "unexpected file descriptors after daemonizing %d %d %d\n", fd0, fd1, fd2);
		exit(1);
	}
	fprintf(stderr, "start.\n");
}

int main()
{
	daemonize();
	log_init();
	g_log_context.log_level = LOG_INFO;
	
	config("fdfs_jobs_monitor.config", g_config_list, sizeof(g_config_list)/sizeof(struct configItem));

	g_log_fd = open(LOG_NAME,O_WRONLY | O_APPEND | O_CREAT);
	if(g_log_fd == 0){
		logError("can't open log file %s",LOG_NAME);
		return 1;
	}
	g_log_context.log_fd = g_log_fd;

	struct job job;
	job_service(&job);
	job.call = save;
	sleep(1000000000);
	job_destory(&job);
	return 0;
}
