#!/usr/bin/env python
import py_fdfs_monitor
import time
import string
import re
from datetime import datetime
from wsgiref.simple_server import make_server
from apscheduler.schedulers.background import BackgroundScheduler
import logging
import logging.handlers

LOG_FILE = 'fdfs_monitor.log'

URL_PATTERNS= (
    ('/','list_now'),
    ('/save_test','save_test'),
    ('/load_test','load_test'),
	('list_all','list_all'),
    )
class Dispatcher(object):
    def _match(self,path):
        path = path.split('/')[1]
        for url,app in URL_PATTERNS:
            if path in url:
                return app
    def __call__(self,environ, start_response):
        path = environ.get('PATH_INFO','/')
        app = self._match(path)
        if app :
            app = globals()[app]
            return app(environ, start_response)
        else:
            start_response("404 NOT FOUND",[('Content-type', 'text/html')])
            return ["<h1>Page dose not exists!</h1>"]
def list_now(environ, start_response):
    str = py_fdfs_monitor.list()
    start_response("200 OK",[('Content-type', 'text/html')])
    return ["<html><pre>%s</pre><html>" % str]
def save_test(environ, start_response):
    str = py_fdfs_monitor.save('test')
    start_response("200 OK",[('Content-type', 'text/html')])
    return ["<html><pre>%s</pre><html>" % str]
def load_test(environ, start_response):
    str = py_fdfs_monitor.load('test')
    start_response("200 OK",[('Content-type', 'text/html')])
    return ["<html><pre>%s</pre><html>" % str]
def list_all(environ, start_response):
	key = string.atoi(py_fdfs_monitor.get_first_key())
	now_time = time.time()
	str = ''
	while key < now_time:
		key_str = "%d" %key
		temp = time.localtime(key)
		str += time.strftime('%Y-%m-%d %H:%M:%S', temp)
		str += '\t'
		value_str = py_fdfs_monitor.load(key_str)
		total_space = re.findall(r'disk total space = .*MB',value_str)
		free_space = re.findall(r'disk free space = .*MB',value_str)
		if (len(total_space) != 0) :
			str += total_space[0]
			str += '\t'
		else :
			str += 'disk total space = data miss'
			str += '\t'
		if (len(free_space) != 0) :
			str += free_space[0]
			str += '\t'
		else :
			str += 'disk free space = data miss'
			str += '\t'
		str += '\n'
		key = key + 60
	start_response("200 OK",[('Content-type', 'text/html')])
	return ["<html><pre>%s</pre><html>" % str]
app = Dispatcher()
httpd = make_server('', 8000, app)

#logger
handler = logging.handlers.RotatingFileHandler(LOG_FILE, maxBytes = 1024*1024, backupCount = 5)
fmt = '%(asctime)s - %(levelname)s - %(filename)s:%(lineno)s - %(name)s - %(message)s'
formatter = logging.Formatter(fmt)
handler.setFormatter(formatter)

sched_logger = logging.getLogger('apscheduler.executors.default')
sched_logger.addHandler(handler)
sched_logger.setLevel(logging.INFO)

main_logger = logging.getLogger('main')
main_logger.addHandler(handler)
main_logger.setLevel(logging.INFO)

sched = BackgroundScheduler()
#sched.daemonic = False 
def do_monitor_save_job():
	time_key = int(time.time())
	time_key_str = "%d" %time_key
	str = py_fdfs_monitor.save(time_key_str)
	if(str != 'save failed!'):
		main_logger.info('save success at ' + time_key_str)
	else:
		main_logger.error('save failed at ' + time_key_str)
sched.add_job(do_monitor_save_job,'cron',second = 0)
sched.start()
#print py_fdfs_monitor.get_first_key()
#str = py_fdfs_monitor.get_first_key()
#print list(str)
print "Serving on port 8000..."
httpd.serve_forever()
