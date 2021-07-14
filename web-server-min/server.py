#!/usr/bin/python

# Minimal HTTP server
#
# Nov 2020
# Apr 2021  -dbb  small changes for CGI handling

import sys, os, time
from datetime import datetime

import re

import select
import socket
import threading
import mimetypes
import subprocess

gPy =  sys.version[0]

try:
    from urllib import urlopen
    import urlparse
    from urllib import unquote
except ImportError:
    from urlparse import urlparse
    from urllib import urlopen
    from urllib.parse import unquote

class HTTP_server:
    host =  '172.16.0.3' #'192.168.1.127'  #'127.0.0.1' #'172.16.0.3'  # '170.75.170.11' #'127.0.0.1'
    port = 8111
    buf_size = 1024
    max_conn = 32
    abs_path = 'http://' + str(host) + ':' + str(port)
    #url_pattern = re.compile('^((\/\w+\.?\w*.*)+)\??((\w+=\w+[&]?)*)', re.IGNORECASE)
    url_pattern = re.compile( '^((.*)\?idA=([0-9]+)&idB=([0-9]+))', re.IGNORECASE)
    #url_pattern = re.compile('htt.*\://(.*?)\/(.*?)\?((\w+=\w+[&]?)*)', re.IGNORECASE)
    # constructor defines which type of connection we use

    def __init__(self, threaded=False):

        # init a server socket
        #  bind it to a port; listen for incoming connections
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.bind((self.host, self.port))

        if threaded:
            self.s.listen(self.max_conn)
        else:
            self.s.listen(1)

        while True:
            threading.Thread( target=self.handle_client_conn, args=(self.s.accept())).start()

    # handle_client_conn():
    # Accepts an incoming client connection and parse the input data stream into a HTTP request
    # Read from socket until input is exhausted

    def handle_client_conn(self, connection, address):

        connection.setblocking(False)
        inputs, outputs, errors = select.select([connection], [], [])

        if inputs:
            request = ''
            while True:
                try:
                    request += connection.recv(self.buf_size).decode()
                except Exception as exp:
                    print(request)
                    break

            response = self.parse_request(request)
            connection.sendall(response)
            connection.close()

    # Check request type

    def parse_request(self, request):
        fields = request.split('\r\n')[0].split(' ')
        if fields[0] == 'GET':
            resource = fields[1]
            protocol = fields[2]
            response = self.handle_GET(unquote(resource), protocol)
        else:
            response = 'HTTP/1.0 501 Not Implemented\r\n'

        return response

    # handle CGI scripts

    def call_cgi(self, cgi, args=[], inputs=None):
        serv_env = {}
        serv_env['SERVER_SOFTWARE'] = 'web-server-min/0a0'
        serv_env['REQUEST_METHOD'] = 'GET'
        serv_env['SERVER_PROTOCOL'] = 'HTTP/1.1'
        serv_env['SERVER_NAME'] = 'a-tmp-server-name'
        serv_env['SERVER_PORT'] = '8111'
        serv_env['REQUEST_SCHEME'] = 'http'

        self.write_log( ' CALL_CGI:' +str(cgi) +'; '+str(args)+'; '+str(inputs))
        cgi_proc = subprocess.Popen( [cgi]+args, env=serv_env, stdout=subprocess.PIPE, stdin=subprocess.PIPE)
        if inputs:
            output = cgi_proc.communicate(input=inputs)[0]
        else:
            output = cgi_proc.communicate()[0]

        return output

    # If index.html exists: open; if not: traverse directory
    #   path exists?
    #   If not: page not found; if yes: need to know if it's a file or dir
    #   If file: display it; if directory: check if index.html exists

    def handle_GET(self, resource, protocol):

        parse_obj = urlparse.urlparse( resource)
        self.write_log( str(parse_obj))

        components = None
        arg_string = None
        if ( len(resource) > 2 and resource.find('?') != -1 ):
            self.write_log( ' PATH RESOURCE:'+str(resource))
            components = self.url_pattern.match(resource)

        if components:
            grps = components.groups()
            self.write_log( ' PATH grps')
            self.write_log( components.group(2))
            self.write_log( str(grps))
            path = '.'+components.group(2)
            arg_string = components.group(3)
        else:
            self.write_log( ' PATH PATH')
            path = './'

        if resource == '/favicon.ico':
            return ''.encode()

        response_line = protocol + ' 200 OK\r\n'
        content_line = 'Content-Type: '
        content_type = 'text/plain'
        blank_line = '\r\n'

        # if path has a query (in our case, form submission),
        # then parse parameters passed

        if '?' in path:
            # parse query? args into a python dict
            parsed = urlparse.parse_qs(urlparse.urlparse(resource).query)
            body = self.query_response(parsed).encode()
        elif not os.path.exists(path):
            body = b'HTTP/1.0 404 Not Found\r\n'

        if True:
           self.write_log( ' RESOURCE:' +str(resource))
           if path[-4:] == '.cgi':
                if arg_string:
                    #args = arg_string.replace('&',' ').replace('=',' ').split(' ')
                    self.write_log(' PATH arg_string')
                    args = [ components.group(3), components.group(4) ]
                    body = self.call_cgi(path, args)
                else:
                    self.write_log(' PATH arg_string=None')
                    body = self.call_cgi(path)

           elif os.path.isdir(path):
                index = path + 'index.html'
                if os.path.exists(index):
                    with open(index, 'rb') as f:
                        body = f.read()
                    f.close()
                else:
                    body = self.traverse_dir('.' + resource).encode()

           else:
                content_type = mimetypes.guess_type(path)[0]
                with open(path, 'rb') as f:
                    body = f.read()
                f.close()

        #print( str(type(response_line)),str(type(content_line)),str(type(content_type)), str(type(blank_line)) )
        content_type = 'text/plain'
        header_part = response_line + content_line + content_type + blank_line + blank_line
        response = header_part.encode() + body
        return response

    # Query response
    def write_log(self, in_text):
        with open('server/log.txt', 'a') as f:
            f.write("log time:" + str(datetime.now())+' - '+in_text+'\n')

    def query_response(self, args):
        with open('server/log.txt', 'a') as f:
            for key in args:
                f.write("%s: " % key)
                for val in args[key]:
                    f.write("%s " % val)
                f.write(";")
            f.write("access time: " + str(datetime.now()) + '\n')
        f.close()

        return '<html><body> oetool ACK </body</html>'

    # Traverse current dir and display all subdirectories and files

    def traverse_dir(self, path):

        # path must end with slash or error occurs
        if path[-1] != '/':
            path = path + '/'

        # make sure we can't go past home directory for server
        # otherwise get a relative path for parent directory

        if path == './':
            parent = './'
        else:
            parent = os.path.dirname(os.path.dirname(path))

        # create html file

        begin = """
        <!DOCTYPE html>
        <html>
        <body>
        """
        header = "<h1>Index of " + path[1:] + "</h1>"

        table_start = """
        <table cols=\"3\" cellspacing=\"10\">
            <tr>
                <th>Name</th>
                <th>LastModified</th>
                <th>Size</th>
            </tr>
        """

        # create a row in a table for parent directory

        parent_row = self.create_link_col('Parent', self.abs_path + parent[1:])\
                     + self.create_modified_col(parent) + self.create_size_col(parent)

        traversed =  begin + header + table_start + parent_row

        # traverse all the files and create html table contents

        files = os.listdir(path)
        for f in files:
            fpath = path + f
            traversed = traversed + self.create_link_col(f, self.abs_path + fpath[1:])\
                     + self.create_modified_col(fpath) + self.create_size_col(fpath)

        end = """
        </table>
        </body>
        </html>
        """
        return traversed + end

    #  misc functions used for generating html content in traverse_dir
    def create_link_col(self, name, link):
        return '<tr><th align=\"left\" ><a href=\"'+ link + '\">' + name + '</a></th>'

    def create_modified_col(self, path):
        date = time.strftime("%m/%d/%Y %I:%M:%S %p", time.localtime(os.path.getmtime(path)))
        return '<th>' + date + '</th>'

    def create_size_col(self, path):
        if os.path.isfile(path):
            size = str(os.path.getsize(path)) + 'B'
        else:
            size = '-'
        return '<th align=\"right\" >' + size + '</th></tr>'


if __name__ == "__main__":

    rp = sys.version_info

    # Accept a flag '--threaded' to make the server threaded
    if len(sys.argv) > 1:
        if sys.argv[1] == '--threaded':
            http = HTTP_server(threaded=True)
    else:
        http = HTTP_server()
