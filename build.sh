gcc -Wall -D_XOPEN_SOURCE -lpthread -I /usr/include/python2.6 -I headers libfdfsclient.so.1 -lleveldb \
	-g -O -DDEBUG_FLAG -DOS_LINUX -DIOEVENT_USE_EPOLL -o fdfs_monitor jobs.c main.c
