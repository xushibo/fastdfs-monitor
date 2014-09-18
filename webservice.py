#!/usr/bin/env python
import py_fdfs_monitor
from wsgiref.simple_server import make_server
URL_PATTERNS= (
    ('/','list'),
    ('/save','save'),
    ('/load','load'),
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
def list(environ, start_response):
    str = py_fdfs_monitor.list()
    start_response("200 OK",[('Content-type', 'text/html')])
    return ["<html><pre>%s</pre><html>" % str]
def save(environ, start_response):
    str = py_fdfs_monitor.save('123')
    start_response("200 OK",[('Content-type', 'text/html')])
    return ["<html><pre>%s</pre><html>" % str]
def load(environ, start_response):
    str = py_fdfs_monitor.load('123')
    start_response("200 OK",[('Content-type', 'text/html')])
    return ["<html><pre>%s</pre><html>" % str]
app = Dispatcher()
httpd = make_server('', 8000, app)
print "Serving on port 8000..."
httpd.serve_forever()
