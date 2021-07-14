#!/usr/bin/env python

#http://localhost:8111/server/cgi_bin/oetool_form.cgi?idA=68&idB=555

#--

import os, sys
import json, re, random

try:
  idBeg = int(sys.argv[1])
  span = int(sys.argv[2])
  idEnd = int(sys.argv[3])
except:
  print( 'FAIL:',sys.argv)
  sys.exit(-1)

import psycopg2 as pg
conn  = pg.connect("dbname=btc_hist")
curs = conn.cursor()

tSQL = '''  COPY (
SELECT calcops_28( a.blkheight, a.median_time, a.chain_reward, a.chainwork_hex,
                   b.blkheight, b.median_time, b.chain_reward, b.chainwork_hex),
                   b.blkheight, b.median_time
FROM data_chain as a, data_chain as b  WHERE
  a.blkheight > %s AND a.blkheight %% %s = 0  AND  a.blkheight < %s AND
  b.blkheight = (a.blkheight+%s)  AND b.median_time <> a.median_time)
to '/tmp/res283.csv' with CSV header
;
'''
try:
    res = curs.execute(tSQL,(idBeg,span,idEnd,span))
    #res = curs.fetchall()
    #print('SQL:',res)
except:
    print('FAIL SQL',tSQL)

tFile = open('/tmp/res283.csv','r')
buf = tFile.read()
#  'Content-Type: text/css; charset=UTF-8'
print(buf)

##  args a,b   where a is less than b and b is less than TIP
## [(372960, 1441354718L, 1460857994284830L, '0x9dab25c45676a0f76fffd'),
##  (374976, 1442516783L, 1465935896374343L, '0xa43227a48d21c842843ce') ]


