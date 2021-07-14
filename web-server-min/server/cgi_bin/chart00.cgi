#!/usr/bin/env python


#http://170.75.170.11/calcops_28.cgi?idBeg=200000&span=2016&idEnd=360000&mime_type=csv
#http://localhost:8111/server/cgi_bin/chart00.cgi?format=png
#http://localhost:8111/server/cgi_bin/chart00.cgi?format=svg
#http://localhost:8111/server/cgi_bin/chart00.cgi?format=pdf
#http://localhost:8111/server/cgi_bin/chart00.cgi
#--

import re

url_pattern = re.compile( '^((.*)\?idA=([0-9]+)&idB=([0-9]+))', re.IGNORECASE)

import sys, os
import cgi, cgitb
import traceback
import httplib, urllib, urlparse
#import pdb

cgitb.enable( display=0, format='text', logdir='/tmp' )

def cgiHandler ():
    """cgiHandler used to create a CGI endpoint."""
    try:
        params = {}

        #pdb.set_trace()

        request_method = os.environ["REQUEST_METHOD"]
        accepts = ""
        if "CONTENT_TYPE" in os.environ:
            accepts = os.environ['CONTENT_TYPE']
        elif "HTTP_ACCEPT" in os.environ:
            accepts = os.environ['HTTP_ACCEPT']

        post_data = None
        if request_method != "GET" and request_method != "DELETE":
            post_data = sys.stdin.read()

        input = cgi.FieldStorage()
        try:
            for key in input.keys():
                params[key] = input[key].value
        except TypeError:
            pass

        path_info = host = ""

        if "PATH_INFO" in os.environ:
            path_info = os.environ["PATH_INFO"]

        if "HTTP_X_FORWARDED_HOST" in os.environ:
            host      = "http://" + os.environ["HTTP_X_FORWARDED_HOST"]
        elif "HTTP_HOST" in os.environ:
            host      = "http://" + os.environ["HTTP_HOST"]

        host += os.environ["SCRIPT_NAME"]

        if ( cmp(request_method,"POST") == 0):
            ## special case for POST
            inPostData = post_data.split("|")  # TODO json
            params['idA'] = inPostData[0]
            params['idB'] = inPostData[1]
            post_data_body = inPostData[2]
            foundIdA = True
            foundIdB = True
        else:
            post_data_body = post_data
            foundIdA = False
            foundIdB = False
            for tKey in params.keys():
                if ( cmp( "idA", tKey) == 0):
                    foundIdA = True
                if ( cmp( "idB", tKey) == 0):
                    foundIdB = True
                #print >>sys.stderr, "%s" % tKey
                #print >>sys.stderr, "  %s" % params[tKey]


        if not foundIdA or not foundIdB:
            print "Content-type: text/plain\n"
            print "Status: 500 CGI failed\n"
            print "Usage: oetool.cgi?idA=000&idB=0000\n"
            print "RequestMethod: %s" % request_method
            print "Path_Info: %s" % path_info
            print "Host: %s" % host
            print "params[] length= %d" % len(params)
            print "params=> %s" % str(params)
            #for tKey in params:
            #    print "Found: %s = %s" % tKey, params[tKey]
            sys.exit()

        ##----------------------------------------------------------------------
        ##  Now do the interaction with the server
        headers_dict = { "User-Agent:" : "oetool.cgi" }

        tData = 'OKbyME'
        print "Content-type: text/plain\n"
        print tData


    except Exception, E:
        print "Cache-Control: max-age=10, must-revalidate" # make the client reload
        print "Content-type: text/plain\n"
        error = "An error occurred: %s\n%s\n" % (
            str(E),
            "".join(traceback.format_tb(sys.exc_traceback)))
        print error
        print >>sys.stderr, error

if __name__ == '__main__':
    cgiHandler()




