#!/usr/bin/env python
import py_fdfs_monitor
from wsgiref.simple_server import make_server
URL_PATTERNS= (
    ('hello/','say_hello'),
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
def say_hello(environ, start_response):
    (str0,str1,str2) = py_fdfs_monitor.list()
    start_response("200 OK",[('Content-type', 'text/html')])
    return ["<html><pre>%s%s%s</pre><html>" % (str0,str1,str2)]
app = Dispatcher()
httpd = make_server('', 8000, app)
print "Serving on port 8000..."
httpd.serve_forever()
