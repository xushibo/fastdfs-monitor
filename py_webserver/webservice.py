#!/usr/bin/env python
import py_fdfs_monitor
import time
import string
import re
from datetime import datetime
from wsgiref.simple_server import make_server

LOG_FILE = 'fdfs_monitor.log'

URL_PATTERNS= (
    ('/','list_now'),
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
def datetime_timestamp(dt):
	time.strptime(dt, '%Y-%m-%d %H:%M:%S')
	s = time.mktime(time.strptime(dt, '%Y-%m-%d %H:%M:%S'))
	return int(s)
def timestamp_datetime(value):
	format = '%Y-%m-%d %H:%M:%S'
	value = time.localtime(value)
	dt = time.strftime(format, value)
	return dt
def list_now(environ, start_response):
    str = py_fdfs_monitor.list()
    start_response("200 OK",[('Content-type', 'text/html')])
    return ["<html><pre>%s</pre><html>" % str]
def list_all(environ, start_response):
	key_str = py_fdfs_monitor.get_first_key()
	key = datetime_timestamp(key_str)
	now_time = time.time()
	str = ''
	while key < now_time:
		str += key_str
		str += '\n'
		value_str = py_fdfs_monitor.load(key_str)
		tracker_server = re.findall(r'tracker server is .*\n',value_str)
		group_name = re.findall(r'Group .*:',value_str)
		storage_name = re.findall(r'Storage .*:',value_str)
		total_space = re.findall(r'disk total space = .*MB\n',value_str)
		free_space = re.findall(r'disk free space = .*MB\n',value_str)
		group_index = 0
		storage_index = 0
		while group_index < len(group_name):
			str += group_name[group_index]
			str += '\n'
			index = 0
			while index < len(storage_name):
				str += storage_name[storage_index]
				str += '\n'
				str += total_space[storage_index]
				str += free_space[storage_index]
				index += 1
				storage_index += 1
			group_index += 1
		str += '\n'
		key = key + 60
		key_str = timestamp_datetime(key)
	start_response("200 OK",[('Content-type', 'text/html')])
	return ["<html><pre>%s</pre><html>" % str]
app = Dispatcher()
httpd = make_server('', 8000, app)

print "Serving on port 8000..."
httpd.serve_forever()
