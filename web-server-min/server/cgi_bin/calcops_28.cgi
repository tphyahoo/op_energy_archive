#!/usr/bin/env python

#http://localhost:8111/server/cgi_bin/oetool_form.cgi?idA=68&idB=555

#--

import os, sys
import json, re, random

try:
  idA = int(sys.argv[1])
  idB = int(sys.argv[2])
except:
  print( 'FAIL:',sys.argv)
  sys.exit(-1)

import psycopg2 as pg
conn  = pg.connect("dbname=btc_hist")
curs = conn.cursor()

tSQL = 'select blkheight,median_time,chain_reward,chainwork_hex from data_chain where blkheight in (%s,%s)'
res = curs.execute(tSQL,(idA,idB))

res = curs.fetchall()
#print('SQL:',res)

##  args a,b   where a is less than b and b is less than TIP
## [(372960, 1441354718L, 1460857994284830L, '0x9dab25c45676a0f76fffd'),
##  (374976, 1442516783L, 1465935896374343L, '0xa43227a48d21c842843ce') ]

cnt_m1_secs  = res[1][1] - res[0][1]
cnt_m2_rwd   = res[1][2] - res[0][2]
cnt_m3_wrk   = int(res[1][3],base=16) - int(res[0][3],base=16)
cnt_Tm1      = (res[1][0] - res[0][0]) * 600

#  actualSeconds = cnt_m1_secs = data[b].median_time  - data[a].median_time;
#  revenue       = cnt_m2_rwd  = data[b].chain_reward - data[a].chain_reward;
#  work            = cnt_m3_wrk  = data[b].chainwork    - data[a].chainwork;
#  expectedSeconds = cnt_Tm1  =  (b-a) * 600;

op_energy_price = ( cnt_m3_wrk * cnt_Tm1) / (cnt_m1_secs * cnt_m2_rwd )

print(op_energy_price)

