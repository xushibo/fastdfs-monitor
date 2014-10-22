#for mysql and leveldb
#gcc -Wall -std=c99 -DUSE_MYSQL -D_XOPEN_SOURCE -lpthread -I mysql_headers -I /usr/include/python2.6 -I fdfs_headers libfdfsclient.so.1 libmysqlclient.so.16 libfastcommon.so -lleveldb \
#	-g -O -DDEBUG_FLAG -DOS_LINUX -DIOEVENT_USE_EPOLL -o fdfs_jobs_monitor jobs.c config.c main.c

#for mysql
gcc -g -Wall -std=c99 -DUSE_MYSQL -D_XOPEN_SOURCE -lpthread -I mysql_headers -I fdfs_headers -lfdfsclient -lmysqlclient -lfastcommon\
	-O -DDEBUG_FLAG -DOS_LINUX -DIOEVENT_USE_EPOLL -o fdfs_jobs_monitor jobs.c config.c main.c
