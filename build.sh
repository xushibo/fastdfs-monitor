gcc -I /usr/include/python2.6 -I headers libfdfsclient.so.1 -lleveldb py_fdfs_monitor.c \
	-fPIC -shared -Wall \
	-g -O -DDEBUG_FLAG -DOS_LINUX -DIOEVENT_USE_EPOLL -o py_fdfs_monitor.so
