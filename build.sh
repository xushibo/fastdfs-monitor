gcc -Wall -DUSE_MYSQL -D_XOPEN_SOURCE -lpthread -I mysql_headers -I /usr/include/python2.6 -I fdfs_headers libfdfsclient.so.1 libmysqlclient.so.16 -lleveldb \
	-g -O -DDEBUG_FLAG -DOS_LINUX -DIOEVENT_USE_EPOLL -o fdfs_jobs_monitor jobs.c main.c
