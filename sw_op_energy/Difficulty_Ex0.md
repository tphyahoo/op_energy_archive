

```python
%matplotlib inline
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

import seaborn 
from datetime import datetime
```

### Demo of BTC Difficulty Adjustment
####    &nbsp;  OE Skunk Works    &nbsp;  &nbsp;       27nov20        -dbb


```python
import psycopg2 as pg
import pandas.io.sql as psql

conn  = pg.connect("dbname=btc_hist")
```

    btc_hist=# \d data_chain
           Table "public.data_chain"
        Column     |       Type       |  
    ---------------+------------------+
     height_str    | text             |
     blkhash       | text             |
     bits_hex      | text             |
     difficulty    | double precision |
     chainwork_hex | text             |
     time_str      | text             |
     totaltime_str | text             |

--

     class CBlockHeader {  public:
       // header
       int32_t   nVersion;
       uint256   hashPrevBlock;
       uint256   hashMerkleRoot;
       uint32_t  nTime;
       uint32_t  nBits;
       uint32_t  nNonce;
       


```python
tSQL = ''' SELECT
      height_str::bigint as blk_ht,
      bits_hex,
      difficulty,
      time_str::bigint as blk_time_longint,
      totaltime_str::bigint as blk_totaltime_longint
FROM
    data_chain   ORDER BY   height_str::bigint
'''
df_datachain = pd.read_sql_query( tSQL,con=conn )
```


```python
df_datachain.info()
```

    <class 'pandas.core.frame.DataFrame'>
    RangeIndex: 649600 entries, 0 to 649599
    Data columns (total 5 columns):
    blk_ht                   649600 non-null int64
    bits_hex                 649600 non-null object
    difficulty               649600 non-null float64
    blk_time_longint         649600 non-null int64
    blk_totaltime_longint    649600 non-null int64
    dtypes: float64(1), int64(3), object(1)
    memory usage: 24.8+ MB



```python
## Note:  by examination, the first adjusted BTC difficulty was 
##  the 15th 2-week interval .. so demo-calc that interval
##  the start offset is one less than a full cycle (2015)

tbase = 15
##  constant for 2-weeks of mining, defined for the network at init time
dif_interval = 6*24*14    # =>2016 ten minute blocks     

## convenience variables to mark start and end in the interval, a.k.a. Regimen
sbeg = (dif_interval * tbase) + 2015   ## skip the genesis interval in all calcs
send = sbeg + dif_interval             ## the last block with the same difficulty

##  df_datachain[ sbeg:send ]

##  a full 2016 row interval is too long to show inline, so..
##  instead, show the first four rows, 
##    then the last four ending exactly on the last blk in the regimine
df_datachain[ sbeg:(sbeg+4) ]
```




<div>
<style scoped>
    .dataframe tbody tr th:only-of-type {
        vertical-align: middle;
    }

    .dataframe tbody tr th {
        vertical-align: top;
    }

    .dataframe thead th {
        text-align: right;
    }
</style>
<table border="1" class="dataframe">
  <thead>
    <tr style="text-align: right;">
      <th></th>
      <th>blk_ht</th>
      <th>bits_hex</th>
      <th>difficulty</th>
      <th>blk_time_longint</th>
      <th>blk_totaltime_longint</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <th>32255</th>
      <td>32256</td>
      <td>0x1d00d86a</td>
      <td>1.1829</td>
      <td>1262153464</td>
      <td>1262150214</td>
    </tr>
    <tr>
      <th>32256</th>
      <td>32257</td>
      <td>0x1d00d86a</td>
      <td>1.1829</td>
      <td>1262154352</td>
      <td>1262150242</td>
    </tr>
    <tr>
      <th>32257</th>
      <td>32258</td>
      <td>0x1d00d86a</td>
      <td>1.1829</td>
      <td>1262155561</td>
      <td>1262151952</td>
    </tr>
    <tr>
      <th>32258</th>
      <td>32259</td>
      <td>0x1d00d86a</td>
      <td>1.1829</td>
      <td>1262156580</td>
      <td>1262152015</td>
    </tr>
  </tbody>
</table>
</div>




```python
df_datachain[ (send-4):(send) ]
```




<div>
<style scoped>
    .dataframe tbody tr th:only-of-type {
        vertical-align: middle;
    }

    .dataframe tbody tr th {
        vertical-align: top;
    }

    .dataframe thead th {
        text-align: right;
    }
</style>
<table border="1" class="dataframe">
  <thead>
    <tr style="text-align: right;">
      <th></th>
      <th>blk_ht</th>
      <th>bits_hex</th>
      <th>difficulty</th>
      <th>blk_time_longint</th>
      <th>blk_totaltime_longint</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <th>34267</th>
      <td>34268</td>
      <td>0x1d00d86a</td>
      <td>1.1829</td>
      <td>1263248530</td>
      <td>1263246631</td>
    </tr>
    <tr>
      <th>34268</th>
      <td>34269</td>
      <td>0x1d00d86a</td>
      <td>1.1829</td>
      <td>1263248863</td>
      <td>1263247097</td>
    </tr>
    <tr>
      <th>34269</th>
      <td>34270</td>
      <td>0x1d00d86a</td>
      <td>1.1829</td>
      <td>1263249045</td>
      <td>1263247477</td>
    </tr>
    <tr>
      <th>34270</th>
      <td>34271</td>
      <td>0x1d00d86a</td>
      <td>1.1829</td>
      <td>1263249842</td>
      <td>1263247807</td>
    </tr>
  </tbody>
</table>
</div>



###  Compute BITS Difficulty Change   &nbsp; &nbsp; method-0


```python
##
##  Method Zero recreates the C++ source code found in  pow.cpp
##   The main change from method-1 is eliminate the float multiply.
##
##  08dec20  match; seven discrepancies solved   -dbb
##


def unpack_exp(x):
    ## defined in the compact BITS format
    return 8*(x - 3)

def getCompact( inLongInt ):
    ##  inLongInt -> long integer  uint256
    ##  returns a 4 byte BITS compact representation
    #assert( str(type(inLongInt)) == "<type 'long'>" )
    
    ## get bytes needed to represent this number
    cnt = ( (inLongInt.bit_length()+7)/8  )
    
    ## get an integer from inputs most-significant three bytes 
    resInt = inLongInt >> ( (cnt*8) - (3*8) )
    #print '  resInt:  ' +hex(resInt)
    
    ## build a mask to test the high bit of the large integer input
    ##  python-specific, this must result in a whole number of bytes
    shft_cnt = inLongInt.bit_length()
    if (shft_cnt % 8)!=0:  shft_cnt += (8-(shft_cnt % 8))

    ## mask test the high bit of the input
    ##  the top bit of the third byte in compact format is RESERVED
    ##  check if the result has this bit set, if so
    ##    shift the value content right (losing precision) 
    ##    increment the total number of bytes of padding
    if (0x80 << (shft_cnt-8)) & inLongInt :
        #print 'big8'
        resInt = resInt >> (1*8)
        cnt += 1

    
    ## result -> compact BITS format  (4 bytes)
    ##  top byte -> count of bytes in the expanded format
    ##  lower three bytes -> integer value
    resInt = (cnt << (3*8))  | resInt
    return resInt

```


```python
##
##  for a given Regimen ID# 
##
##        bits_hex: the BITS field in the BTC blockchain
##     exp_const:  expanded uint256 value from that BITS field
##
##    calc_diff_full:  predicted TARGET value, defines a new difficulty
##   calc_diff_compact:  predicted BITS compact for calc_diff_full
##
##  e.g.  Reg# 220  predicted BITS_hex: 0x18034379
##        Reg# 221  shows actual  BITS_hex: 0x18034379
##
##---------------------------------------------------------------------------------


tbase = 0
## well-known constant for 2-weeks of mining
dif_interval = 6*24*14    # =>2016 ten minute blocks     

## convenience variables to mark start and end in the interval
sbeg = (dif_interval * tbase) + 2015   ## skip the genesis interval in all calcs
send = sbeg + dif_interval             ## the last block with the same difficulty

elem  = sbeg
cnt   = 0

while cnt < len(df_datachain):
    ## note: time calc considers only blocks leading to the last block
    try:
        blk_start_ts = df_datachain['blk_time_longint'][sbeg]
        blk_end_ts =  df_datachain['blk_time_longint'][send-1]
    except:
        break
    blk_interval_secs =  blk_end_ts - blk_start_ts

    ##--
    print 'Reg# '+str(tbase+cnt)+'     blk_ht: '+ str(df_datachain['blk_ht'][sbeg]) 
    print '        seconds: '+str(blk_interval_secs)
    print " from "+datetime.utcfromtimestamp(float(blk_start_ts)).strftime("%Y-%m-%d %H:%M:%S")
    print "   to "+datetime.utcfromtimestamp(float(blk_end_ts)).strftime("%Y-%m-%d %H:%M:%S")
    print " "

    ## retrieve compact BITS string for this Regimen; extract parts
    bits_hex = int( df_datachain['bits_hex'][sbeg], 16) 
    reg_difficulty  = bits_hex & 0x007FFFFF
    reg_exp = (bits_hex & 0xFF000000) >> 24
    print '      bits_hex: '+hex(bits_hex)+'  | '+hex(reg_exp)+'  '+hex(reg_difficulty)

    ## calc the ratio of (actual seconds) / (ideal seconds) for this regimen
    ##  ratio < 1.0 shows the engine is producing too many blocks
    ##   therefore increase the difficulty to make new blocks
    ##  ratio > 1.0 shows the engine did not produce enough blocks
    ##   therefore decrease the difficulty to make a new block
    trt = blk_interval_secs / (2016*600.0)

    if int(df_datachain['blk_ht'][sbeg]) < 30000:  tind = 'NO_CHANGE'
    elif trt > 1.0:  tind = 'DECREASE'
    else:  tind = 'INCREASE'  

    #  extract target value     TODO use factors
    exp_const = reg_difficulty * 2**unpack_exp(reg_exp)
    exp_bits =  1 * 2**unpack_exp(reg_exp)
    #print "exp_const bit length: "+ str(exp_const.bit_length())
    #print "exp_const byte length: "+ str((exp_const.bit_length()+7)/8)
    #print "exp_bits byte length: "+ str((exp_bits.bit_length()+7)/8)
    print "     exp_const: " + hex(exp_const)[:-1] + "\n"
    
    # calc new hex value in two steps, using integers 
    MBFLAG = True
    if MBFLAG:
      calc_diff = blk_interval_secs * exp_const
      calc_diff_pre = calc_diff / (2016*600)
      print "calc_diff_full: " + hex(calc_diff_pre)[:-1] 
      calc_diff_post = getCompact( calc_diff_pre )
      print "calc_diff_compact: " + hex(calc_diff_post)[:-1] + "\n"
      print "              Difficulty -> " + tind 

      #calc_diff_long = calc_diff / (2016*600)
      #print "calc_diff_long  bytes: "+ str((calc_diff_long.bit_length()+7)/8)
      #print "calc_diff byte length: "+ str((calc_diff.bit_length()+7)/8)
    else:
      calc_diff = blk_interval_secs * reg_difficulty
      calc_diff = calc_diff / (2016*600)
      print " Difficulty -> " + tind + " calc_diff: " + hex(calc_diff)[:-1] + "\n"
    
    print "------------------------------------"
    
    cnt = cnt + 1
    sbeg = sbeg + 2016
    send = send + 2016
    #if cnt == 36: break

```

    Reg# 0     blk_ht: 2016
            seconds: 1401591
     from 2009-01-27 13:38:51
       to 2009-02-12 18:58:42
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x128a0e4b1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1
    calc_diff_compact: 0x1d0128a0
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 1     blk_ht: 4032
            seconds: 1499277
     from 2009-02-12 19:16:30
       to 2009-03-02 03:44:27
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x13d4d6e02702702702702702702702702702702702702702702702702
    calc_diff_compact: 0x1d013d4d
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 2     blk_ht: 6048
            seconds: 1540887
     from 2009-03-02 04:01:53
       to 2009-03-20 00:03:20
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1461bd21ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1a
    calc_diff_compact: 0x1d01461b
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 3     blk_ht: 8064
            seconds: 1546192
     from 2009-03-20 00:26:26
       to 2009-04-06 21:56:18
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1473b3d9cd9cd9cd9cd9cd9cd9cd9cd9cd9cd9cd9cd9cd9cd9cd9cd9c
    calc_diff_compact: 0x1d01473b
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 4     blk_ht: 10080
            seconds: 1543629
     from 2009-04-06 22:04:23
       to 2009-04-24 18:51:32
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x146b06135a35a35a35a35a35a35a35a35a35a35a35a35a35a35a35a35
    calc_diff_compact: 0x1d0146b0
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 5     blk_ht: 12096
            seconds: 1498902
     from 2009-04-24 18:51:38
       to 2009-05-12 03:13:20
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x13d391cd1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1
    calc_diff_compact: 0x1d013d39
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 6     blk_ht: 14112
            seconds: 1637373
     from 2009-05-12 03:20:25
       to 2009-05-31 02:09:58
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x15a87579c09c09c09c09c09c09c09c09c09c09c09c09c09c09c09c09c
    calc_diff_compact: 0x1d015a87
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 7     blk_ht: 16128
            seconds: 2313755
     from 2009-05-31 02:31:25
       to 2009-06-26 21:14:00
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1e9ad1696596596596596596596596596596596596596596596596596
    calc_diff_compact: 0x1d01e9ad
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 8     blk_ht: 18144
            seconds: 2429549
     from 2009-06-26 21:32:53
       to 2009-07-25 00:25:22
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2022eb2fcbfcbfcbfcbfcbfcbfcbfcbfcbfcbfcbfcbfcbfcbfcbfcbfc
    calc_diff_compact: 0x1d02022e
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 9     blk_ht: 20160
            seconds: 3585115
     from 2009-07-25 00:30:16
       to 2009-09-04 12:22:11
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2f6be2ffbefbefbefbefbefbefbefbefbefbefbefbefbefbefbefbefb
    calc_diff_compact: 0x1d02f6be
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 10     blk_ht: 22176
            seconds: 2384993
     from 2009-09-04 13:01:38
       to 2009-10-02 03:31:31
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1f8c0b256f56f56f56f56f56f56f56f56f56f56f56f56f56f56f56f56
    calc_diff_compact: 0x1d01f8c0
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 11     blk_ht: 24192
            seconds: 2546179
     from 2009-10-02 03:27:08
       to 2009-10-31 14:43:27
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x21add9a93193193193193193193193193193193193193193193193193
    calc_diff_compact: 0x1d021add
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 12     blk_ht: 26208
            seconds: 2355548
     from 2009-10-31 15:28:20
       to 2009-11-27 21:47:28
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1f28564dd0dd0dd0dd0dd0dd0dd0dd0dd0dd0dd0dd0dd0dd0dd0dd0dd
    calc_diff_compact: 0x1d01f285
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 13     blk_ht: 28224
            seconds: 1769956
     from 2009-11-27 21:51:07
       to 2009-12-18 09:30:23
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1769690c64c64c64c64c64c64c64c64c64c64c64c64c64c64c64c64c6
    calc_diff_compact: 0x1d017696
    
                  Difficulty -> NO_CHANGE
    ------------------------------------
    Reg# 14     blk_ht: 30240
            seconds: 1022578
     from 2009-12-18 09:56:01
       to 2009-12-30 05:58:59
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
         exp_const: 0xffff0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xd86a528bc8bc8bc8bc8bc8bc8bc8bc8bc8bc8bc8bc8bc8bc8bc8bc8b
    calc_diff_compact: 0x1d00d86a
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 15     blk_ht: 32256
            seconds: 1096378
     from 2009-12-30 06:11:04
       to 2010-01-11 22:44:02
     
          bits_hex: 0x1d00d86a  | 0x1d  0xd86a
         exp_const: 0xd86a0000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xc428371a0f7ed5cb3a9186f64d42b208fe6dc4ba298075e53c31a0f7
    calc_diff_compact: 0x1d00c428
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 16     blk_ht: 34272
            seconds: 1174364
     from 2010-01-11 22:48:37
       to 2010-01-25 13:01:21
     
          bits_hex: 0x1d00c428  | 0x1d  0xc428
         exp_const: 0xc4280000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xbe71317c8f406b7e2f5a6d1e495c0d384afc2739eb1628da0517c8f4
    calc_diff_compact: 0x1d00be71
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 17     blk_ht: 36288
            seconds: 894058
     from 2010-01-25 13:07:59
       to 2010-02-04 21:28:57
     
          bits_hex: 0x1d00be71  | 0x1d  0xbe71
         exp_const: 0xbe710000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x8cc30f97a647313fe0cad97a647313fe0cad97a647313fe0cad97a64
    calc_diff_compact: 0x1d008cc3
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 18     blk_ht: 38304
            seconds: 870279
     from 2010-02-04 21:43:14
       to 2010-02-14 23:27:53
     
          bits_hex: 0x1d008cc3  | 0x1d  0x8cc3
         exp_const: 0x8cc30000000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x654657a76a76a76a76a76a76a76a76a76a76a76a76a76a76a76a76a7
    calc_diff_compact: 0x1c654657
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 19     blk_ht: 40320
            seconds: 808624
     from 2010-02-14 23:52:59
       to 2010-02-24 08:30:03
     
          bits_hex: 0x1c654657  | 0x1c  0x654657
         exp_const: 0x65465700000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x43b3e53670cd733d9a400a670cd733d9a400a670cd733d9a400a670c
    calc_diff_compact: 0x1c43b3e5
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 20     blk_ht: 42336
            seconds: 1009409
     from 2010-02-24 08:41:04
       to 2010-03-08 01:04:33
     
          bits_hex: 0x1c43b3e5  | 0x1c  0x43b3e5
         exp_const: 0x43b3e500000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x387f6f38fdd532a87fdd532a87fdd532a87fdd532a87fdd532a87fdd
    calc_diff_compact: 0x1c387f6f
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 21     blk_ht: 44352
            seconds: 1200570
     from 2010-03-08 01:14:33
       to 2010-03-21 22:44:03
     
          bits_hex: 0x1c387f6f  | 0x1c  0x387f6f
         exp_const: 0x387f6f00000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x381375c5777777777777777777777777777777777777777777777777
    calc_diff_compact: 0x1c381375
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 22     blk_ht: 46368
            seconds: 907410
     from 2010-03-21 22:54:24
       to 2010-04-01 10:57:54
     
          bits_hex: 0x1c381375  | 0x1c  0x381375
         exp_const: 0x38137500000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2a1115c5333333333333333333333333333333333333333333333333
    calc_diff_compact: 0x1c2a1115
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 23     blk_ht: 48384
            seconds: 941328
     from 2010-04-01 11:07:22
       to 2010-04-12 08:36:10
     
          bits_hex: 0x1c2a1115  | 0x1c  0x2a1115
         exp_const: 0x2a111500000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x20bca74a83a83a83a83a83a83a83a83a83a83a83a83a83a83a83a83a
    calc_diff_compact: 0x1c20bca7
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 24     blk_ht: 50400
            seconds: 825067
     from 2010-04-12 08:39:46
       to 2010-04-21 21:50:53
     
          bits_hex: 0x1c20bca7  | 0x1c  0x20bca7
         exp_const: 0x20bca700000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x16546f575ab277f44c118de5ab277f44c118de5ab277f44c118de5ab
    calc_diff_compact: 0x1c16546f
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 25     blk_ht: 52416
            seconds: 1079231
     from 2010-04-21 21:52:52
       to 2010-05-04 09:40:03
     
          bits_hex: 0x1c16546f  | 0x1c  0x16546f
         exp_const: 0x16546f00000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x13ec5309257e139cf58b146d028be47a035bf17ad368f24ae069c257
    calc_diff_compact: 0x1c13ec53
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 26     blk_ht: 54432
            seconds: 1312011
     from 2010-05-04 09:46:16
       to 2010-05-19 14:13:07
     
          bits_hex: 0x1c13ec53  | 0x1c  0x13ec53
         exp_const: 0x13ec5300000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x159c24e264efe898231bcb564efe898231bcb564efe898231bcb564e
    calc_diff_compact: 0x1c159c24
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 27     blk_ht: 56448
            seconds: 862214
     from 2010-05-19 14:13:55
       to 2010-05-29 13:44:09
     
          bits_hex: 0x1c159c24  | 0x1c  0x159c24
         exp_const: 0x159c2400000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xf675c56844eab511b781de844eab511b781de844eab511b781de844
    calc_diff_compact: 0x1c0f675c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 28     blk_ht: 58464
            seconds: 1156544
     from 2010-05-29 13:57:28
       to 2010-06-11 23:13:12
     
          bits_hex: 0x1c0f675c  | 0x1c  0xf675c
         exp_const: 0xf675c00000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xeba64df7df7df7df7df7df7df7df7df7df7df7df7df7df7df7df7df
    calc_diff_compact: 0x1c0eba64
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 29     blk_ht: 60480
            seconds: 1083477
     from 2010-06-11 23:26:26
       to 2010-06-24 12:24:23
     
          bits_hex: 0x1c0eba64  | 0x1c  0xeba64
         exp_const: 0xeba6400000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xd314219a35a35a35a35a35a35a35a35a35a35a35a35a35a35a35a35
    calc_diff_compact: 0x1c0d3142
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 30     blk_ht: 62496
            seconds: 998758
     from 2010-06-24 12:27:26
       to 2010-07-06 01:53:24
     
          bits_hex: 0x1c0d3142  | 0x1c  0xd3142
         exp_const: 0xd314200000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xae493d265887aa9ccbeee110332554776998bbaddcfff2214436658
    calc_diff_compact: 0x1c0ae493
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 31     blk_ht: 64512
            seconds: 626344
     from 2010-07-06 01:57:44
       to 2010-07-13 07:56:48
     
          bits_hex: 0x1c0ae493  | 0x1c  0xae493
         exp_const: 0xae49300000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x5a3f437d4a7f529fd4a7f529fd4a7f529fd4a7f529fd4a7f529fd4a
    calc_diff_compact: 0x1c05a3f4
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 32     blk_ht: 66528
            seconds: 289434
     from 2010-07-13 08:03:57
       to 2010-07-16 16:27:51
     
          bits_hex: 0x1c05a3f4  | 0x1c  0x5a3f4
         exp_const: 0x5a3f400000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x159829bf8d9272c0c5a5f3f8d9272c0c5a5f3f8d9272c0c5a5f3f8d
    calc_diff_compact: 0x1c015982
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 33     blk_ht: 68544
            seconds: 899195
     from 2010-07-16 16:29:39
       to 2010-07-27 02:16:14
     
          bits_hex: 0x1c0168fd  | 0x1c  0x168fd
         exp_const: 0x168fd00000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x10c5a2bc0e1636b8c0e1636b8c0e1636b8c0e1636b8c0e1636b8c0e
    calc_diff_compact: 0x1c010c5a
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 34     blk_ht: 70560
            seconds: 838835
     from 2010-07-27 02:42:38
       to 2010-08-05 19:43:13
     
          bits_hex: 0x1c010c5a  | 0x1c  0x10c5a
         exp_const: 0x10c5a00000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xba18c78a6dfc3518a6dfc3518a6dfc3518a6dfc3518a6dfc3518a6
    calc_diff_compact: 0x1c00ba18
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 35     blk_ht: 72576
            seconds: 832370
     from 2010-08-05 19:46:35
       to 2010-08-15 10:59:25
     
          bits_hex: 0x1c00ba18  | 0x1c  0xba18
         exp_const: 0xba1800000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x800ed38e38e38e38e38e38e38e38e38e38e38e38e38e38e38e38e3
    calc_diff_compact: 0x1c00800e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 36     blk_ht: 74592
            seconds: 993029
     from 2010-08-15 11:11:11
       to 2010-08-26 23:01:40
     
          bits_hex: 0x1c00800e  | 0x1c  0x800e
         exp_const: 0x800e00000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x692098b19a088f77e66d55c44b33a229118006ef5de4cd3bc2ab19
    calc_diff_compact: 0x1b692098
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 37     blk_ht: 76608
            seconds: 1057743
     from 2010-08-26 23:13:23
       to 2010-09-08 05:02:26
     
          bits_hex: 0x1b692098  | 0x1b  0x692098
         exp_const: 0x692098000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x5bede6de2fc962fc962fc962fc962fc962fc962fc962fc962fc962
    calc_diff_compact: 0x1b5bede6
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 38     blk_ht: 78624
            seconds: 939504
     from 2010-09-08 05:04:49
       to 2010-09-19 02:03:13
     
          bits_hex: 0x1b5bede6  | 0x1b  0x5bede6
         exp_const: 0x5bede6000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x4766eda78edf545bac212878edf545bac212878edf545bac212878
    calc_diff_compact: 0x1b4766ed
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 39     blk_ht: 80640
            seconds: 841915
     from 2010-09-19 02:04:07
       to 2010-09-28 19:56:02
     
          bits_hex: 0x1b4766ed  | 0x1b  0x4766ed
         exp_const: 0x4766ed000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x31b2a316fa94fea53fa94fea53fa94fea53fa94fea53fa94fea53f
    calc_diff_compact: 0x1b31b2a3
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 40     blk_ht: 82656
            seconds: 1157497
     from 2010-09-28 19:58:28
       to 2010-10-12 05:30:05
     
          bits_hex: 0x1b31b2a3  | 0x1b  0x31b2a3
         exp_const: 0x31b2a3000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2f8e9d572d82d82d82d82d82d82d82d82d82d82d82d82d82d82d82
    calc_diff_compact: 0x1b2f8e9d
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 41     blk_ht: 84672
            seconds: 775638
     from 2010-10-12 05:35:05
       to 2010-10-21 05:02:23
     
          bits_hex: 0x1b2f8e9d  | 0x1b  0x2f8e9d
         exp_const: 0x2f8e9d000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1e7eca230369d0369d0369d0369d0369d0369d0369d0369d0369d0
    calc_diff_compact: 0x1b1e7eca
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 42     blk_ht: 86688
            seconds: 840776
     from 2010-10-21 05:13:15
       to 2010-10-30 22:46:11
     
          bits_hex: 0x1b1e7eca  | 0x1b  0x1e7eca
         exp_const: 0x1e7eca000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x153263bdf1ce0bcfabe9ad89c78b67a569458347236125013f02df
    calc_diff_compact: 0x1b153263
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 43     blk_ht: 88704
            seconds: 824399
     from 2010-10-30 22:58:47
       to 2010-11-09 11:58:46
     
          bits_hex: 0x1b153263  | 0x1b  0x153263
         exp_const: 0x153263000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xe7256226140beb696140beb696140beb696140beb696140beb696
    calc_diff_compact: 0x1b0e7256
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 44     blk_ht: 90720
            seconds: 799077
     from 2010-11-09 12:29:28
       to 2010-11-18 18:27:25
     
          bits_hex: 0x1b0e7256  | 0x1b  0xe7256
         exp_const: 0xe7256000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x98b2ad53f8d9272c0c5a5f3f8d9272c0c5a5f3f8d9272c0c5a5f3
    calc_diff_compact: 0x1b098b2a
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 45     blk_ht: 92736
            seconds: 1028226
     from 2010-11-18 18:44:34
       to 2010-11-30 16:21:40
     
          bits_hex: 0x1b098b2a  | 0x1b  0x98b2a
         exp_const: 0x98b2a000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x81cd2a071c71c71c71c71c71c71c71c71c71c71c71c71c71c71c7
    calc_diff_compact: 0x1b081cd2
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 46     blk_ht: 94752
            seconds: 797535
     from 2010-11-30 16:37:55
       to 2010-12-09 22:10:10
     
          bits_hex: 0x1b081cd2  | 0x1b  0x81cd2
         exp_const: 0x81cd2000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x559537d5075075075075075075075075075075075075075075075
    calc_diff_compact: 0x1b055953
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 47     blk_ht: 96768
            seconds: 1023191
     from 2010-12-09 22:20:02
       to 2010-12-21 18:33:13
     
          bits_hex: 0x1b055953  | 0x1b  0x55953
         exp_const: 0x55953000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x4864ce9e0f475adc1427a8e0f475adc1427a8e0f475adc1427a8e
    calc_diff_compact: 0x1b04864c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 48     blk_ht: 98784
            seconds: 1074363
     from 2010-12-21 18:34:03
       to 2011-01-03 05:00:06
     
          bits_hex: 0x1b04864c  | 0x1b  0x4864c
         exp_const: 0x4864c000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x404cbb4726d8d3f3a5a0c0726d8d3f3a5a0c0726d8d3f3a5a0c07
    calc_diff_compact: 0x1b0404cb
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 49     blk_ht: 100800
            seconds: 1069848
     from 2011-01-03 05:10:11
       to 2011-01-15 14:20:59
     
          bits_hex: 0x1b0404cb  | 0x1b  0x404cb
         exp_const: 0x404cb000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x38dee3e4efe898231bcb564efe898231bcb564efe898231bcb564
    calc_diff_compact: 0x1b038dee
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 50     blk_ht: 102816
            seconds: 1013168
     from 2011-01-15 14:26:07
       to 2011-01-27 07:52:15
     
          bits_hex: 0x1b038dee  | 0x1b  0x38dee
         exp_const: 0x38dee000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2fa29932fbb8440cc9551dda662eeb773ffc88510d99621eaa732
    calc_diff_compact: 0x1b02fa29
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 51     blk_ht: 104832
            seconds: 1024171
     from 2011-01-27 08:16:11
       to 2011-02-08 04:45:42
     
          bits_hex: 0x1b02fa29  | 0x1b  0x2fa29
         exp_const: 0x2fa29000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x285529c013f02df1ce0bcfabe9ad89c78b67a5694583472361250
    calc_diff_compact: 0x1b028552
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 52     blk_ht: 106848
            seconds: 862511
     from 2011-02-08 04:53:20
       to 2011-02-18 04:28:31
     
          bits_hex: 0x1b028552  | 0x1b  0x28552
         exp_const: 0x28552000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1cc26113b6f72b2e6ea2a5e61a1d5d9194d5090c4c8083c3f7fb3
    calc_diff_compact: 0x1b01cc26
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 53     blk_ht: 108864
            seconds: 793357
     from 2011-02-18 05:15:52
       to 2011-02-27 09:38:29
     
          bits_hex: 0x1b01cc26  | 0x1b  0x1cc26
         exp_const: 0x1cc26000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x12dcdcba88dbc0ef422755a88dbc0ef422755a88dbc0ef422755a
    calc_diff_compact: 0x1b012dcd
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 54     blk_ht: 110880
            seconds: 882515
     from 2011-02-27 09:59:20
       to 2011-03-09 15:07:55
     
          bits_hex: 0x1b012dcd  | 0x1b  0x12dcd
         exp_const: 0x12dcd000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xdc310a5fa0a4b4f5fa0a4b4f5fa0a4b4f5fa0a4b4f5fa0a4b4f5
    calc_diff_compact: 0x1b00dc31
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 55     blk_ht: 112896
            seconds: 1336130
     from 2011-03-09 15:25:55
       to 2011-03-25 02:34:45
     
          bits_hex: 0x1b00dc31  | 0x1b  0xdc31
         exp_const: 0xdc31000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xf33978348df389e348df389e348df389e348df389e348df389e3
    calc_diff_compact: 0x1b00f339
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 56     blk_ht: 114912
            seconds: 1013251
     from 2011-03-25 02:39:45
       to 2011-04-05 20:07:16
     
          bits_hex: 0x1b00f339  | 0x1b  0xf339
         exp_const: 0xf339000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xcbbdcc82d82d82d82d82d82d82d82d82d82d82d82d82d82d82d8
    calc_diff_compact: 0x1b00cbbd
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 57     blk_ht: 116928
            seconds: 1078600
     from 2011-04-05 20:09:57
       to 2011-04-18 07:46:37
     
          bits_hex: 0x1b00cbbd  | 0x1b  0xcbbd
         exp_const: 0xcbbd000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xb5ac6212f684bda12f684bda12f684bda12f684bda12f684bda1
    calc_diff_compact: 0x1b00b5ac
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 58     blk_ht: 118944
            seconds: 1018564
     from 2011-04-18 07:49:36
       to 2011-04-30 02:45:40
     
          bits_hex: 0x1b00b5ac  | 0x1b  0xb5ac
         exp_const: 0xb5ac000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x98fad7ba375f31aed6a9264e209dc598153d0f8cb487042bfe7b
    calc_diff_compact: 0x1b0098fa
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 59     blk_ht: 120960
            seconds: 842714
     from 2011-04-30 02:53:00
       to 2011-05-09 20:58:14
     
          bits_hex: 0x1b0098fa  | 0x1b  0x98fa
         exp_const: 0x98fa000000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x6a93b3a79412dac7460dfa79412dac7460dfa79412dac7460dfa
    calc_diff_compact: 0x1a6a93b3
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 60     blk_ht: 122976
            seconds: 780013
     from 2011-05-09 21:17:24
       to 2011-05-18 21:57:37
     
          bits_hex: 0x1a6a93b3  | 0x1a  0x6a93b3
         exp_const: 0x6a93b30000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x44b9f2cf707a3ad6e0a13d4707a3ad6e0a13d4707a3ad6e0a13d
    calc_diff_compact: 0x1a44b9f2
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 61     blk_ht: 124992
            seconds: 678993
     from 2011-05-18 22:04:47
       to 2011-05-26 18:41:20
     
          bits_hex: 0x1a44b9f2  | 0x1a  0x44b9f2
         exp_const: 0x44b9f20000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2694210a00000000000000000000000000000000000000000000
    calc_diff_compact: 0x1a269421
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 62     blk_ht: 127008
            seconds: 927297
     from 2011-05-26 18:41:56
       to 2011-06-06 12:16:53
     
          bits_hex: 0x1a269421  | 0x1a  0x269421
         exp_const: 0x2694210000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1d932f37e147ae147ae147ae147ae147ae147ae147ae147ae147
    calc_diff_compact: 0x1a1d932f
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 63     blk_ht: 129024
            seconds: 782446
     from 2011-06-06 12:25:05
       to 2011-06-15 13:45:51
     
          bits_hex: 0x1a1d932f  | 0x1a  0x1d932f
         exp_const: 0x1d932f0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x13218591eac2f0734b78fbd401845c8a0ce512956d9b1df623a6
    calc_diff_compact: 0x1a132185
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 64     blk_ht: 131040
            seconds: 769120
     from 2011-06-15 13:49:34
       to 2011-06-24 11:28:14
     
          bits_hex: 0x1a132185  | 0x1a  0x132185
         exp_const: 0x1321850000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xc2a1229d1f2747c9d1f2747c9d1f2747c9d1f2747c9d1f2747c
    calc_diff_compact: 0x1a0c2a12
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 65     blk_ht: 133056
            seconds: 1067334
     from 2011-06-24 11:45:23
       to 2011-07-06 20:14:17
     
          bits_hex: 0x1a0c2a12  | 0x1a  0xc2a12
         exp_const: 0xc2a120000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xabbcfd07ae147ae147ae147ae147ae147ae147ae147ae147ae1
    calc_diff_compact: 0x1a0abbcf
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 66     blk_ht: 135072
            seconds: 1118129
     from 2011-07-06 20:35:46
       to 2011-07-19 19:11:15
     
          bits_hex: 0x1a0abbcf  | 0x1a  0xabbcf
         exp_const: 0xabbcf0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x9ec045557034e12bf09ce7ac58a368145f23d01adf8bd69b479
    calc_diff_compact: 0x1a09ec04
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 67     blk_ht: 137088
            seconds: 1082870
     from 2011-07-19 19:23:09
       to 2011-08-01 08:10:59
     
          bits_hex: 0x1a09ec04  | 0x1a  0x9ec04
         exp_const: 0x9ec040000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x8e1e5d305b05b05b05b05b05b05b05b05b05b05b05b05b05b05
    calc_diff_compact: 0x1a08e1e5
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 68     blk_ht: 139104
            seconds: 1265258
     from 2011-08-01 08:11:19
       to 2011-08-15 23:38:57
     
          bits_hex: 0x1a08e1e5  | 0x1a  0x8e1e5
         exp_const: 0x8e1e50000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x94a864642f762a95dc90fc42f762a95dc90fc42f762a95dc90f
    calc_diff_compact: 0x1a094a86
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 69     blk_ht: 141120
            seconds: 1228602
     from 2011-08-15 23:44:54
       to 2011-08-30 05:01:36
     
          bits_hex: 0x1a094a86  | 0x1a  0x94a86
         exp_const: 0x94a860000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x96fe3708d159e26af37c048d159e26af37c048d159e26af37c0
    calc_diff_compact: 0x1a096fe3
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 70     blk_ht: 143136
            seconds: 1225000
     from 2011-08-30 05:15:03
       to 2011-09-13 09:31:43
     
          bits_hex: 0x1a096fe3  | 0x1a  0x96fe3
         exp_const: 0x96fe30000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x98ea5004bda12f684bda12f684bda12f684bda12f684bda12f6
    calc_diff_compact: 0x1a098ea5
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 71     blk_ht: 145152
            seconds: 1256924
     from 2011-09-13 09:31:56
       to 2011-09-27 22:40:40
     
          bits_hex: 0x1a098ea5  | 0x1a  0x98ea5
         exp_const: 0x98ea50000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x9ee5dc35c97eba0dc2fe520742964b86da8fcb1ed40f631853a
    calc_diff_compact: 0x1a09ee5d
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 72     blk_ht: 147168
            seconds: 1391791
     from 2011-09-27 22:47:04
       to 2011-10-14 01:23:35
     
          bits_hex: 0x1a09ee5d  | 0x1a  0x9ee5d
         exp_const: 0x9ee5d0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xb6d4bd679a244cef79a244cef79a244cef79a244cef79a244ce
    calc_diff_compact: 0x1a0b6d4b
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 73     blk_ht: 149184
            seconds: 1475684
     from 2011-10-14 01:44:35
       to 2011-10-31 03:39:19
     
          bits_hex: 0x1a0b6d4b  | 0x1a  0xb6d4b
         exp_const: 0xb6d4b0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xdf0ca2a38e38e38e38e38e38e38e38e38e38e38e38e38e38e38
    calc_diff_compact: 0x1a0df0ca
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 74     blk_ht: 151200
            seconds: 1220722
     from 2011-10-31 03:42:14
       to 2011-11-14 06:47:36
     
          bits_hex: 0x1a0df0ca  | 0x1a  0xdf0ca
         exp_const: 0xdf0ca0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xe119a70421976ecc421976ecc421976ecc421976ecc421976ec
    calc_diff_compact: 0x1a0e119a
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 75     blk_ht: 153216
            seconds: 1322477
     from 2011-11-14 06:56:10
       to 2011-11-29 14:17:27
     
          bits_hex: 0x1a0e119a  | 0x1a  0xe119a
         exp_const: 0xe119a0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xf61b1c8b696140beb696140beb696140beb696140beb696140b
    calc_diff_compact: 0x1a0f61b1
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 76     blk_ht: 155232
            seconds: 1142240
     from 2011-11-29 14:20:20
       to 2011-12-12 19:37:40
     
          bits_hex: 0x1a0f61b1  | 0x1a  0xf61b1
         exp_const: 0xf61b10000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xe8668f01e573ac901e573ac901e573ac901e573ac901e573ac9
    calc_diff_compact: 0x1a0e8668
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 77     blk_ht: 157248
            seconds: 1204500
     from 2011-12-12 19:42:35
       to 2011-12-26 18:17:35
     
          bits_hex: 0x1a0e8668  | 0x1a  0xe8668
         exp_const: 0xe86680000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xe76ba7b6db6db6db6db6db6db6db6db6db6db6db6db6db6db6d
    calc_diff_compact: 0x1a0e76ba
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 78     blk_ht: 159264
            seconds: 1121761
     from 2011-12-26 18:43:25
       to 2012-01-08 18:19:26
     
          bits_hex: 0x1a0e76ba  | 0x1a  0xe76ba
         exp_const: 0xe76ba0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xd69d75569d0369d0369d0369d0369d0369d0369d0369d0369d0
    calc_diff_compact: 0x1a0d69d7
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 79     blk_ht: 161280
            seconds: 1156905
     from 2012-01-08 18:26:16
       to 2012-01-22 03:48:01
     
          bits_hex: 0x1a0d69d7  | 0x1a  0xd69d7
         exp_const: 0xd69d70000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xcd43f828af8af8af8af8af8af8af8af8af8af8af8af8af8af8a
    calc_diff_compact: 0x1a0cd43f
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 80     blk_ht: 163296
            seconds: 1146546
     from 2012-01-22 03:55:04
       to 2012-02-04 10:24:10
     
          bits_hex: 0x1a0cd43f  | 0x1a  0xcd43f
         exp_const: 0xcd43f0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xc290b84914f7b5e1c482ae914f7b5e1c482ae914f7b5e1c482a
    calc_diff_compact: 0x1a0c290b
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 81     blk_ht: 165312
            seconds: 1212540
     from 2012-02-04 10:32:41
       to 2012-02-18 11:21:41
     
          bits_hex: 0x1a0c290b  | 0x1a  0xc290b
         exp_const: 0xc290b0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xc309c0127d27d27d27d27d27d27d27d27d27d27d27d27d27d27
    calc_diff_compact: 0x1a0c309c
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 82     blk_ht: 167328
            seconds: 1112091
     from 2012-02-18 11:24:15
       to 2012-03-02 08:19:06
     
          bits_hex: 0x1a0c309c  | 0x1a  0xc309c
         exp_const: 0xc309c0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xb350c87796ac9dfd130463796ac9dfd130463796ac9dfd13046
    calc_diff_compact: 0x1a0b350c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 83     blk_ht: 169344
            seconds: 1208538
     from 2012-03-02 08:25:36
       to 2012-03-16 08:07:54
     
          bits_hex: 0x1a0b350c  | 0x1a  0xb350c
         exp_const: 0xb350c0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xb328725c67600f9a9342cdc67600f9a9342cdc67600f9a9342c
    calc_diff_compact: 0x1a0b3287
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 84     blk_ht: 171360
            seconds: 1114220
     from 2012-03-16 08:09:54
       to 2012-03-29 05:40:14
     
          bits_hex: 0x1a0b3287  | 0x1a  0xb3287
         exp_const: 0xb32870000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xa507e94d4629b7f0d4629b7f0d4629b7f0d4629b7f0d4629b7f
    calc_diff_compact: 0x1a0a507e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 85     blk_ht: 173376
            seconds: 1246887
     from 2012-03-29 05:41:47
       to 2012-04-12 16:03:14
     
          bits_hex: 0x1a0a507e  | 0x1a  0xa507e
         exp_const: 0xa507e0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xaa1e34167dce434a9b101767dce434a9b101767dce434a9b101
    calc_diff_compact: 0x1a0aa1e3
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 86     blk_ht: 175392
            seconds: 1265185
     from 2012-04-12 16:04:49
       to 2012-04-27 07:31:14
     
          bits_hex: 0x1a0aa1e3  | 0x1a  0xaa1e3
         exp_const: 0xaa1e30000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xb1ef7553d3928e7e3d3928e7e3d3928e7e3d3928e7e3d3928e7
    calc_diff_compact: 0x1a0b1ef7
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 87     blk_ht: 177408
            seconds: 1052841
     from 2012-04-27 07:39:30
       to 2012-05-09 12:06:51
     
          bits_hex: 0x1a0b1ef7  | 0x1a  0xb1ef7
         exp_const: 0xb1ef70000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x9ae02a5f4c8e627fc195b2f4c8e627fc195b2f4c8e627fc195b
    calc_diff_compact: 0x1a09ae02
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 88     blk_ht: 179424
            seconds: 1317656
     from 2012-05-09 12:08:33
       to 2012-05-24 18:09:29
     
          bits_hex: 0x1a09ae02  | 0x1a  0x9ae02
         exp_const: 0x9ae020000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xa8b5f8c2c6d7181c2c6d7181c2c6d7181c2c6d7181c2c6d7181
    calc_diff_compact: 0x1a0a8b5f
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 89     blk_ht: 181440
            seconds: 1215635
     from 2012-05-24 18:10:29
       to 2012-06-07 19:51:04
     
          bits_hex: 0x1a0a8b5f  | 0x1a  0xa8b5f
         exp_const: 0xa8b5f0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xa98d6c4a60fb650ba60fb650ba60fb650ba60fb650ba60fb650
    calc_diff_compact: 0x1a0a98d6
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 90     blk_ht: 183456
            seconds: 1109145
     from 2012-06-07 20:05:25
       to 2012-06-20 16:11:10
     
          bits_hex: 0x1a0a98d6  | 0x1a  0xa98d6
         exp_const: 0xa98d60000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x9b78a3f2b12b12b12b12b12b12b12b12b12b12b12b12b12b12b
    calc_diff_compact: 0x1a09b78a
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 91     blk_ht: 185472
            seconds: 1192412
     from 2012-06-20 16:16:04
       to 2012-07-04 11:29:36
     
          bits_hex: 0x1a09b78a  | 0x1a  0x9b78a
         exp_const: 0x9b78a0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x99431261ee1ee1ee1ee1ee1ee1ee1ee1ee1ee1ee1ee1ee1ee1e
    calc_diff_compact: 0x1a099431
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 92     blk_ht: 187488
            seconds: 1135110
     from 2012-07-04 11:37:21
       to 2012-07-17 14:55:51
     
          bits_hex: 0x1a099431  | 0x1a  0x99431
         exp_const: 0x994310000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x8fd2e0203403403403403403403403403403403403403403403
    calc_diff_compact: 0x1a08fd2e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 93     blk_ht: 189504
            seconds: 1108470
     from 2012-07-17 14:59:26
       to 2012-07-30 10:53:56
     
          bits_hex: 0x1a08fd2e  | 0x1a  0x8fd2e
         exp_const: 0x8fd2e0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x83cc97308f08f08f08f08f08f08f08f08f08f08f08f08f08f08
    calc_diff_compact: 0x1a083cc9
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 94     blk_ht: 191520
            seconds: 1124469
     from 2012-07-30 11:26:17
       to 2012-08-12 11:47:26
     
          bits_hex: 0x1a083cc9  | 0x1a  0x83cc9
         exp_const: 0x83cc90000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x7a85eb7721a54d880bb3ee721a54d880bb3ee721a54d880bb3e
    calc_diff_compact: 0x1a07a85e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 95     blk_ht: 193536
            seconds: 1085811
     from 2012-08-12 12:00:55
       to 2012-08-25 01:37:46
     
          bits_hex: 0x1a07a85e  | 0x1a  0x7a85e
         exp_const: 0x7a85e0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x6dfbede799466132dffacc799466132dffacc799466132dffac
    calc_diff_compact: 0x1a06dfbe
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 96     blk_ht: 195552
            seconds: 1095825
     from 2012-08-25 01:46:39
       to 2012-09-06 18:10:24
     
          bits_hex: 0x1a06dfbe  | 0x1a  0x6dfbe
         exp_const: 0x6dfbe0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x63a388682082082082082082082082082082082082082082082
    calc_diff_compact: 0x1a063a38
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 97     blk_ht: 197568
            seconds: 1137768
     from 2012-09-06 18:10:37
       to 2012-09-19 22:13:25
     
          bits_hex: 0x1a063a38  | 0x1a  0x63a38
         exp_const: 0x63a380000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x5db8bd381381381381381381381381381381381381381381381
    calc_diff_compact: 0x1a05db8b
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 98     blk_ht: 199584
            seconds: 1134170
     from 2012-09-19 22:14:11
       to 2012-10-03 01:17:01
     
          bits_hex: 0x1a05db8b  | 0x1a  0x5db8b
         exp_const: 0x5db8b0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x57e0843a9da9da9da9da9da9da9da9da9da9da9da9da9da9da9
    calc_diff_compact: 0x1a057e08
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 99     blk_ht: 201600
            seconds: 1202635
     from 2012-10-03 01:11:00
       to 2012-10-16 23:14:55
     
          bits_hex: 0x1a057e08  | 0x1a  0x57e08
         exp_const: 0x57e080000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x575ef67e1a8c536fe1a8c536fe1a8c536fe1a8c536fe1a8c536
    calc_diff_compact: 0x1a0575ef
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 100     blk_ht: 203616
            seconds: 1124662
     from 2012-10-16 22:56:08
       to 2012-10-29 23:20:30
     
          bits_hex: 0x1a0575ef  | 0x1a  0x575ef
         exp_const: 0x575ef0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x513c554de3ef500611722833944a55b66c77d88e99fab0bc1cd
    calc_diff_compact: 0x1a0513c5
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 101     blk_ht: 205632
            seconds: 1186476
     from 2012-10-30 00:16:35
       to 2012-11-12 17:51:11
     
          bits_hex: 0x1a0513c5  | 0x1a  0x513c5
         exp_const: 0x513c50000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x4faebf81bf4f2825b58e8c1bf4f2825b58e8c1bf4f2825b58e8
    calc_diff_compact: 0x1a04faeb
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 102     blk_ht: 207648
            seconds: 1184931
     from 2012-11-12 17:59:46
       to 2012-11-26 11:08:37
     
          bits_hex: 0x1a04faeb  | 0x1a  0x4faeb
         exp_const: 0x4faeb0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x4e0eab6c869536202ecfb9c869536202ecfb9c869536202ecfb
    calc_diff_compact: 0x1a04e0ea
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 103     blk_ht: 209664
            seconds: 1234268
     from 2012-11-26 11:10:29
       to 2012-12-10 18:01:37
     
          bits_hex: 0x1a04e0ea  | 0x1a  0x4e0ea
         exp_const: 0x4e0ea0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x4fa6241abcdf0123456789abcdf0123456789abcdf012345678
    calc_diff_compact: 0x1a04fa62
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 104     blk_ht: 211680
            seconds: 1368145
     from 2012-12-10 18:03:33
       to 2012-12-26 14:05:58
     
          bits_hex: 0x1a04fa62  | 0x1a  0x4fa62
         exp_const: 0x4fa620000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x5a16b46536fe1a8c536fe1a8c536fe1a8c536fe1a8c536fe1a8
    calc_diff_compact: 0x1a05a16b
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 105     blk_ht: 213696
            seconds: 1109130
     from 2012-12-26 14:05:40
       to 2013-01-08 10:11:10
     
          bits_hex: 0x1a05a16b  | 0x1a  0x5a16b
         exp_const: 0x5a16b0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x529b16ba35a35a35a35a35a35a35a35a35a35a35a35a35a35a3
    calc_diff_compact: 0x1a0529b1
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 106     blk_ht: 215712
            seconds: 1324001
     from 2013-01-08 10:40:34
       to 2013-01-23 18:27:15
     
          bits_hex: 0x1a0529b1  | 0x1a  0x529b1
         exp_const: 0x529b10000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x5a6b198b5aa499388277166054f43e32d21c10aff9ee8dd7cc6
    calc_diff_compact: 0x1a05a6b1
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 107     blk_ht: 217728
            seconds: 1096343
     from 2013-01-23 18:41:27
       to 2013-02-05 11:13:50
     
          bits_hex: 0x1a05a6b1  | 0x1a  0x5a6b1
         exp_const: 0x5a6b10000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x51f3c309125df2abf78c459125df2abf78c459125df2abf78c4
    calc_diff_compact: 0x1a051f3c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 108     blk_ht: 219744
            seconds: 1085180
     from 2013-02-05 11:19:06
       to 2013-02-18 00:45:26
     
          bits_hex: 0x1a051f3c  | 0x1a  0x51f3c
         exp_const: 0x51f3c0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x4985c3736736736736736736736736736736736736736736736
    calc_diff_compact: 0x1a04985c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 109     blk_ht: 221760
            seconds: 1011079
     from 2013-02-18 00:47:50
       to 2013-03-01 17:39:09
     
          bits_hex: 0x1a04985c  | 0x1a  0x4985c
         exp_const: 0x4985c0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x3d74b3ad028be47a035bf17ad368f24ae069c257e139cf58b14
    calc_diff_compact: 0x1a03d74b
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 110     blk_ht: 223776
            seconds: 1089888
     from 2013-03-01 17:42:44
       to 2013-03-14 08:27:32
     
          bits_hex: 0x1a03d74b  | 0x1a  0x3d74b
         exp_const: 0x3d74b0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x375fa6a096d63a306fd3ca096d63a306fd3ca096d63a306fd3c
    calc_diff_compact: 0x1a0375fa
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 111     blk_ht: 225792
            seconds: 875727
     from 2013-03-14 08:32:26
       to 2013-03-24 11:47:53
     
          bits_hex: 0x1a0375fa  | 0x1a  0x375fa
         exp_const: 0x375fa0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2816e0d63f63f63f63f63f63f63f63f63f63f63f63f63f63f63
    calc_diff_compact: 0x1a02816e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 112     blk_ht: 227808
            seconds: 1055556
     from 2013-03-24 12:00:25
       to 2013-04-05 17:13:01
     
          bits_hex: 0x1a02816e  | 0x1a  0x2816e
         exp_const: 0x2816e0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x22fbe2ba06d3a06d3a06d3a06d3a06d3a06d3a06d3a06d3a06d
    calc_diff_compact: 0x1a022fbe
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 113     blk_ht: 229824
            seconds: 1034206
     from 2013-04-05 17:40:43
       to 2013-04-17 16:57:29
     
          bits_hex: 0x1a022fbe  | 0x1a  0x22fbe
         exp_const: 0x22fbe0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1de9422110332554776998bbaddcfff221443665887aa9ccbee
    calc_diff_compact: 0x1a01de94
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 114     blk_ht: 231840
            seconds: 1077321
     from 2013-04-17 17:02:14
       to 2013-04-30 04:17:35
     
          bits_hex: 0x1a01de94  | 0x1a  0x1de94
         exp_const: 0x1de940000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1aa3df083fb72ea61d950c83fb72ea61d950c83fb72ea61d950
    calc_diff_compact: 0x1a01aa3d
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 115     blk_ht: 233856
            seconds: 1089484
     from 2013-04-30 04:34:31
       to 2013-05-12 19:12:35
     
          bits_hex: 0x1a01aa3d  | 0x1a  0x1aa3d
         exp_const: 0x1aa3d0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x17fe96fc0804c4908d4d1915d5a19e5e2a26e6b2af6f3b37f7c
    calc_diff_compact: 0x1a017fe9
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 116     blk_ht: 235872
            seconds: 1113442
     from 2013-05-12 19:15:23
       to 2013-05-25 16:32:45
     
          bits_hex: 0x1a017fe9  | 0x1a  0x17fe9
         exp_const: 0x17fe90000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x16164153fc50d61e72f840951a62b73c84d95ea6fb80c91da2e
    calc_diff_compact: 0x1a016164
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 117     blk_ht: 237888
            seconds: 942027
     from 2013-05-25 16:35:46
       to 2013-06-05 14:16:13
     
          bits_hex: 0x1a016164  | 0x1a  0x16164
         exp_const: 0x161640000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x11337c4f5c28f5c28f5c28f5c28f5c28f5c28f5c28f5c28f5c2
    calc_diff_compact: 0x1a011337
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 118     blk_ht: 239904
            seconds: 976089
     from 2013-06-05 14:25:18
       to 2013-06-16 21:33:27
     
          bits_hex: 0x1a011337  | 0x1a  0x11337
         exp_const: 0x113370000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xde15cb88888888888888888888888888888888888888888888
    calc_diff_compact: 0x1a00de15
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 119     blk_ht: 241920
            seconds: 1096436
     from 2013-06-16 21:37:34
       to 2013-06-29 14:11:30
     
          bits_hex: 0x1a00de15  | 0x1a  0xde15
         exp_const: 0xde150000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xc94e1fa453ded787120aba453ded787120aba453ded787120a
    calc_diff_compact: 0x1a00c94e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 120     blk_ht: 243936
            seconds: 986426
     from 2013-06-29 14:22:05
       to 2013-07-11 00:22:31
     
          bits_hex: 0x1a00c94e  | 0x1a  0xc94e
         exp_const: 0xc94e0000000000000000000000000000000000000000000000
    
    calc_diff_full: 0xa429db92c5f92c5f92c5f92c5f92c5f92c5f92c5f92c5f92c5
    calc_diff_compact: 0x1a00a429
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 121     blk_ht: 245952
            seconds: 1012494
     from 2013-07-11 00:22:43
       to 2013-07-22 17:37:37
     
          bits_hex: 0x1a00a429  | 0x1a  0xa429
         exp_const: 0xa4290000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x8968f7c71c71c71c71c71c71c71c71c71c71c71c71c71c71c7
    calc_diff_compact: 0x1a008968
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 122     blk_ht: 247968
            seconds: 1011116
     from 2013-07-22 17:57:07
       to 2013-08-03 10:49:03
     
          bits_hex: 0x1a008968  | 0x1a  0x8968
         exp_const: 0x89680000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x72dbf2e1b6a3f2c7b503d8c614e9d725fae8370bf9481d0a59
    calc_diff_compact: 0x1972dbf2
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 123     blk_ht: 249984
            seconds: 890179
     from 2013-08-03 10:51:55
       to 2013-08-13 18:08:14
     
          bits_hex: 0x1972dbf2  | 0x19  0x72dbf2
         exp_const: 0x72dbf200000000000000000000000000000000000000000000
    
    calc_diff_full: 0x548732d7def344899def344899def344899def344899def344
    calc_diff_compact: 0x19548732
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 124     blk_ht: 252000
            seconds: 934755
     from 2013-08-13 18:11:30
       to 2013-08-24 13:50:45
     
          bits_hex: 0x19548732  | 0x19  0x548732
         exp_const: 0x54873200000000000000000000000000000000000000000000
    
    calc_diff_full: 0x415257ca98c98c98c98c98c98c98c98c98c98c98c98c98c98c
    calc_diff_compact: 0x19415257
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 125     blk_ht: 254016
            seconds: 914857
     from 2013-08-24 14:08:39
       to 2013-09-04 04:16:16
     
          bits_hex: 0x19415257  | 0x19  0x415257
         exp_const: 0x41525700000000000000000000000000000000000000000000
    
    calc_diff_full: 0x31679c49d106aa0439dd376d106aa0439dd376d106aa0439dd
    calc_diff_compact: 0x1931679c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 126     blk_ht: 256032
            seconds: 933637
     from 2013-09-04 04:21:00
       to 2013-09-14 23:41:37
     
          bits_hex: 0x1931679c  | 0x19  0x31679c
         exp_const: 0x31679c00000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2622220c238ac1349bd245ace356bdf467cf0578e01689f127
    calc_diff_compact: 0x19262222
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 127     blk_ht: 258048
            seconds: 915443
     from 2013-09-14 23:44:08
       to 2013-09-25 14:01:31
     
          bits_hex: 0x19262222  | 0x19  0x262222
         exp_const: 0x26222200000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1cdc2008088f77e66d55c44b33a229118006ef5de4cd3bc2ab
    calc_diff_compact: 0x191cdc20
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 128     blk_ht: 260064
            seconds: 951028
     from 2013-09-25 14:09:06
       to 2013-10-06 14:19:34
     
          bits_hex: 0x191cdc20  | 0x19  0x1cdc20
         exp_const: 0x1cdc2000000000000000000000000000000000000000000000
    
    calc_diff_full: 0x16b0ca870b1b5c6070b1b5c6070b1b5c6070b1b5c6070b1b5c
    calc_diff_compact: 0x1916b0ca
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 129     blk_ht: 262080
            seconds: 855166
     from 2013-10-06 14:42:32
       to 2013-10-16 12:15:18
     
          bits_hex: 0x1916b0ca  | 0x19  0x16b0ca
         exp_const: 0x16b0ca00000000000000000000000000000000000000000000
    
    calc_diff_full: 0x100ab6498b8362e0d8b8362e0d8b8362e0d8b8362e0d8b8362
    calc_diff_compact: 0x19100ab6
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 130     blk_ht: 264096
            seconds: 828406
     from 2013-10-16 12:16:28
       to 2013-10-26 02:23:14
     
          bits_hex: 0x19100ab6  | 0x19  0x100ab6
         exp_const: 0x100ab600000000000000000000000000000000000000000000
    
    calc_diff_full: 0xafc85134838c7d0c1505949d8e1d2616a5ae9f2e3727b6bfb
    calc_diff_compact: 0x190afc85
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 131     blk_ht: 266112
            seconds: 925504
     from 2013-10-26 02:24:32
       to 2013-11-05 19:29:36
     
          bits_hex: 0x190afc85  | 0x19  0xafc85
         exp_const: 0xafc8500000000000000000000000000000000000000000000
    
    calc_diff_full: 0x867f310525a7afd0525a7afd0525a7afd0525a7afd0525a7a
    calc_diff_compact: 0x190867f3
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 132     blk_ht: 268128
            seconds: 1014009
     from 2013-11-05 19:52:03
       to 2013-11-17 13:32:12
     
          bits_hex: 0x190867f3  | 0x19  0x867f3
         exp_const: 0x867f300000000000000000000000000000000000000000000
    
    calc_diff_full: 0x70bfb3b9dfd130463796ac9dfd130463796ac9dfd13046379
    calc_diff_compact: 0x19070bfb
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 133     blk_ht: 270144
            seconds: 1042157
     from 2013-11-17 14:44:59
       to 2013-11-29 16:14:16
     
          bits_hex: 0x19070bfb  | 0x19  0x70bfb
         exp_const: 0x70bfb00000000000000000000000000000000000000000000
    
    calc_diff_full: 0x6124221c630a74eb92fd741b85fca40e852c970db51f963da
    calc_diff_compact: 0x19061242
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 134     blk_ht: 272160
            seconds: 942018
     from 2013-11-29 16:30:48
       to 2013-12-10 14:11:06
     
          bits_hex: 0x19061242  | 0x19  0x61242
         exp_const: 0x6124200000000000000000000000000000000000000000000
    
    calc_diff_full: 0x4ba6ea7333333333333333333333333333333333333333333
    calc_diff_compact: 0x1904ba6e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 135     blk_ht: 274176
            seconds: 930412
     from 2013-12-10 14:11:26
       to 2013-12-21 08:38:18
     
          bits_hex: 0x1904ba6e  | 0x19  0x4ba6e
         exp_const: 0x4ba6e00000000000000000000000000000000000000000000
    
    calc_diff_full: 0x3a30cd9111111111111111111111111111111111111111111
    calc_diff_compact: 0x1903a30c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 136     blk_ht: 276192
            seconds: 1007027
     from 2013-12-21 09:11:52
       to 2014-01-02 00:55:39
     
          bits_hex: 0x1903a30c  | 0x19  0x3a30c
         exp_const: 0x3a30c00000000000000000000000000000000000000000000
    
    calc_diff_full: 0x3071f9b4cfd585e0e696f1f7a80308b91419ca252adb363be
    calc_diff_compact: 0x1903071f
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 137     blk_ht: 278208
            seconds: 958789
     from 2014-01-02 00:58:38
       to 2014-01-13 03:18:27
     
          bits_hex: 0x1903071f  | 0x19  0x3071f
         exp_const: 0x3071f00000000000000000000000000000000000000000000
    
    calc_diff_full: 0x26666428d5af7d19f3c15e3805a27c49e6c08e2b04d26f491
    calc_diff_compact: 0x19026666
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 138     blk_ht: 280224
            seconds: 986691
     from 2014-01-13 03:20:20
       to 2014-01-24 13:25:11
     
          bits_hex: 0x19026666  | 0x19  0x26666
         exp_const: 0x2666600000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1f52cd3c5cf902c35f6929c5cf902c35f6929c5cf902c35f6
    calc_diff_compact: 0x1901f52c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 139     blk_ht: 282240
            seconds: 1012318
     from 2014-01-24 13:28:46
       to 2014-02-05 06:40:44
     
          bits_hex: 0x1901f52c  | 0x19  0x1f52c
         exp_const: 0x1f52c00000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1a36eab3788cde233788cde233788cde233788cde233788cd
    calc_diff_compact: 0x1901a36e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 140     blk_ht: 284256
            seconds: 1013191
     from 2014-02-05 07:14:16
       to 2014-02-17 00:40:47
     
          bits_hex: 0x1901a36e  | 0x19  0x1a36e
         exp_const: 0x1a36e00000000000000000000000000000000000000000000
    
    calc_diff_full: 0x15f532105e7d5f4d6c4e3c5b3d2b4a2c1a391b092809f816f
    calc_diff_compact: 0x19015f53
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 141     blk_ht: 286272
            seconds: 992091
     from 2014-02-17 00:43:59
       to 2014-02-28 12:18:50
     
          bits_hex: 0x19015f53  | 0x19  0x15f53
         exp_const: 0x15f5300000000000000000000000000000000000000000000
    
    calc_diff_full: 0x12026437b348014ce19ae67b348014ce19ae67b348014ce19
    calc_diff_compact: 0x19012026
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 142     blk_ht: 288288
            seconds: 1085950
     from 2014-02-28 12:29:45
       to 2014-03-13 02:08:55
     
          bits_hex: 0x19012026  | 0x19  0x12026
         exp_const: 0x1202600000000000000000000000000000000000000000000
    
    calc_diff_full: 0x102b15a84bda12f684bda12f684bda12f684bda12f684bda1
    calc_diff_compact: 0x190102b1
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 143     blk_ht: 290304
            seconds: 1026813
     from 2014-03-13 02:12:44
       to 2014-03-24 23:26:17
     
          bits_hex: 0x190102b1  | 0x19  0x102b1
         exp_const: 0x102b100000000000000000000000000000000000000000000
    
    calc_diff_full: 0xdb99809e79e79e79e79e79e79e79e79e79e79e79e79e79e7
    calc_diff_compact: 0x1900db99
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 144     blk_ht: 292320
            seconds: 989657
     from 2014-03-24 23:30:32
       to 2014-04-05 10:24:49
     
          bits_hex: 0x1900db99  | 0x19  0xdb99
         exp_const: 0xdb9900000000000000000000000000000000000000000000
    
    calc_diff_full: 0xb3aaff0fedcba987654320fedcba987654320fedcba98765
    calc_diff_compact: 0x1900b3aa
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 145     blk_ht: 294336
            seconds: 1060716
     from 2014-04-05 10:41:18
       to 2014-04-17 17:19:54
     
          bits_hex: 0x1900b3aa  | 0x19  0xb3aa
         exp_const: 0xb3aa00000000000000000000000000000000000000000000
    
    calc_diff_full: 0x9d8cd05c5291f5ec2b8f85c5291f5ec2b8f85c5291f5ec2b
    calc_diff_compact: 0x19009d8c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 146     blk_ht: 296352
            seconds: 1055108
     from 2014-04-17 17:27:26
       to 2014-04-29 22:32:34
     
          bits_hex: 0x19009d8c  | 0x19  0x9d8c
         exp_const: 0x9d8c00000000000000000000000000000000000000000000
    
    calc_diff_full: 0x896cbbe7f1b24e5818b4be7f1b24e5818b4be7f1b24e5818
    calc_diff_compact: 0x1900896c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 147     blk_ht: 298368
            seconds: 1093121
     from 2014-04-29 22:39:35
       to 2014-05-12 14:18:16
     
          bits_hex: 0x1900896c  | 0x19  0x896c
         exp_const: 0x896c00000000000000000000000000000000000000000000
    
    calc_diff_full: 0x7c30534c1f6ca174c1f6ca174c1f6ca174c1f6ca174c1f6c
    calc_diff_compact: 0x187c3053
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 148     blk_ht: 300384
            seconds: 1024233
     from 2014-05-12 14:18:31
       to 2014-05-24 10:49:04
     
          bits_hex: 0x187c3053  | 0x18  0x7c3053
         exp_const: 0x7c3053000000000000000000000000000000000000000000
    
    calc_diff_full: 0x692842cbcccccccccccccccccccccccccccccccccccccccc
    calc_diff_compact: 0x18692842
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 149     blk_ht: 302400
            seconds: 1075761
     from 2014-05-24 10:52:30
       to 2014-06-05 21:41:51
     
          bits_hex: 0x18692842  | 0x18  0x692842
         exp_const: 0x692842000000000000000000000000000000000000000000
    
    calc_diff_full: 0x5d859a77fd130463796ac9dfd130463796ac9dfd13046379
    calc_diff_compact: 0x185d859a
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 150     blk_ht: 304416
            seconds: 1056315
     from 2014-06-05 21:49:53
       to 2014-06-18 03:15:08
     
          bits_hex: 0x185d859a  | 0x18  0x5d859a
         exp_const: 0x5d859a000000000000000000000000000000000000000000
    
    calc_diff_full: 0x51aba20b1451451451451451451451451451451451451451
    calc_diff_compact: 0x1851aba2
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 151     blk_ht: 306432
            seconds: 968242
     from 2014-06-18 03:14:40
       to 2014-06-29 08:12:02
     
          bits_hex: 0x1851aba2  | 0x18  0x51aba2
         exp_const: 0x51aba2000000000000000000000000000000000000000000
    
    calc_diff_full: 0x415fd1fdd0369d0369d0369d0369d0369d0369d0369d0369
    calc_diff_compact: 0x18415fd1
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 152     blk_ht: 308448
            seconds: 1173468
     from 2014-06-29 08:12:36
       to 2014-07-12 22:10:24
     
          bits_hex: 0x18415fd1  | 0x18  0x415fd1
         exp_const: 0x415fd1000000000000000000000000000000000000000000
    
    calc_diff_full: 0x3f6be67a434a9b101767dce434a9b101767dce434a9b1017
    calc_diff_compact: 0x183f6be6
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 153     blk_ht: 310464
            seconds: 1119210
     from 2014-07-12 22:58:14
       to 2014-07-25 21:51:44
     
          bits_hex: 0x183f6be6  | 0x18  0x3f6be6
         exp_const: 0x3f6be6000000000000000000000000000000000000000000
    
    calc_diff_full: 0x3aaea2af8e38e38e38e38e38e38e38e38e38e38e38e38e38
    calc_diff_compact: 0x183aaea2
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 154     blk_ht: 312480
            seconds: 1148708
     from 2014-07-25 21:51:32
       to 2014-08-08 04:56:40
     
          bits_hex: 0x183aaea2  | 0x18  0x3aaea2
         exp_const: 0x3aaea2000000000000000000000000000000000000000000
    
    calc_diff_full: 0x37ba6263d779334ef0aac668223ddf99b557112cce88a446
    calc_diff_compact: 0x1837ba62
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 155     blk_ht: 314496
            seconds: 1000852
     from 2014-08-08 05:01:52
       to 2014-08-19 19:02:44
     
          bits_hex: 0x1837ba62  | 0x18  0x37ba62
         exp_const: 0x37ba62000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2e1c58c17530eca8641fdb97530eca8641fdb97530eca864
    calc_diff_compact: 0x182e1c58
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 156     blk_ht: 316512
            seconds: 1051548
     from 2014-08-19 19:11:58
       to 2014-08-31 23:17:46
     
          bits_hex: 0x182e1c58  | 0x18  0x2e1c58
         exp_const: 0x2e1c58000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2815eed3ee721a54d880bb3ee721a54d880bb3ee721a54d8
    calc_diff_compact: 0x182815ee
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 157     blk_ht: 318528
            seconds: 1112235
     from 2014-08-31 23:19:12
       to 2014-09-13 20:16:27
     
          bits_hex: 0x182815ee  | 0x18  0x2815ee
         exp_const: 0x2815ee000000000000000000000000000000000000000000
    
    calc_diff_full: 0x24dbe917e45e45e45e45e45e45e45e45e45e45e45e45e45e
    calc_diff_compact: 0x1824dbe9
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 158     blk_ht: 320544
            seconds: 1040986
     from 2014-09-13 20:08:16
       to 2014-09-25 21:18:02
     
          bits_hex: 0x1824dbe9  | 0x18  0x24dbe9
         exp_const: 0x24dbe9000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1fb893cd9ba8a9798687576465354243132020f0fdfecedb
    calc_diff_compact: 0x181fb893
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 159     blk_ht: 322560
            seconds: 1197814
     from 2014-09-25 21:21:20
       to 2014-10-09 18:04:54
     
          bits_hex: 0x181fb893  | 0x18  0x1fb893
         exp_const: 0x1fb893000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1f69731bc10aff9ee8dd7cc6bb5aa499388277166054f43e
    calc_diff_compact: 0x181f6973
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 160     blk_ht: 324576
            seconds: 1176553
     from 2014-10-09 18:04:26
       to 2014-10-23 08:53:39
     
          bits_hex: 0x181f6973  | 0x18  0x1f6973
         exp_const: 0x1f6973000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1e8dc0822d21c10aff9ee8dd7cc6bb5aa499388277166054
    calc_diff_compact: 0x181e8dc0
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 161     blk_ht: 326592
            seconds: 1099096
     from 2014-10-23 09:09:53
       to 2014-11-05 02:28:09
     
          bits_hex: 0x181e8dc0  | 0x18  0x1e8dc0
         exp_const: 0x1e8dc0000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1bc330077a113aad446de077a113aad446de077a113aad44
    calc_diff_compact: 0x181bc330
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 162     blk_ht: 328608
            seconds: 1188699
     from 2014-11-05 02:30:31
       to 2014-11-18 20:42:10
     
          bits_hex: 0x181bc330  | 0x18  0x1bc330
         exp_const: 0x1bc330000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1b4861699999999999999999999999999999999999999999
    calc_diff_compact: 0x181b4861
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 163     blk_ht: 330624
            seconds: 1218446
     from 2014-11-18 21:12:04
       to 2014-12-02 23:39:30
     
          bits_hex: 0x181b4861  | 0x18  0x1b4861
         exp_const: 0x1b4861000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1b7b74f09a322bab433cbc544dcd655ede766fef87810098
    calc_diff_compact: 0x181b7b74
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 164     blk_ht: 332640
            seconds: 1226455
     from 2014-12-02 23:41:45
       to 2014-12-17 04:22:40
     
          bits_hex: 0x181b7b74  | 0x18  0x1b7b74
         exp_const: 0x1b7b74000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1bdd7cd6af004559aaf004559aaf004559aaf004559aaf00
    calc_diff_compact: 0x181bdd7c
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 165     blk_ht: 334656
            seconds: 1174382
     from 2014-12-17 04:37:04
       to 2014-12-30 18:50:06
     
          bits_hex: 0x181bdd7c  | 0x18  0x1bdd7c
         exp_const: 0x1bdd7c000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1b0dca40de5ab277f44c118de5ab277f44c118de5ab277f4
    calc_diff_compact: 0x181b0dca
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 166     blk_ht: 336672
            seconds: 1117977
     from 2014-12-30 18:53:08
       to 2015-01-12 17:26:05
     
          bits_hex: 0x181b0dca  | 0x18  0x1b0dca
         exp_const: 0x1b0dca000000000000000000000000000000000000000000
    
    calc_diff_full: 0x19012f4137c048d159e26af37c048d159e26af37c048d159
    calc_diff_compact: 0x1819012f
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 167     blk_ht: 338688
            seconds: 1288695
     from 2015-01-12 17:34:33
       to 2015-01-27 15:32:48
     
          bits_hex: 0x1819012f  | 0x18  0x19012f
         exp_const: 0x19012f000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1aa3c0c7ff2ff2ff2ff2ff2ff2ff2ff2ff2ff2ff2ff2ff2f
    calc_diff_compact: 0x181aa3c0
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 168     blk_ht: 340704
            seconds: 1123006
     from 2015-01-27 15:35:46
       to 2015-02-09 15:32:32
     
          bits_hex: 0x181aa3c0  | 0x18  0x1aa3c0
         exp_const: 0x1aa3c0000000000000000000000000000000000000000000
    
    calc_diff_full: 0x18bb87fe4b17e4b17e4b17e4b17e4b17e4b17e4b17e4b17e
    calc_diff_compact: 0x1818bb87
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 169     blk_ht: 342720
            seconds: 1151848
     from 2015-02-09 15:40:15
       to 2015-02-22 23:37:43
     
          bits_hex: 0x1818bb87  | 0x18  0x18bb87
         exp_const: 0x18bb87000000000000000000000000000000000000000000
    
    calc_diff_full: 0x178d3afc5c8a0ce512956d9b1df623a67eac2f0734b78fbd
    calc_diff_compact: 0x18178d3a
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 170     blk_ht: 344736
            seconds: 1190646
     from 2015-02-22 23:48:57
       to 2015-03-08 18:33:03
     
          bits_hex: 0x18178d3a  | 0x18  0x178d3a
         exp_const: 0x178d3a000000000000000000000000000000000000000000
    
    calc_diff_full: 0x172ec03034a9b101767dce434a9b101767dce434a9b10176
    calc_diff_compact: 0x18172ec0
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 171     blk_ht: 346752
            seconds: 1227984
     from 2015-03-08 18:42:45
       to 2015-03-22 23:49:09
     
          bits_hex: 0x18172ec0  | 0x18  0x172ec0
         exp_const: 0x172ec0000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1788f2e33b007cd49a166e33b007cd49a166e33b007cd49a
    calc_diff_compact: 0x181788f2
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 172     blk_ht: 348768
            seconds: 1142845
     from 2015-03-22 23:53:31
       to 2015-04-05 05:20:56
     
          bits_hex: 0x181788f2  | 0x18  0x1788f2
         exp_const: 0x1788f2000000000000000000000000000000000000000000
    
    calc_diff_full: 0x163c71514e4a39f8f4e4a39f8f4e4a39f8f4e4a39f8f4e4a
    calc_diff_compact: 0x18163c71
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 173     blk_ht: 350784
            seconds: 1256242
     from 2015-04-05 05:22:25
       to 2015-04-19 18:19:47
     
          bits_hex: 0x18163c71  | 0x18  0x163c71
         exp_const: 0x163c71000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1717f0ed2632632632632632632632632632632632632632
    calc_diff_compact: 0x181717f0
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 174     blk_ht: 352800
            seconds: 1208767
     from 2015-04-19 18:25:06
       to 2015-05-03 18:11:13
     
          bits_hex: 0x181717f0  | 0x18  0x1717f0
         exp_const: 0x1717f0000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1713ddbf500611722833944a55b66c77d88e99fab0bc1cd2
    calc_diff_compact: 0x181713dd
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 175     blk_ht: 354816
            seconds: 1180751
     from 2015-05-03 18:22:21
       to 2015-05-17 10:21:32
     
          bits_hex: 0x181713dd  | 0x18  0x1713dd
         exp_const: 0x1713dd000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1686f5e6623a67eac2f0734b78fbd401845c8a0ce512956d
    calc_diff_compact: 0x181686f5
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 176     blk_ht: 356832
            seconds: 1240556
     from 2015-05-17 10:27:13
       to 2015-05-31 19:03:09
     
          bits_hex: 0x181686f5  | 0x18  0x1686f5
         exp_const: 0x1686f5000000000000000000000000000000000000000000
    
    calc_diff_full: 0x171a8b6ad5cb3a9186f64d42b208fe6dc4ba298075e53c31
    calc_diff_compact: 0x18171a8b
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 177     blk_ht: 358848
            seconds: 1158415
     from 2015-05-31 19:06:25
       to 2015-06-14 04:53:20
     
          bits_hex: 0x18171a8b  | 0x18  0x171a8b
         exp_const: 0x171a8b000000000000000000000000000000000000000000
    
    calc_diff_full: 0x162043e9f4e4a39f8f4e4a39f8f4e4a39f8f4e4a39f8f4e4
    calc_diff_compact: 0x18162043
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 178     blk_ht: 360864
            seconds: 1216710
     from 2015-06-14 04:56:03
       to 2015-06-28 06:54:33
     
          bits_hex: 0x18162043  | 0x18  0x162043
         exp_const: 0x162043000000000000000000000000000000000000000000
    
    calc_diff_full: 0x16418e5d8138138138138138138138138138138138138138
    calc_diff_compact: 0x1816418e
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 179     blk_ht: 362880
            seconds: 1169948
     from 2015-06-28 07:07:26
       to 2015-07-11 20:06:34
     
          bits_hex: 0x1816418e  | 0x18  0x16418e
         exp_const: 0x16418e000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1586c85cbb076c327ee3a9f65b216dd298e54a105cc187d4
    calc_diff_compact: 0x181586c8
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 180     blk_ht: 364896
            seconds: 1181790
     from 2015-07-11 20:24:46
       to 2015-07-25 12:41:16
     
          bits_hex: 0x181586c8  | 0x18  0x1586c8
         exp_const: 0x1586c8000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1508151db6db6db6db6db6db6db6db6db6db6db6db6db6db
    calc_diff_compact: 0x18150815
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 181     blk_ht: 366912
            seconds: 1199925
     from 2015-07-25 12:44:45
       to 2015-08-08 10:03:30
     
          bits_hex: 0x18150815  | 0x18  0x150815
         exp_const: 0x150815000000000000000000000000000000000000000000
    
    calc_diff_full: 0x14dd047379e79e79e79e79e79e79e79e79e79e79e79e79e7
    calc_diff_compact: 0x1814dd04
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 182     blk_ht: 368928
            seconds: 1174893
     from 2015-08-08 10:15:30
       to 2015-08-22 00:37:03
     
          bits_hex: 0x1814dd04  | 0x18  0x14dd04
         exp_const: 0x14dd04000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1443c41eebaebaebaebaebaebaebaebaebaebaebaebaebae
    calc_diff_compact: 0x181443c4
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 183     blk_ht: 370944
            seconds: 1152239
     from 2015-08-22 00:49:43
       to 2015-09-04 08:53:42
     
          bits_hex: 0x181443c4  | 0x18  0x1443c4
         exp_const: 0x1443c4000000000000000000000000000000000000000000
    
    calc_diff_full: 0x134dc113042bfe7ba375f31aed6a9264e209dc598153d0f8
    calc_diff_compact: 0x18134dc1
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 184     blk_ht: 372960
            seconds: 1161129
     from 2015-09-04 09:05:07
       to 2015-09-17 19:37:16
     
          bits_hex: 0x18134dc1  | 0x18  0x134dc1
         exp_const: 0x134dc1000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1287ba72f0123456789abcdf0123456789abcdf012345678
    calc_diff_compact: 0x181287ba
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 185     blk_ht: 374976
            seconds: 1180205
     from 2015-09-17 19:50:04
       to 2015-10-01 11:40:09
     
          bits_hex: 0x181287ba  | 0x18  0x1287ba
         exp_const: 0x1287ba000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1214727fbd5bd5bd5bd5bd5bd5bd5bd5bd5bd5bd5bd5bd5b
    calc_diff_compact: 0x18121472
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 186     blk_ht: 376992
            seconds: 1208198
     from 2015-10-01 11:53:10
       to 2015-10-15 11:29:48
     
          bits_hex: 0x18121472  | 0x18  0x121472
         exp_const: 0x121472000000000000000000000000000000000000000000
    
    calc_diff_full: 0x120f14a65082e60c3ea1c7fa5d83b6193f71d4fb2d90b6e9
    calc_diff_compact: 0x18120f14
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 187     blk_ht: 379008
            seconds: 1182978
     from 2015-10-15 11:32:31
       to 2015-10-29 04:08:49
     
          bits_hex: 0x18120f14  | 0x18  0x120f14
         exp_const: 0x120f14000000000000000000000000000000000000000000
    
    calc_diff_full: 0x11a95441c869536202ecfb9c869536202ecfb9c869536202
    calc_diff_compact: 0x1811a954
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 188     blk_ht: 381024
            seconds: 1143575
     from 2015-10-29 04:25:06
       to 2015-11-11 10:04:41
     
          bits_hex: 0x1811a954  | 0x18  0x11a954
         exp_const: 0x11a954000000000000000000000000000000000000000000
    
    calc_diff_full: 0x10b28904a28a28a28a28a28a28a28a28a28a28a28a28a28a
    calc_diff_compact: 0x1810b289
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 189     blk_ht: 383040
            seconds: 1095256
     from 2015-11-11 10:11:32
       to 2015-11-24 02:25:48
     
          bits_hex: 0x1810b289  | 0x18  0x10b289
         exp_const: 0x10b289000000000000000000000000000000000000000000
    
    calc_diff_full: 0xf1e762764d42b208fe6dc4ba298075e53c31a0f7ed5cb3a
    calc_diff_compact: 0x180f1e76
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 190     blk_ht: 385056
            seconds: 1112047
     from 2015-11-24 02:34:22
       to 2015-12-06 23:28:29
     
          bits_hex: 0x180f1e76  | 0x18  0xf1e76
         exp_const: 0xf1e76000000000000000000000000000000000000000000
    
    calc_diff_full: 0xde64fee58a368145f23d01adf8bd69b479257034e12bf09
    calc_diff_compact: 0x180de64f
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 191     blk_ht: 387072
            seconds: 1023902
     from 2015-12-06 23:30:52
       to 2015-12-18 19:55:54
     
          bits_hex: 0x180de64f  | 0x18  0xde64f
         exp_const: 0xde64f000000000000000000000000000000000000000000
    
    calc_diff_full: 0xbc409548fcb1ed40f631853a75c97eba0dc2fe520742964
    calc_diff_compact: 0x180bc409
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 192     blk_ht: 389088
            seconds: 1088132
     from 2015-12-18 20:08:09
       to 2015-12-31 10:23:41
     
          bits_hex: 0x180bc409  | 0x18  0xbc409
         exp_const: 0xbc409000000000000000000000000000000000000000000
    
    calc_diff_full: 0xa959110fe3649cb031697cfe3649cb031697cfe3649cb03
    calc_diff_compact: 0x180a9591
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 193     blk_ht: 391104
            seconds: 1108505
     from 2015-12-31 10:42:42
       to 2016-01-13 06:37:47
     
          bits_hex: 0x180a9591  | 0x18  0xa9591
         exp_const: 0xa9591000000000000000000000000000000000000000000
    
    calc_diff_full: 0x9b31bbf7afd0525a7afd0525a7afd0525a7afd0525a7afd
    calc_diff_compact: 0x1809b31b
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 194     blk_ht: 393120
            seconds: 1142295
     from 2016-01-13 06:39:38
       to 2016-01-26 11:57:53
     
          bits_hex: 0x1809b31b  | 0x18  0x9b31b
         exp_const: 0x9b31b000000000000000000000000000000000000000000
    
    calc_diff_full: 0x928f08ac71c71c71c71c71c71c71c71c71c71c71c71c71c
    calc_diff_compact: 0x180928f0
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 195     blk_ht: 395136
            seconds: 1007467
     from 2016-01-26 12:19:05
       to 2016-02-07 04:10:12
     
          bits_hex: 0x180928f0  | 0x18  0x928f0
         exp_const: 0x928f0000000000000000000000000000000000000000000
    
    calc_diff_full: 0x7a114d0804c4908d4d1915d5a19e5e2a26e6b2af6f3b37f
    calc_diff_compact: 0x1807a114
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 196     blk_ht: 397152
            seconds: 1066252
     from 2016-02-07 04:12:40
       to 2016-02-19 12:23:32
     
          bits_hex: 0x1807a114  | 0x18  0x7a114
         exp_const: 0x7a114000000000000000000000000000000000000000000
    
    calc_diff_full: 0x6b99f1f878100989211a9a322bab433cbc544dcd655ede7
    calc_diff_compact: 0x1806b99f
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 197     blk_ht: 399168
            seconds: 1248268
     from 2016-02-19 12:34:16
       to 2016-03-04 23:18:44
     
          bits_hex: 0x1806b99f  | 0x18  0x6b99f
         exp_const: 0x6b99f000000000000000000000000000000000000000000
    
    calc_diff_full: 0x6f0a83ca6dfc3518a6dfc3518a6dfc3518a6dfc3518a6df
    calc_diff_compact: 0x1806f0a8
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 198     blk_ht: 401184
            seconds: 1157929
     from 2016-03-04 23:25:56
       to 2016-03-18 09:04:45
     
          bits_hex: 0x1806f0a8  | 0x18  0x6f0a8
         exp_const: 0x6f0a8000000000000000000000000000000000000000000
    
    calc_diff_full: 0x6a4c316c01f3526859b8cec01f3526859b8cec01f352685
    calc_diff_compact: 0x1806a4c3
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 199     blk_ht: 403200
            seconds: 1199781
     from 2016-03-18 09:07:48
       to 2016-04-01 06:24:09
     
          bits_hex: 0x1806a4c3  | 0x18  0x6a4c3
         exp_const: 0x6a4c3000000000000000000000000000000000000000000
    
    calc_diff_full: 0x696f4a7b94b94b94b94b94b94b94b94b94b94b94b94b94b
    calc_diff_compact: 0x180696f4
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 200     blk_ht: 405216
            seconds: 1129537
     from 2016-04-01 06:34:35
       to 2016-04-14 08:20:12
     
          bits_hex: 0x180696f4  | 0x18  0x696f4
         exp_const: 0x696f4000000000000000000000000000000000000000000
    
    calc_diff_full: 0x6274b5685d307db285d307db285d307db285d307db285d3
    calc_diff_compact: 0x1806274b
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 201     blk_ht: 407232
            seconds: 1209731
     from 2016-04-14 08:25:41
       to 2016-04-28 08:27:52
     
          bits_hex: 0x1806274b  | 0x18  0x6274b
         exp_const: 0x6274b000000000000000000000000000000000000000000
    
    calc_diff_full: 0x62776acbefbefbefbefbefbefbefbefbefbefbefbefbefb
    calc_diff_compact: 0x18062776
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 202     blk_ht: 409248
            seconds: 1112491
     from 2016-04-28 08:28:30
       to 2016-05-11 05:30:01
     
          bits_hex: 0x18062776  | 0x18  0x62776
         exp_const: 0x62776000000000000000000000000000000000000000000
    
    calc_diff_full: 0x5a8fadc2d671ab5efa33e782bc700b44f893cd811c5609a
    calc_diff_compact: 0x1805a8fa
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 203     blk_ht: 411264
            seconds: 1178909
     from 2016-05-11 05:34:26
       to 2016-05-24 21:02:55
     
          bits_hex: 0x1805a8fa  | 0x18  0x5a8fa
         exp_const: 0x5a8fa000000000000000000000000000000000000000000
    
    calc_diff_full: 0x584363eddb441aa810e774ddb441aa810e774ddb441aa81
    calc_diff_compact: 0x18058436
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 204     blk_ht: 413280
            seconds: 1229655
     from 2016-05-24 21:02:46
       to 2016-06-08 02:37:01
     
          bits_hex: 0x18058436  | 0x18  0x58436
         exp_const: 0x58436000000000000000000000000000000000000000000
    
    calc_diff_full: 0x59ba00d05b05b05b05b05b05b05b05b05b05b05b05b05b0
    calc_diff_compact: 0x18059ba0
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 205     blk_ht: 415296
            seconds: 1132263
     from 2016-06-08 02:41:58
       to 2016-06-21 05:13:01
     
          bits_hex: 0x18059ba0  | 0x18  0x59ba0
         exp_const: 0x59ba0000000000000000000000000000000000000000000
    
    calc_diff_full: 0x53fd63ca5ca5ca5ca5ca5ca5ca5ca5ca5ca5ca5ca5ca5ca
    calc_diff_compact: 0x18053fd6
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 206     blk_ht: 417312
            seconds: 1187237
     from 2016-06-21 05:18:58
       to 2016-07-04 23:06:15
     
          bits_hex: 0x18053fd6  | 0x18  0x53fd6
         exp_const: 0x53fd6000000000000000000000000000000000000000000
    
    calc_diff_full: 0x526fdbed159e26af37c048d159e26af37c048d159e26af3
    calc_diff_compact: 0x180526fd
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 207     blk_ht: 419328
            seconds: 1209071
     from 2016-07-04 23:16:01
       to 2016-07-18 23:07:12
     
          bits_hex: 0x180526fd  | 0x18  0x526fd
         exp_const: 0x526fd000000000000000000000000000000000000000000
    
    calc_diff_full: 0x526695442b208fe6dc4ba298075e53c31a0f7ed5cb3a918
    calc_diff_compact: 0x18052669
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 208     blk_ht: 421344
            seconds: 1279095
     from 2016-07-18 23:22:42
       to 2016-08-02 18:40:57
     
          bits_hex: 0x18052669  | 0x18  0x52669
         exp_const: 0x52669000000000000000000000000000000000000000000
    
    calc_diff_full: 0x5722817c64c64c64c64c64c64c64c64c64c64c64c64c64c
    calc_diff_compact: 0x18057228
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 209     blk_ht: 423360
            seconds: 1123451
     from 2016-08-02 18:50:42
       to 2016-08-15 18:54:53
     
          bits_hex: 0x18057228  | 0x18  0x57228
         exp_const: 0x57228000000000000000000000000000000000000000000
    
    calc_diff_full: 0x50edceac5f92c5f92c5f92c5f92c5f92c5f92c5f92c5f92
    calc_diff_compact: 0x18050edc
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 210     blk_ht: 425376
            seconds: 1191079
     from 2016-08-15 18:59:14
       to 2016-08-29 13:50:33
     
          bits_hex: 0x18050edc  | 0x18  0x50edc
         exp_const: 0x50edc000000000000000000000000000000000000000000
    
    calc_diff_full: 0x4fb086ce01689f1279b0238ac1349bd245ace356bdf467c
    calc_diff_compact: 0x1804fb08
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 211     blk_ht: 427392
            seconds: 1182409
     from 2016-08-29 14:11:01
       to 2016-09-12 06:37:50
     
          bits_hex: 0x1804fb08  | 0x18  0x4fb08
         exp_const: 0x4fb08000000000000000000000000000000000000000000
    
    calc_diff_full: 0x4de5e90f24ae069c257e139cf58b146d028be47a035bf17
    calc_diff_compact: 0x1804de5e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 212     blk_ht: 429408
            seconds: 1132409
     from 2016-09-12 06:39:07
       to 2016-09-25 09:12:36
     
          bits_hex: 0x1804de5e  | 0x18  0x4de5e
         exp_const: 0x4de5e000000000000000000000000000000000000000000
    
    calc_diff_full: 0x48ed472e278d237ce278d237ce278d237ce278d237ce278
    calc_diff_compact: 0x18048ed4
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 213     blk_ht: 431424
            seconds: 1128680
     from 2016-09-25 09:16:55
       to 2016-10-08 10:48:15
     
          bits_hex: 0x18048ed4  | 0x18  0x48ed4
         exp_const: 0x48ed4000000000000000000000000000000000000000000
    
    calc_diff_full: 0x440c4fceb240795ceb240795ceb240795ceb240795ceb24
    calc_diff_compact: 0x180440c4
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 214     blk_ht: 433440
            seconds: 1232994
     from 2016-10-08 10:53:30
       to 2016-10-22 17:23:24
     
          bits_hex: 0x180440c4  | 0x18  0x440c4
         exp_const: 0x440c4000000000000000000000000000000000000000000
    
    calc_diff_full: 0x455d2997e4b17e4b17e4b17e4b17e4b17e4b17e4b17e4b1
    calc_diff_compact: 0x180455d2
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 215     blk_ht: 435456
            seconds: 1204842
     from 2016-10-22 18:02:58
       to 2016-11-05 16:43:40
     
          bits_hex: 0x180455d2  | 0x18  0x455d2
         exp_const: 0x455d2000000000000000000000000000000000000000000
    
    calc_diff_full: 0x451746db634fce968301c9b634fce968301c9b634fce968
    calc_diff_compact: 0x18045174
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 216     blk_ht: 437472
            seconds: 1092930
     from 2016-11-05 16:46:58
       to 2016-11-18 08:22:28
     
          bits_hex: 0x18045174  | 0x18  0x45174
         exp_const: 0x45174000000000000000000000000000000000000000000
    
    calc_diff_full: 0x3e6d414cccccccccccccccccccccccccccccccccccccccc
    calc_diff_compact: 0x1803e6d4
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 217     blk_ht: 439488
            seconds: 1188659
     from 2016-11-18 08:30:15
       to 2016-12-02 02:41:14
     
          bits_hex: 0x1803e6d4  | 0x18  0x3e6d4
         exp_const: 0x3e6d4000000000000000000000000000000000000000000
    
    calc_diff_full: 0x3d5893c6c5c1b1706c5c1b1706c5c1b1706c5c1b1706c5c
    calc_diff_compact: 0x1803d589
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 218     blk_ht: 441504
            seconds: 1118387
     from 2016-12-02 02:46:26
       to 2016-12-15 01:26:13
     
          bits_hex: 0x1803d589  | 0x18  0x3d589
         exp_const: 0x3d589000000000000000000000000000000000000000000
    
    calc_diff_full: 0x38b851cebbc99a778556334111eefccdaab889667445223
    calc_diff_compact: 0x18038b85
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 219     blk_ht: 443520
            seconds: 1180914
     from 2016-12-15 01:28:33
       to 2016-12-28 17:30:27
     
          bits_hex: 0x18038b85  | 0x18  0x38b85
         exp_const: 0x38b85000000000000000000000000000000000000000000
    
    calc_diff_full: 0x375ff591c71c71c71c71c71c71c71c71c71c71c71c71c71
    calc_diff_compact: 0x180375ff
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 220     blk_ht: 445536
            seconds: 1140624
     from 2016-12-28 17:40:55
       to 2017-01-10 22:31:19
     
          bits_hex: 0x180375ff  | 0x18  0x375ff
         exp_const: 0x375ff000000000000000000000000000000000000000000
    
    calc_diff_full: 0x34379250750750750750750750750750750750750750750
    calc_diff_compact: 0x18034379
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 221     blk_ht: 447552
            seconds: 1037031
     from 2017-01-10 22:40:52
       to 2017-01-22 22:44:43
     
          bits_hex: 0x18034379  | 0x18  0x34379
         exp_const: 0x34379000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2cc476ab0ae3e1714a47d7b0ae3e1714a47d7b0ae3e1714
    calc_diff_compact: 0x1802cc47
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 222     blk_ht: 449568
            seconds: 1125918
     from 2017-01-22 22:52:52
       to 2017-02-04 23:38:10
     
          bits_hex: 0x1802cc47  | 0x18  0x2cc47
         exp_const: 0x2cc47000000000000000000000000000000000000000000
    
    calc_diff_full: 0x29ab9631cc4ff832b65e991cc4ff832b65e991cc4ff832b
    calc_diff_compact: 0x18029ab9
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 223     blk_ht: 451584
            seconds: 1158538
     from 2017-02-04 23:38:49
       to 2017-02-18 09:27:47
     
          bits_hex: 0x18029ab9  | 0x18  0x29ab9
         exp_const: 0x29ab9000000000000000000000000000000000000000000
    
    calc_diff_full: 0x27e93e143e32d21c10aff9ee8dd7cc6bb5aa49938827716
    calc_diff_compact: 0x18027e93
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 224     blk_ht: 453600
            seconds: 1157127
     from 2017-02-18 09:38:26
       to 2017-03-03 19:03:53
     
          bits_hex: 0x18027e93  | 0x18  0x27e93
         exp_const: 0x27e93000000000000000000000000000000000000000000
    
    calc_diff_full: 0x262df614104104104104104104104104104104104104104
    calc_diff_compact: 0x180262df
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 225     blk_ht: 455616
            seconds: 1171626
     from 2017-03-03 19:04:46
       to 2017-03-17 08:31:52
     
          bits_hex: 0x180262df  | 0x18  0x262df
         exp_const: 0x262df000000000000000000000000000000000000000000
    
    calc_diff_full: 0x24fb189224bbe557ef188b224bbe557ef188b224bbe557e
    calc_diff_compact: 0x18024fb1
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 226     blk_ht: 457632
            seconds: 1151672
     from 2017-03-17 08:36:15
       to 2017-03-30 16:30:47
     
          bits_hex: 0x18024fb1  | 0x18  0x24fb1
         exp_const: 0x24fb1000000000000000000000000000000000000000000
    
    calc_diff_full: 0x2335aed0c83fb72ea61d950c83fb72ea61d950c83fb72ea
    calc_diff_compact: 0x1802335a
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 227     blk_ht: 459648
            seconds: 1160433
     from 2017-03-30 16:39:08
       to 2017-04-13 02:59:41
     
          bits_hex: 0x1802335a  | 0x18  0x2335a
         exp_const: 0x2335a000000000000000000000000000000000000000000
    
    calc_diff_full: 0x21c73ecb277f44c118de5ab277f44c118de5ab277f44c11
    calc_diff_compact: 0x18021c73
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 228     blk_ht: 461664
            seconds: 1206901
     from 2017-04-13 02:59:50
       to 2017-04-27 02:14:51
     
          bits_hex: 0x18021c73  | 0x18  0x21c73
         exp_const: 0x21c73000000000000000000000000000000000000000000
    
    calc_diff_full: 0x21b3e49480f2b9d6480f2b9d6480f2b9d6480f2b9d6480f
    calc_diff_compact: 0x18021b3e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 229     blk_ht: 463680
            seconds: 1127529
     from 2017-04-27 02:20:01
       to 2017-05-10 03:32:10
     
          bits_hex: 0x18021b3e  | 0x18  0x21b3e
         exp_const: 0x21b3e000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1f6a79eb7584250f1dbea8b7584250f1dbea8b7584250f1
    calc_diff_compact: 0x1801f6a7
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 230     blk_ht: 465696
            seconds: 1136627
     from 2017-05-10 03:40:48
       to 2017-05-23 07:24:35
     
          bits_hex: 0x1801f6a7  | 0x18  0x1f6a7
         exp_const: 0x1f6a7000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1d85406ccf66900299c335ccf66900299c335ccf6690029
    calc_diff_compact: 0x1801d854
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 231     blk_ht: 467712
            seconds: 1061984
     from 2017-05-23 07:29:52
       to 2017-06-04 14:29:36
     
          bits_hex: 0x1801d854  | 0x18  0x1d854
         exp_const: 0x1d854000000000000000000000000000000000000000000
    
    calc_diff_full: 0x19eafc50672894ab6cd8efb11d33f5617839a5bc7dea00c
    calc_diff_compact: 0x18019eaf
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 232     blk_ht: 469728
            seconds: 1153621
     from 2017-06-04 14:35:07
       to 2017-06-17 23:02:08
     
          bits_hex: 0x18019eaf  | 0x18  0x19eaf
         exp_const: 0x19eaf000000000000000000000000000000000000000000
    
    calc_diff_full: 0x18b7e1313b8b302a7a1f19690e0857fcf746ebe635dad52
    calc_diff_compact: 0x18018b7e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 233     blk_ht: 471744
            seconds: 1214793
     from 2017-06-17 23:18:53
       to 2017-07-02 00:45:26
     
          bits_hex: 0x18018b7e  | 0x18  0x18b7e
         exp_const: 0x18b7e000000000000000000000000000000000000000000
    
    calc_diff_full: 0x18d30aa2cdc67600f9a9342cdc67600f9a9342cdc67600f
    calc_diff_compact: 0x18018d30
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 234     blk_ht: 473760
            seconds: 1065472
     from 2017-07-02 00:47:17
       to 2017-07-14 08:45:09
     
          bits_hex: 0x18018d30  | 0x18  0x18d30
         exp_const: 0x18d30000000000000000000000000000000000000000000
    
    calc_diff_full: 0x15ddc7a7251cfc7a7251cfc7a7251cfc7a7251cfc7a7251
    calc_diff_compact: 0x18015ddc
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 235     blk_ht: 475776
            seconds: 1131293
     from 2017-07-14 08:45:42
       to 2017-07-27 11:00:35
     
          bits_hex: 0x18015ddc  | 0x18  0x15ddc
         exp_const: 0x15ddc000000000000000000000000000000000000000000
    
    calc_diff_full: 0x14735cfe24f3604715826937a48b59c6ad7be8cf9e0af1c
    calc_diff_compact: 0x18014735
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 236     blk_ht: 477792
            seconds: 1127057
     from 2017-07-27 11:03:54
       to 2017-08-09 12:08:11
     
          bits_hex: 0x18014735  | 0x18  0x14735
         exp_const: 0x14735000000000000000000000000000000000000000000
    
    calc_diff_full: 0x130e0e233bde689133bde689133bde689133bde689133bd
    calc_diff_compact: 0x180130e0
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 237     blk_ht: 479808
            seconds: 1257361
     from 2017-08-09 12:36:50
       to 2017-08-24 01:52:51
     
          bits_hex: 0x180130e0  | 0x18  0x130e0
         exp_const: 0x130e0000000000000000000000000000000000000000000
    
    calc_diff_full: 0x13ce9b8bf258bf258bf258bf258bf258bf258bf258bf258
    calc_diff_compact: 0x18013ce9
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 238     blk_ht: 481824
            seconds: 1164310
     from 2017-08-24 01:57:37
       to 2017-09-06 13:22:47
     
          bits_hex: 0x18013ce9  | 0x18  0x13ce9
         exp_const: 0x13ce9000000000000000000000000000000000000000000
    
    calc_diff_full: 0x1310b5b82d82d82d82d82d82d82d82d82d82d82d82d82d8
    calc_diff_compact: 0x1801310b
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 239     blk_ht: 483840
            seconds: 1011542
     from 2017-09-06 13:23:15
       to 2017-09-18 06:22:17
     
          bits_hex: 0x1801310b  | 0x18  0x1310b
         exp_const: 0x1310b000000000000000000000000000000000000000000
    
    calc_diff_full: 0xff1880a9264e209dc598153d0f8cb487042bfe7ba375f3
    calc_diff_compact: 0x1800ff18
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 240     blk_ht: 485856
            seconds: 1187580
     from 2017-09-18 06:31:16
       to 2017-10-02 00:24:16
     
          bits_hex: 0x1800ff18  | 0x18  0xff18
         exp_const: 0xff18000000000000000000000000000000000000000000
    
    calc_diff_full: 0xfa732ea0ea0ea0ea0ea0ea0ea0ea0ea0ea0ea0ea0ea0ea
    calc_diff_compact: 0x1800fa73
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 241     blk_ht: 487872
            seconds: 1135896
     from 2017-10-02 00:27:46
       to 2017-10-15 03:59:22
     
          bits_hex: 0x1800fa73  | 0x18  0xfa73
         exp_const: 0xfa73000000000000000000000000000000000000000000
    
    calc_diff_full: 0xeb304f6a76a76a76a76a76a76a76a76a76a76a76a76a76
    calc_diff_compact: 0x1800eb30
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 242     blk_ht: 489888
            seconds: 996423
     from 2017-10-15 04:05:02
       to 2017-10-26 16:52:05
     
          bits_hex: 0x1800eb30  | 0x18  0xeb30
         exp_const: 0xeb30000000000000000000000000000000000000000000
    
    calc_diff_full: 0xc1bd162d2f9fc6c9396062d2f9fc6c9396062d2f9fc6c9
    calc_diff_compact: 0x1800c1bd
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 243     blk_ht: 491904
            seconds: 1287999
     from 2017-10-26 16:52:42
       to 2017-11-10 14:39:21
     
          bits_hex: 0x1800c1bd  | 0x18  0xc1bd
         exp_const: 0xc1bd000000000000000000000000000000000000000000
    
    calc_diff_full: 0xce4b94516eb084a1e3b7d516eb084a1e3b7d516eb084a1
    calc_diff_compact: 0x1800ce4b
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 244     blk_ht: 493920
            seconds: 1225251
     from 2017-11-10 15:13:51
       to 2017-11-24 19:34:42
     
          bits_hex: 0x1800ce4b  | 0x18  0xce4b
         exp_const: 0xce4b000000000000000000000000000000000000000000
    
    calc_diff_full: 0xd0f65226859b8cec01f3526859b8cec01f3526859b8cec
    calc_diff_compact: 0x1800d0f6
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 245     blk_ht: 495936
            seconds: 1024166
     from 2017-11-24 19:53:16
       to 2017-12-06 16:22:42
     
          bits_hex: 0x1800d0f6  | 0x18  0xd0f6
         exp_const: 0xd0f6000000000000000000000000000000000000000000
    
    calc_diff_full: 0xb0ed44672894ab6cd8efb11d33f5617839a5bc7dea00c2
    calc_diff_compact: 0x1800b0ed
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 246     blk_ht: 497952
            seconds: 1027377
     from 2017-12-06 16:23:21
       to 2017-12-18 13:46:18
     
          bits_hex: 0x1800b0ed  | 0x18  0xb0ed
         exp_const: 0xb0ed000000000000000000000000000000000000000000
    
    calc_diff_full: 0x9645bb1a54d880bb3ee721a54d880bb3ee721a54d880bb
    calc_diff_compact: 0x18009645
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 247     blk_ht: 499968
            seconds: 1173260
     from 2017-12-18 13:55:20
       to 2018-01-01 03:49:40
     
          bits_hex: 0x18009645  | 0x18  0x9645
         exp_const: 0x9645000000000000000000000000000000000000000000
    
    calc_diff_full: 0x91c146b7ab7ab7ab7ab7ab7ab7ab7ab7ab7ab7ab7ab7ab
    calc_diff_compact: 0x180091c1
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 248     blk_ht: 501984
            seconds: 1048502
     from 2018-01-01 03:56:10
       to 2018-01-13 07:11:12
     
          bits_hex: 0x180091c1  | 0x18  0x91c1
         exp_const: 0x91c1000000000000000000000000000000000000000000
    
    calc_diff_full: 0x7e578c2cc0a9e87c65a438215ff3dd1baf98d76b549327
    calc_diff_compact: 0x177e578c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 249     blk_ht: 504000
            seconds: 1035238
     from 2018-01-13 07:12:34
       to 2018-01-25 06:46:32
     
          bits_hex: 0x177e578c  | 0x17  0x7e578c
         exp_const: 0x7e578c0000000000000000000000000000000000000000
    
    calc_diff_full: 0x6c2146174916b38d5af7d19f3c15e3805a27c49e6c08e2
    calc_diff_compact: 0x176c2146
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 250     blk_ht: 506016
            seconds: 1095318
     from 2018-01-25 06:48:20
       to 2018-02-06 23:03:38
     
          bits_hex: 0x176c2146  | 0x17  0x6c2146
         exp_const: 0x6c21460000000000000000000000000000000000000000
    
    calc_diff_full: 0x61e9f844369d0369d0369d0369d0369d0369d0369d0369
    calc_diff_compact: 0x1761e9f8
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 251     blk_ht: 508032
            seconds: 1156223
     from 2018-02-06 23:08:07
       to 2018-02-20 08:18:30
     
          bits_hex: 0x1761e9f8  | 0x17  0x61e9f8
         exp_const: 0x61e9f80000000000000000000000000000000000000000
    
    calc_diff_full: 0x5d97dcc7b572cea461bd9350ac823f9b712e8a601d794f
    calc_diff_compact: 0x175d97dc
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 252     blk_ht: 510048
            seconds: 1105490
     from 2018-02-20 08:20:59
       to 2018-03-05 03:25:49
     
          bits_hex: 0x175d97dc  | 0x17  0x5d97dc
         exp_const: 0x5d97dc0000000000000000000000000000000000000000
    
    calc_diff_full: 0x5589a3c12d3d7e8292d3d7e8292d3d7e8292d3d7e8292d
    calc_diff_compact: 0x175589a3
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 253     blk_ht: 512064
            seconds: 1149536
     from 2018-03-05 04:21:18
       to 2018-03-18 11:40:14
     
          bits_hex: 0x175589a3  | 0x17  0x5589a3
         exp_const: 0x5589a30000000000000000000000000000000000000000
    
    calc_diff_full: 0x514a492b3e0935e8b3e0935e8b3e0935e8b3e0935e8b3e
    calc_diff_compact: 0x17514a49
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 254     blk_ht: 514080
            seconds: 1192885
     from 2018-03-18 11:40:18
       to 2018-04-01 07:01:43
     
          bits_hex: 0x17514a49  | 0x17  0x514a49
         exp_const: 0x514a490000000000000000000000000000000000000000
    
    calc_diff_full: 0x502ab73d6b46b46b46b46b46b46b46b46b46b46b46b46b
    calc_diff_compact: 0x17502ab7
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 255     blk_ht: 516096
            seconds: 1106181
     from 2018-04-01 07:05:57
       to 2018-04-14 02:22:18
     
          bits_hex: 0x17502ab7  | 0x17  0x502ab7
         exp_const: 0x502ab70000000000000000000000000000000000000000
    
    calc_diff_full: 0x49500da2bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
    calc_diff_compact: 0x1749500d
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 256     blk_ht: 518112
            seconds: 1154642
     from 2018-04-14 02:28:52
       to 2018-04-27 11:12:54
     
          bits_hex: 0x1749500d  | 0x17  0x49500d
         exp_const: 0x49500d0000000000000000000000000000000000000000
    
    calc_diff_full: 0x45fb53be7382f3eafa6b6271e2d9e95a5160d1c8d84940
    calc_diff_compact: 0x1745fb53
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 257     blk_ht: 520128
            seconds: 1174041
     from 2018-04-27 11:24:13
       to 2018-05-11 01:31:34
     
          bits_hex: 0x1745fb53  | 0x17  0x45fb53
         exp_const: 0x45fb530000000000000000000000000000000000000000
    
    calc_diff_full: 0x43eca9637ae147ae147ae147ae147ae147ae147ae147ae
    calc_diff_compact: 0x1743eca9
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 258     blk_ht: 522144
            seconds: 1163802
     from 2018-05-11 01:54:15
       to 2018-05-24 13:10:57
     
          bits_hex: 0x1743eca9  | 0x17  0x43eca9
         exp_const: 0x43eca90000000000000000000000000000000000000000
    
    calc_diff_full: 0x415a49f6180e4db1a7e74b4180e4db1a7e74b4180e4db1
    calc_diff_compact: 0x17415a49
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 259     blk_ht: 524160
            seconds: 1054442
     from 2018-05-24 13:20:53
       to 2018-06-05 18:14:55
     
          bits_hex: 0x17415a49  | 0x17  0x415a49
         exp_const: 0x415a490000000000000000000000000000000000000000
    
    calc_diff_full: 0x38f84162603936c69f9d2d0603936c69f9d2d0603936c6
    calc_diff_compact: 0x1738f841
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 260     blk_ht: 526176
            seconds: 1177012
     from 2018-06-05 18:18:06
       to 2018-06-19 09:14:58
     
          bits_hex: 0x1738f841  | 0x17  0x38f841
         exp_const: 0x38f8410000000000000000000000000000000000000000
    
    calc_diff_full: 0x376f566769d0369d0369d0369d0369d0369d0369d0369d
    calc_diff_compact: 0x17376f56
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 261     blk_ht: 528192
            seconds: 1145062
     from 2018-06-19 09:20:45
       to 2018-07-02 15:25:07
     
          bits_hex: 0x17376f56  | 0x17  0x376f56
         exp_const: 0x376f560000000000000000000000000000000000000000
    
    calc_diff_full: 0x347a28debf78c459125df2abf78c459125df2abf78c459
    calc_diff_compact: 0x17347a28
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 262     blk_ht: 530208
            seconds: 1252813
     from 2018-07-02 15:34:21
       to 2018-07-17 03:34:34
     
          bits_hex: 0x17347a28  | 0x17  0x347a28
         exp_const: 0x347a280000000000000000000000000000000000000000
    
    calc_diff_full: 0x365a17887b18c29d3ae4bf5d06e17f2903a14b25c36d47
    calc_diff_compact: 0x17365a17
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 263     blk_ht: 532224
            seconds: 1052893
     from 2018-07-17 03:50:49
       to 2018-07-29 08:19:02
     
          bits_hex: 0x17365a17  | 0x17  0x365a17
         exp_const: 0x365a170000000000000000000000000000000000000000
    
    calc_diff_full: 0x2f4f7b376b0f397c204a8d315b9e426caf537dc0648ed1
    calc_diff_compact: 0x172f4f7b
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 264     blk_ht: 534240
            seconds: 1126324
     from 2018-07-29 08:19:31
       to 2018-08-11 09:11:35
     
          bits_hex: 0x172f4f7b  | 0x17  0x2f4f7b
         exp_const: 0x2f4f7b0000000000000000000000000000000000000000
    
    calc_diff_full: 0x2c0da79a8c1bf4f2825b58e8c1bf4f2825b58e8c1bf4f2
    calc_diff_compact: 0x172c0da7
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 265     blk_ht: 536256
            seconds: 1148842
     from 2018-08-11 09:40:59
       to 2018-08-24 16:48:21
     
          bits_hex: 0x172c0da7  | 0x17  0x2c0da7
         exp_const: 0x2c0da70000000000000000000000000000000000000000
    
    calc_diff_full: 0x29d72d84ae59d48c37b26a159047f36e25d14c03af29e1
    calc_diff_compact: 0x1729d72d
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 266     blk_ht: 538272
            seconds: 1159285
     from 2018-08-24 16:50:31
       to 2018-09-07 02:51:56
     
          bits_hex: 0x1729d72d  | 0x17  0x29d72d
         exp_const: 0x29d72d0000000000000000000000000000000000000000
    
    calc_diff_full: 0x2819a1256a314dbf86a314dbf86a314dbf86a314dbf86a
    calc_diff_compact: 0x172819a1
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 267     blk_ht: 540288
            seconds: 1187035
     from 2018-09-07 03:14:39
       to 2018-09-20 20:58:34
     
          bits_hex: 0x172819a1  | 0x17  0x2819a1
         exp_const: 0x2819a10000000000000000000000000000000000000000
    
    calc_diff_full: 0x275a1fd5210cbb766210cbb766210cbb766210cbb76621
    calc_diff_compact: 0x17275a1f
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 268     blk_ht: 542304
            seconds: 1160545
     from 2018-09-20 21:15:39
       to 2018-10-04 07:38:04
     
          bits_hex: 0x17275a1f  | 0x17  0x275a1f
         exp_const: 0x275a1f0000000000000000000000000000000000000000
    
    calc_diff_full: 0x25c1915e425ed097b425ed097b425ed097b425ed097b42
    calc_diff_compact: 0x1725c191
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 269     blk_ht: 544320
            seconds: 1255425
     from 2018-10-04 07:49:22
       to 2018-10-18 20:33:07
     
          bits_hex: 0x1725c191  | 0x17  0x25c191
         exp_const: 0x25c1910000000000000000000000000000000000000000
    
    calc_diff_full: 0x272fbda6e79e79e79e79e79e79e79e79e79e79e79e79e7
    calc_diff_compact: 0x17272fbd
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 270     blk_ht: 546336
            seconds: 1209339
     from 2018-10-18 20:37:47
       to 2018-11-01 20:33:26
     
          bits_hex: 0x17272fbd  | 0x17  0x272fbd
         exp_const: 0x272fbd0000000000000000000000000000000000000000
    
    calc_diff_full: 0x272d92ddaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
    calc_diff_compact: 0x17272d92
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 271     blk_ht: 548352
            seconds: 1306157
     from 2018-11-01 20:54:16
       to 2018-11-16 23:43:33
     
          bits_hex: 0x17272d92  | 0x17  0x272d92
         exp_const: 0x272d920000000000000000000000000000000000000000
    
    calc_diff_full: 0x2a4e2fb7fa0a4b4f5fa0a4b4f5fa0a4b4f5fa0a4b4f5fa
    calc_diff_compact: 0x172a4e2f
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 272     blk_ht: 550368
            seconds: 1425303
     from 2018-11-16 23:51:24
       to 2018-12-03 11:46:27
     
          bits_hex: 0x172a4e2f  | 0x17  0x2a4e2f
         exp_const: 0x2a4e2f0000000000000000000000000000000000000000
    
    calc_diff_full: 0x31d97c5a4d880bb3ee721a54d880bb3ee721a54d880bb3
    calc_diff_compact: 0x1731d97c
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 273     blk_ht: 552384
            seconds: 1337510
     from 2018-12-03 11:59:28
       to 2018-12-18 23:31:18
     
          bits_hex: 0x1731d97c  | 0x17  0x31d97c
         exp_const: 0x31d97c0000000000000000000000000000000000000000
    
    calc_diff_full: 0x371ef4dc51451451451451451451451451451451451451
    calc_diff_compact: 0x17371ef4
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 274     blk_ht: 554400
            seconds: 1099337
     from 2018-12-18 23:32:45
       to 2018-12-31 16:55:02
     
          bits_hex: 0x17371ef4  | 0x17  0x371ef4
         exp_const: 0x371ef40000000000000000000000000000000000000000
    
    calc_diff_full: 0x3218a59a71ab5efa33e782bc700b44f893cd811c5609a4
    calc_diff_compact: 0x173218a5
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 275     blk_ht: 556416
            seconds: 1155042
     from 2018-12-31 17:20:09
       to 2019-01-14 02:10:51
     
          bits_hex: 0x173218a5  | 0x17  0x3218a5
         exp_const: 0x3218a50000000000000000000000000000000000000000
    
    calc_diff_full: 0x2fd6331abf258bf258bf258bf258bf258bf258bf258bf2
    calc_diff_compact: 0x172fd633
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 276     blk_ht: 558432
            seconds: 1224022
     from 2019-01-14 02:19:54
       to 2019-01-28 06:20:16
     
          bits_hex: 0x172fd633  | 0x17  0x2fd633
         exp_const: 0x2fd6330000000000000000000000000000000000000000
    
    calc_diff_full: 0x306835c4f37c048d159e26af37c048d159e26af37c048d
    calc_diff_compact: 0x17306835
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 277     blk_ht: 560448
            seconds: 1160339
     from 2019-01-28 06:35:13
       to 2019-02-10 16:54:12
     
          bits_hex: 0x17306835  | 0x17  0x306835
         exp_const: 0x3068350000000000000000000000000000000000000000
    
    calc_diff_full: 0x2e6f88b3935e8b3e0935e8b3e0935e8b3e0935e8b3e093
    calc_diff_compact: 0x172e6f88
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 278     blk_ht: 562464
            seconds: 1207543
     from 2019-02-10 16:59:41
       to 2019-02-24 16:25:24
     
          bits_hex: 0x172e6f88  | 0x17  0x2e6f88
         exp_const: 0x2e6f880000000000000000000000000000000000000000
    
    calc_diff_full: 0x2e5b50d5ea00c22e450672894ab6cd8efb11d33f561783
    calc_diff_compact: 0x172e5b50
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 279     blk_ht: 564480
            seconds: 1210189
     from 2019-02-24 16:33:58
       to 2019-03-10 16:43:47
     
          bits_hex: 0x172e5b50  | 0x17  0x2e5b50
         exp_const: 0x2e5b500000000000000000000000000000000000000000
    
    calc_diff_full: 0x2e6117552c970db51f963da81ec630a74eb92fd741b85f
    calc_diff_compact: 0x172e6117
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 280     blk_ht: 566496
            seconds: 1150749
     from 2019-03-10 16:45:04
       to 2019-03-24 00:24:13
     
          bits_hex: 0x172e6117  | 0x17  0x2e6117
         exp_const: 0x2e61170000000000000000000000000000000000000000
    
    calc_diff_full: 0x2c1f6cd6cd49a166e33b007cd49a166e33b007cd49a166
    calc_diff_compact: 0x172c1f6c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 281     blk_ht: 568512
            seconds: 1206997
     from 2019-03-24 00:24:53
       to 2019-04-06 23:41:30
     
          bits_hex: 0x172c1f6c  | 0x17  0x2c1f6c
         exp_const: 0x2c1f6c0000000000000000000000000000000000000000
    
    calc_diff_full: 0x2c071d5baa2dd610943c76faa2dd610943c76faa2dd610
    calc_diff_compact: 0x172c071d
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 282     blk_ht: 570528
            seconds: 1217215
     from 2019-04-06 23:43:43
       to 2019-04-21 01:50:38
     
          bits_hex: 0x172c071d  | 0x17  0x2c071d
         exp_const: 0x2c071d0000000000000000000000000000000000000000
    
    calc_diff_full: 0x2c4e11fab990ee643b990ee643b990ee643b990ee643b9
    calc_diff_compact: 0x172c4e11
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 283     blk_ht: 572544
            seconds: 1146588
     from 2019-04-21 01:54:28
       to 2019-05-04 08:24:16
     
          bits_hex: 0x172c4e11  | 0x17  0x2c4e11
         exp_const: 0x2c4e110000000000000000000000000000000000000000
    
    calc_diff_full: 0x29ff38a485eec552bb921f885eec552bb921f885eec552
    calc_diff_compact: 0x1729ff38
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 284     blk_ht: 574560
            seconds: 1209156
     from 2019-05-04 08:32:13
       to 2019-05-18 08:24:49
     
          bits_hex: 0x1729ff38  | 0x17  0x29ff38
         exp_const: 0x29ff380000000000000000000000000000000000000000
    
    calc_diff_full: 0x29fb45ba0c0726d8d3f3a5a0c0726d8d3f3a5a0c0726d8
    calc_diff_compact: 0x1729fb45
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 285     blk_ht: 576576
            seconds: 1087168
     from 2019-05-18 08:31:36
       to 2019-05-30 22:31:04
     
          bits_hex: 0x1729fb45  | 0x17  0x29fb45
         exp_const: 0x29fb450000000000000000000000000000000000000000
    
    calc_diff_full: 0x25bb76c1845c8a0ce512956d9b1df623a67eac2f0734b7
    calc_diff_compact: 0x1725bb76
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 286     blk_ht: 578592
            seconds: 1217809
     from 2019-05-30 22:43:04
       to 2019-06-14 00:59:53
     
          bits_hex: 0x1725bb76  | 0x17  0x25bb76
         exp_const: 0x25bb760000000000000000000000000000000000000000
    
    calc_diff_full: 0x25fd03e86c405d9f7390d2a6c405d9f7390d2a6c405d9f
    calc_diff_compact: 0x1725fd03
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 287     blk_ht: 580608
            seconds: 1129519
     from 2019-06-14 01:03:50
       to 2019-06-27 02:49:09
     
          bits_hex: 0x1725fd03  | 0x17  0x25fd03
         exp_const: 0x25fd030000000000000000000000000000000000000000
    
    calc_diff_full: 0x23792c00a035bf17ad368f24ae069c257e139cf58b146d
    calc_diff_compact: 0x1723792c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 288     blk_ht: 582624
            seconds: 1058877
     from 2019-06-27 02:59:30
       to 2019-07-09 09:07:27
     
          bits_hex: 0x1723792c  | 0x17  0x23792c
         exp_const: 0x23792c0000000000000000000000000000000000000000
    
    calc_diff_full: 0x1f0d9b4413813813813813813813813813813813813813
    calc_diff_compact: 0x171f0d9b
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 289     blk_ht: 584640
            seconds: 1216360
     from 2019-07-09 09:17:48
       to 2019-07-23 11:10:28
     
          bits_hex: 0x171f0d9b  | 0x17  0x1f0d9b
         exp_const: 0x1f0d9b0000000000000000000000000000000000000000
    
    calc_diff_full: 0x1f3a08675fa0a4b4f5fa0a4b4f5fa0a4b4f5fa0a4b4f5f
    calc_diff_compact: 0x171f3a08
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 290     blk_ht: 586656
            seconds: 1091908
     from 2019-07-23 11:22:17
       to 2019-08-05 02:40:45
     
          bits_hex: 0x171f3a08  | 0x17  0x1f3a08
         exp_const: 0x1f3a080000000000000000000000000000000000000000
    
    calc_diff_full: 0x1c3039cb208fe6dc4ba298075e53c31a0f7ed5cb3a9186
    calc_diff_compact: 0x171c3039
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 291     blk_ht: 588672
            seconds: 1186065
     from 2019-08-05 02:52:08
       to 2019-08-18 20:19:53
     
          bits_hex: 0x171c3039  | 0x17  0x1c3039
         exp_const: 0x1c30390000000000000000000000000000000000000000
    
    calc_diff_full: 0x1ba3d156ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad1ad
    calc_diff_compact: 0x171ba3d1
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 292     blk_ht: 590688
            seconds: 1143516
     from 2019-08-18 20:49:42
       to 2019-09-01 02:28:18
     
          bits_hex: 0x171ba3d1  | 0x17  0x1ba3d1
         exp_const: 0x1ba3d10000000000000000000000000000000000000000
    
    calc_diff_full: 0x1a213e68389055d229ef6bc389055d229ef6bc389055d2
    calc_diff_compact: 0x171a213e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 293     blk_ht: 592704
            seconds: 1095808
     from 2019-09-01 02:35:01
       to 2019-09-13 18:58:29
     
          bits_hex: 0x171a213e  | 0x17  0x1a213e
         exp_const: 0x1a213e0000000000000000000000000000000000000000
    
    calc_diff_full: 0x17abf53a67eac2f0734b78fbd401845c8a0ce512956d9b
    calc_diff_compact: 0x1717abf5
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 294     blk_ht: 594720
            seconds: 1127200
     from 2019-09-13 19:06:31
       to 2019-09-26 20:13:11
     
          bits_hex: 0x1717abf5  | 0x17  0x17abf5
         exp_const: 0x17abf50000000000000000000000000000000000000000
    
    calc_diff_full: 0x160f247ac056b015ac056b015ac056b015ac056b015ac0
    calc_diff_compact: 0x17160f24
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 295     blk_ht: 596736
            seconds: 1186514
     from 2019-09-26 20:33:21
       to 2019-10-10 14:08:35
     
          bits_hex: 0x17160f24  | 0x17  0x160f24
         exp_const: 0x160f240000000000000000000000000000000000000000
    
    calc_diff_full: 0x15a35c7d3d0f8cb487042bfe7ba375f31aed6a9264e209
    calc_diff_compact: 0x1715a35c
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 296     blk_ht: 598752
            seconds: 1149225
     from 2019-10-10 14:08:55
       to 2019-10-23 21:22:40
     
          bits_hex: 0x1715a35c  | 0x17  0x15a35c
         exp_const: 0x15a35c0000000000000000000000000000000000000000
    
    calc_diff_full: 0x148edf4c00000000000000000000000000000000000000
    calc_diff_compact: 0x17148edf
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 297     blk_ht: 600768
            seconds: 1301982
     from 2019-10-23 21:42:53
       to 2019-11-07 23:22:35
     
          bits_hex: 0x17148edf  | 0x17  0x148edf
         exp_const: 0x148edf0000000000000000000000000000000000000000
    
    calc_diff_full: 0x1620d151ab7ab7ab7ab7ab7ab7ab7ab7ab7ab7ab7ab7ab
    calc_diff_compact: 0x171620d1
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 298     blk_ht: 602784
            seconds: 1185990
     from 2019-11-07 23:30:36
       to 2019-11-21 16:57:06
     
          bits_hex: 0x171620d1  | 0x17  0x1620d1
         exp_const: 0x1620d10000000000000000000000000000000000000000
    
    calc_diff_full: 0x15b23ee6c98c98c98c98c98c98c98c98c98c98c98c98c9
    calc_diff_compact: 0x1715b23e
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 299     blk_ht: 604800
            seconds: 1218655
     from 2019-11-21 17:08:52
       to 2019-12-05 19:39:47
     
          bits_hex: 0x1715b23e  | 0x17  0x15b23e
         exp_const: 0x15b23e0000000000000000000000000000000000000000
    
    calc_diff_full: 0x15dbd22956f56f56f56f56f56f56f56f56f56f56f56f56
    calc_diff_compact: 0x1715dbd2
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 300     blk_ht: 606816
            seconds: 1202898
     from 2019-12-05 20:02:25
       to 2019-12-19 18:10:43
     
          bits_hex: 0x1715dbd2  | 0x17  0x15dbd2
         exp_const: 0x15dbd20000000000000000000000000000000000000000
    
    calc_diff_full: 0x15bcd0d141d41d41d41d41d41d41d41d41d41d41d41d41
    calc_diff_compact: 0x1715bcd0
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 301     blk_ht: 608832
            seconds: 1135073
     from 2019-12-19 18:17:01
       to 2020-01-01 21:34:54
     
          bits_hex: 0x1715bcd0  | 0x17  0x15bcd0
         exp_const: 0x15bcd00000000000000000000000000000000000000000
    
    calc_diff_full: 0x1465f2b6859b8cec01f3526859b8cec01f3526859b8cec
    calc_diff_compact: 0x171465f2
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 302     blk_ht: 610848
            seconds: 1129575
     from 2020-01-01 21:54:27
       to 2020-01-14 23:40:42
     
          bits_hex: 0x171465f2  | 0x17  0x1465f2
         exp_const: 0x1465f20000000000000000000000000000000000000000
    
    calc_diff_full: 0x130c787b55555555555555555555555555555555555555
    calc_diff_compact: 0x17130c78
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 303     blk_ht: 612864
            seconds: 1155657
     from 2020-01-14 23:42:37
       to 2020-01-28 08:43:34
     
          bits_hex: 0x17130c78  | 0x17  0x130c78
         exp_const: 0x130c780000000000000000000000000000000000000000
    
    calc_diff_full: 0x1232ffc17cfe3649cb031697cfe3649cb031697cfe3649
    calc_diff_compact: 0x171232ff
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 304     blk_ht: 614880
            seconds: 1203326
     from 2020-01-28 08:44:03
       to 2020-02-11 06:59:29
     
          bits_hex: 0x171232ff  | 0x17  0x1232ff
         exp_const: 0x1232ff0000000000000000000000000000000000000000
    
    calc_diff_full: 0x121ad4a45ae9f2e3727b6bfb03f4838c7d0c1505949d8e
    calc_diff_compact: 0x17121ad4
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 305     blk_ht: 616896
            seconds: 1214274
     from 2020-02-11 07:10:24
       to 2020-02-25 08:28:18
     
          bits_hex: 0x17121ad4  | 0x17  0x121ad4
         exp_const: 0x121ad40000000000000000000000000000000000000000
    
    calc_diff_full: 0x122cbccd291f5ec2b8f85c5291f5ec2b8f85c5291f5ec2
    calc_diff_compact: 0x17122cbc
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 306     blk_ht: 618912
            seconds: 1131702
     from 2020-02-25 08:28:42
       to 2020-03-09 10:50:24
     
          bits_hex: 0x17122cbc  | 0x17  0x122cbc
         exp_const: 0x122cbc0000000000000000000000000000000000000000
    
    calc_diff_full: 0x1101196575d75d75d75d75d75d75d75d75d75d75d75d75
    calc_diff_compact: 0x17110119
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 307     blk_ht: 620928
            seconds: 1439165
     from 2020-03-09 11:05:17
       to 2020-03-26 02:51:22
     
          bits_hex: 0x17110119  | 0x17  0x110119
         exp_const: 0x1101190000000000000000000000000000000000000000
    
    calc_diff_full: 0x143b410e4629b7f0d4629b7f0d4629b7f0d4629b7f0d46
    calc_diff_compact: 0x17143b41
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 308     blk_ht: 622944
            seconds: 1143619
     from 2020-03-26 02:51:46
       to 2020-04-08 08:32:05
     
          bits_hex: 0x17143b41  | 0x17  0x143b41
         exp_const: 0x143b410000000000000000000000000000000000000000
    
    calc_diff_full: 0x1320bcb154d880bb3ee721a54d880bb3ee721a54d880bb
    calc_diff_compact: 0x171320bc
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 309     blk_ht: 624960
            seconds: 1115353
     from 2020-04-08 08:54:06
       to 2020-04-21 06:43:19
     
          bits_hex: 0x171320bc  | 0x17  0x1320bc
         exp_const: 0x1320bc0000000000000000000000000000000000000000
    
    calc_diff_full: 0x11a333820d1c8d849404fc0b7c7382f3eafa6b6271e2d9
    calc_diff_compact: 0x1711a333
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 310     blk_ht: 626976
            seconds: 1198623
     from 2020-04-21 07:05:24
       to 2020-05-05 04:02:27
     
          bits_hex: 0x1711a333  | 0x17  0x11a333
         exp_const: 0x11a3330000000000000000000000000000000000000000
    
    calc_diff_full: 0x117a3967627fc195b2f4c8e627fc195b2f4c8e627fc195
    calc_diff_compact: 0x17117a39
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 311     blk_ht: 628992
            seconds: 1286849
     from 2020-05-05 04:05:21
       to 2020-05-20 01:32:50
     
          bits_hex: 0x17117a39  | 0x17  0x117a39
         exp_const: 0x117a390000000000000000000000000000000000000000
    
    calc_diff_full: 0x1297f60ae3805a27c49e6c08e2b04d26f4916b38d5af7d
    calc_diff_compact: 0x171297f6
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 312     blk_ht: 631008
            seconds: 1333419
     from 2020-05-20 02:06:56
       to 2020-06-04 12:30:35
     
          bits_hex: 0x171297f6  | 0x17  0x1297f6
         exp_const: 0x1297f60000000000000000000000000000000000000000
    
    calc_diff_full: 0x147f352775d75d75d75d75d75d75d75d75d75d75d75d75
    calc_diff_compact: 0x17147f35
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 313     blk_ht: 633024
            seconds: 1052324
     from 2020-06-04 12:30:52
       to 2020-06-16 16:49:36
     
          bits_hex: 0x17147f35  | 0x17  0x147f35
         exp_const: 0x147f350000000000000000000000000000000000000000
    
    calc_diff_full: 0x11d4f2dafe1a8c536fe1a8c536fe1a8c536fe1a8c536fe
    calc_diff_compact: 0x1711d4f2
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 314     blk_ht: 635040
            seconds: 1209641
     from 2020-06-16 16:51:07
       to 2020-06-30 16:51:48
     
          bits_hex: 0x1711d4f2  | 0x17  0x11d4f2
         exp_const: 0x11d4f20000000000000000000000000000000000000000
    
    calc_diff_full: 0x11d5199c755a88dbc0ef422755a88dbc0ef422755a88db
    calc_diff_compact: 0x1711d519
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 315     blk_ht: 637056
            seconds: 1100695
     from 2020-06-30 17:18:49
       to 2020-07-13 11:03:44
     
          bits_hex: 0x1711d519  | 0x17  0x11d519
         exp_const: 0x11d5190000000000000000000000000000000000000000
    
    calc_diff_full: 0x103a15899a69a69a69a69a69a69a69a69a69a69a69a69a
    calc_diff_compact: 0x17103a15
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 316     blk_ht: 639072
            seconds: 1245383
     from 2020-07-13 11:51:00
       to 2020-07-27 21:47:23
     
          bits_hex: 0x17103a15  | 0x17  0x103a15
         exp_const: 0x103a150000000000000000000000000000000000000000
    
    calc_diff_full: 0x10b4f85e66210cbb766210cbb766210cbb766210cbb766
    calc_diff_compact: 0x1710b4f8
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 317     blk_ht: 641088
            seconds: 1202446
     from 2020-07-27 21:52:36
       to 2020-08-10 19:53:22
     
          bits_hex: 0x1710b4f8  | 0x17  0x10b4f8
         exp_const: 0x10b4f80000000000000000000000000000000000000000
    
    calc_diff_full: 0x109bac5a3d70a3d70a3d70a3d70a3d70a3d70a3d70a3d7
    calc_diff_compact: 0x17109bac
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 318     blk_ht: 643104
            seconds: 1167563
     from 2020-08-10 20:00:19
       to 2020-08-24 08:19:42
     
          bits_hex: 0x17109bac  | 0x17  0x109bac
         exp_const: 0x109bac0000000000000000000000000000000000000000
    
    calc_diff_full: 0x1007ea114df701923b45d67f8a1ac3ce5f0812a34c56e7
    calc_diff_compact: 0x171007ea
    
                  Difficulty -> INCREASE
    ------------------------------------
    Reg# 319     blk_ht: 645120
            seconds: 1224384
     from 2020-08-24 08:34:19
       to 2020-09-07 12:40:43
     
          bits_hex: 0x171007ea  | 0x17  0x1007ea
         exp_const: 0x1007ea0000000000000000000000000000000000000000
    
    calc_diff_full: 0x103a12b0eca8641fdb97530eca8641fdb97530eca8641f
    calc_diff_compact: 0x17103a12
    
                  Difficulty -> DECREASE
    ------------------------------------
    Reg# 320     blk_ht: 647136
            seconds: 1086311
     from 2020-09-07 12:48:40
       to 2020-09-20 02:33:51
     
          bits_hex: 0x17103a12  | 0x17  0x103a12
         exp_const: 0x103a120000000000000000000000000000000000000000
    
    calc_diff_full: 0xe92aa378c29d3ae4bf5d06e17f2903a14b25c36d47e58
    calc_diff_compact: 0x170e92aa
    
                  Difficulty -> INCREASE
    ------------------------------------



```python
0xba18 * 0.68813  # 32782.5132
hex(32782)  #  0x800e
#--
0x10c5a  * 0.69  # 47401.619999999995
hex(47401)  #  0xb929

#--
#0x7FFF   # 32767

```




    '0xb929'




```python
%whos
```

    Variable            Type          Data/Info
    -------------------------------------------
    MBFLAG              bool          True
    bits_hex            int           386939410
    blk_end_ts          int64         1600569231
    blk_interval_secs   int64         1086311
    blk_start_ts        int64         1600570533
    calc_diff           long          1688368698599932972367448<...>8428006979540066251046912
    calc_diff_post      int           0
    calc_diff_pre       long          1395807455853119190118591<...>0375717946434341550980696
    cnt                 int           321
    conn                connection    <connection object at 0x7<...>ame=btc_hist', closed: 0>
    datetime            type          <type 'datetime.datetime'>
    df_datachain        DataFrame             blk_ht    bits_he<...>[649600 rows x 5 columns]
    dif_interval        int           2016
    elem                int           2015
    exp_bits            long          1461501637330902918203684832716283019655932542976
    exp_const           long          1554222224206450061140363<...>3469446988944215367483392
    getCompact          function      <function getCompact at 0x7f3797739f50>
    np                  module        <module 'numpy' from '/us<...>ages/numpy/__init__.pyc'>
    pd                  module        <module 'pandas' from '/u<...>ges/pandas/__init__.pyc'>
    pg                  module        <module 'psycopg2' from '<...>s/psycopg2/__init__.pyc'>
    plt                 module        <module 'matplotlib.pyplo<...>s/matplotlib/pyplot.pyc'>
    psql                module        <module 'pandas.io.sql' f<...>kages/pandas/io/sql.pyc'>
    reg_difficulty      int           1063442
    reg_exp             int           23
    sbeg                int           649151
    seaborn             module        <module 'seaborn' from '/<...>es/seaborn/__init__.pyc'>
    send                int           651167
    tSQL                str            SELECT\n      height_str<...>BY   height_str::bigint\n
    tbase               int           0
    tind                str           INCREASE
    trt                 float64       0.898074570106
    unpack_exp          function      <function unpack_exp at 0x7f3797739cd0>



```python
send = len(df_datachain)-1
sbeg = send - 2016

blk_start_ts = df_datachain['blk_time_longint'][sbeg]
blk_end_ts =  df_datachain['blk_time_longint'][send]
blk_interval_secs =  blk_end_ts - blk_start_ts

print "time taken to make 2016 blocks : "
print " from "+datetime.utcfromtimestamp(float(blk_start_ts)).strftime("%Y-%m-%d %H:%M:%S")
print "   to "+datetime.utcfromtimestamp(float(blk_end_ts)).strftime("%Y-%m-%d %H:%M:%S")
print " "
print "       " + str(blk_interval_secs)+ ' seconds'
print " " + str(blk_interval_secs/60.0 )+' minutes'
print " " + str((blk_interval_secs/60.0)/60.0 )+' hours'
print " " + str(((blk_interval_secs/60.0)/60.0)/24)+' days'
print " "
print " " + str(blk_interval_secs/(14*24*6.0))+ ' seconds per block'
print " "

if blk_interval_secs > (14*24*6*600):  tind = 'DECREASE'
else:  tind = 'INCREASE'
    
print " Difficulty -> " + tind
```

    time taken to make 2016 blocks : 
     from 2020-09-10 09:39:04
       to 2020-09-23 09:31:16
     
           1122732 seconds
     18712.2 minutes
     311.87 hours
     12.9945833333 days
     
     556.910714286 seconds per block
     
     Difficulty -> INCREASE


### Method 2:  Take an average without SUM()

new average = ((old_count * old_data) + next_data) / next_count

new average = old_average + (next_data - old_average) / next_count


```python
## per Difficulty Formula, compare an ideal time to actual time
##   https://en.bitcoin.it/wiki/Difficulty#
##     How_is_difficulty_calculated.3F_What_is_the_difference_between_bdiff_and_pdiff.3F
```


```python
## calc and emit the mean interval for series blk_time_longint
##    data_chain [ sbeg  to  send  ] 
##

elem  = sbeg
cnt   = 1
## first elem assign an ideal time 60*10
t_avg = 600.0  

while True:
  if cnt == 1:
    elem = elem+1
    cnt  = cnt+1
    continue

  d_elem = df_datachain['blk_time_longint'][elem]
  dp_elem = df_datachain['blk_time_longint'][elem-1]
  time_diff = d_elem - dp_elem
    
  dx = time_diff - t_avg
  new_avg = (t_avg + ( time_diff - t_avg)/ float(cnt))

  if cnt == send - sbeg:
    print ' avg blk time: '+str(new_avg)
    break

  #print ' '+str(elem)+'    '+str(t_avg)+'           '+str(dx)+'     '+str(new_avg)
  t_avg = new_avg
  elem = elem + 1
  cnt = cnt + 1
  # end While

```

     avg blk time: 544.135912698


### Calc Mean Mining Time for the Chain by Difficulty Regimen


```python
## calc and emit the mean interval for series blk_time_longint
##    data_chain [ sbeg  to  send  ] 
##

tbase = 15
## well-known constant for 2-weeks of mining
dif_interval = 6*24*14    # =>2016 ten minute blocks     

## convenience variables to mark start and end in the interval
sbeg = (dif_interval * tbase) + 2015   ## skip the genesis interval in all calcs
send = sbeg + dif_interval             ## the last block with the same difficulty

elem  = sbeg
cnt   = 0

## Simulate the first avg as an ideal time 60*10
t_avg = 600.0  
elem = elem + 1
cnt  = cnt + 1

print "blk_ht,mean_blk_time"
while True:
  d_elem = df_datachain['blk_time_longint'][elem]
  dp_elem = df_datachain['blk_time_longint'][elem-1]
  time_diff = d_elem - dp_elem
    
  dx = time_diff - t_avg
  new_avg = (t_avg + ( time_diff - t_avg)/ float(cnt))

  #print ' '+str(elem)+'    '+str(t_avg)+'           '+str(dx)+'     '+str(new_avg)
  t_avg = new_avg

  if cnt == dif_interval:
    ## pesky list index to blk_ht +1
    print  str(elem-dif_interval+1)+','+str(new_avg)
    cnt = 0
    t_avg = 600.0
    
  elif elem == (len(df_datachain)-1):
    break

  elem = elem + 1
  cnt = cnt + 1

# end While

```

    blk_ht,mean_blk_time
    32256,543.974702381
    34272,582.719246032
    36288,443.90625
    38304,432.433035714
    40320,401.431051587
    42336,500.996527778
    44352,595.828869048
    46368,450.385912698
    48384,467.035714286
    50400,409.318452381
    52416,535.517857143
    54432,650.822916667
    56448,428.081845238
    58464,574.076388889
    60480,537.529761905
    62496,495.544642857
    64512,310.899305556
    66528,143.62202381
    68544,446.814980159
    70560,416.188988095
    72576,413.232142857
    74592,492.922619048
    76608,524.745039683
    78624,466.050595238
    80640,417.688988095
    82656,574.30406746
    84672,385.064484127
    86688,417.426587302
    88704,409.841765873
    90720,396.87797619
    92736,510.516369048
    94752,395.896329365
    96768,507.560019841
    98784,533.218253968
    100800,530.831349206
    102816,503.275793651
    104832,508.248511905
    106848,429.242063492
    108864,394.150793651
    110880,438.291170635
    112896,662.911706349
    114912,502.68452381
    116928,535.108630952
    118944,505.458333333
    120960,418.583333333
    122976,387.124503968
    124992,336.819940476
    127008,460.212797619
    129024,388.228670635
    131040,382.018353175
    133056,530.07093254
    135072,554.981646825
    137088,537.14781746
    139104,627.785218254
    141120,609.825892857
    143136,607.645337302
    145152,623.66468254
    147168,690.997519841
    149184,732.072916667
    151200,605.771825397
    153216,656.076388889
    155232,566.733630952
    157248,598.239087302
    159264,556.632440476
    161280,574.071428571
    163296,568.976686508
    165312,601.534722222
    167328,551.825892857
    169344,599.532738095
    171360,552.734623016
    173376,618.54265873
    175392,627.817956349
    177408,522.293154762
    179424,653.628968254
    181440,603.420634921
    183456,550.316964286
    185472,591.704861111
    187488,563.157242063
    189504,550.799107143
    191520,558.173611111
    193536,538.861111111
    195552,543.570436508
    197568,564.391865079
    199584,562.405257937
    201600,595.986111111
    203616,559.537202381
    205632,588.785218254
    207648,587.818948413
    209664,612.293650794
    211680,678.634424603
    213696,551.038690476
    215712,657.169146825
    217728,543.977678571
    219744,538.35515873
    221760,501.633928571
    223776,540.764880952
    225792,434.76140873
    227808,524.413690476
    229824,513.140376984
    231840,534.889384921
    233856,540.501984127
    235872,552.392361111
    237888,467.545634921
    239904,484.293650794
    241920,544.182043651
    243936,489.304563492
    245952,502.80952381
    247968,501.630952381
    249984,441.654265873
    252000,464.200892857
    254016,453.938988095
    256032,463.188492063
    258048,454.314484127
    260064,472.423611111
    262080,424.224206349
    264096,410.954365079
    266112,459.747519841
    268128,505.146825397
    270144,517.435019841
    272160,467.280753968
    274176,462.512896825
    276192,499.606150794
    278208,475.645833333
    280224,489.536706349
    282240,503.139880952
    284256,502.670138889
    286272,492.433531746
    288288,538.779265873
    290304,509.458333333
    292320,491.391865079
    294336,526.373015873
    296352,523.575892857
    298368,542.23015873
    300384,508.154265873
    302400,533.850694444
    304416,523.951884921
    306432,480.295634921
    308448,583.500992063
    310464,555.157738095
    312480,569.950396825
    314496,496.729166667
    316512,521.643849206
    318528,551.46031746
    320544,516.46031746
    322560,594.139880952
    324576,584.09077381
    326592,545.256944444
    328608,590.522321429
    330624,604.454861111
    332640,608.789186508
    334656,582.621031746
    336672,554.80406746
    338688,639.321924603
    340704,557.276289683
    342720,571.6875
    344736,590.886904762
    346752,609.249007937
    348768,566.931547619
    350784,623.294146825
    352800,599.918154762
    354816,585.859126984
    356832,615.452380952
    358848,574.691468254
    360864,603.910218254
    362880,580.873015873
    364896,586.309027778
    366912,595.558035714
    368928,583.161210317
    370944,571.886904762
    372960,576.337797619
    374976,585.806547619
    376992,599.385416667
    379008,587.279265873
    381024,567.453373016
    383040,543.536706349
    385056,551.681547619
    387072,508.252480159
    389088,540.313988095
    391104,549.908730159
    393120,567.245535714
    395136,499.809027778
    397152,529.214285714
    399168,619.39484127
    401184,574.46031746
    403200,595.439980159
    405216,560.449404762
    407232,600.083829365
    409248,551.962301587
    411264,584.771825397
    413280,610.095238095
    415296,561.81547619
    417312,589.197916667
    419328,600.19890873
    421344,634.761904762
    423360,557.396825397
    425376,591.422123016
    427392,586.550595238
    429408,561.839285714
    431424,560.017361111
    433440,612.781746032
    435456,597.738095238
    437472,542.359623016
    439488,589.767361111
    441504,554.824900794
    443520,586.08234127
    445536,566.069940476
    447552,514.642857143
    449568,558.510416667
    451584,574.98859127
    453600,573.998015873
    455616,581.294146825
    457632,571.514384921
    459648,575.616071429
    461664,598.814980159
    463680,559.547123016
    465696,563.96031746
    467712,526.941964286
    469728,572.731150794
    471744,602.630952381
    473760,528.524305556
    475776,561.255952381
    477792,559.908730159
    479808,623.832837302
    481824,577.548611111
    483840,502.024305556
    485856,589.181547619
    487872,563.609126984
    489888,494.275793651
    491904,639.915178571
    493920,608.315972222
    495936,508.038194444
    497952,509.880456349
    499968,582.16765873
    501984,520.130952381
    504000,513.564484127
    506016,543.44593254
    508032,573.597222222
    510048,550.009424603
    512064,570.208333333
    514080,591.834821429
    516096,548.896329365
    518112,573.075892857
    520128,583.036706349
    522144,577.578373016
    524160,523.131448413
    526176,584.007440476
    528192,568.261904762
    530208,621.918650794
    532224,522.282738095
    534240,559.567460317
    536256,569.926587302
    538272,575.718253968
    540288,589.31547619
    542304,576.003472222
    544320,622.869543651
    546336,600.490575397
    548352,648.128968254
    550368,707.382936508
    552384,663.490575397
    554400,546.053571429
    556416,573.206845238
    558432,607.598710317
    560448,575.728174603
    562464,599.234623016
    564480,600.330357143
    566496,570.827876984
    568512,598.774801587
    570528,603.891369048
    572544,568.980654762
    574560,599.981646825
    576576,539.626984127
    578592,604.189484127
    580608,560.58531746
    582624,525.544642857
    584640,603.704861111
    586656,541.959821429
    588672,589.213293651
    590688,567.420138889
    592704,543.794642857
    594720,559.72718254
    596736,588.558531746
    598752,570.653769841
    600768,646.062996032
    602784,588.638888889
    604800,605.165178571
    606816,596.863095238
    608832,563.614087302
    610848,560.362103175
    612864,573.256944444
    614880,597.212797619
    616896,602.330357143
    618912,561.803075397
    620928,713.88343254
    622944,567.926587302
    624960,553.907738095
    626976,594.641369048
    628992,639.332837302
    631008,661.426587302
    633024,522.03125
    635040,600.824404762
    637056,547.38640873
    639072,617.904761905
    641088,596.658234127
    643104,579.583333333
    645120,607.569940476
    647136,539.490575397


    btc_hist=# select d.height_str, d.bits_hex,c.mean_blk_time from data_chain d JOIN     chng_blk_time c ON (d.height_str::integer = c.blk_height::integer) order by     c.blk_height::integer;
    
    btc_hist=# create table blk_times as select d.height_str::integer as blk_height, d.bits_hex,c.mean_blk_time from data_chain d JOIN chng_blk_time c ON (d.height_str::integer = c.blk_height::integer) order by c.blk_height::integer;
    SELECT 306
    
    btc_hist=# \d blk_times
                     Table "public.blk_times"
        Column     |       Type       | Collation | Nullable | Default 
    ---------------+------------------+-----------+----------+---------
     blk_height    | integer          |           |          | 
     bits_hex      | text             |           |          | 
     mean_blk_time | double precision |           |          | 




```python
bits_hex = int( df_datachain['bits_hex'][sbeg], 16) 
reg_difficulty  = bits_hex & 0x00FFFFFF
reg_exp = (bits_hex & 0xFF000000) >> 24
print str(type(reg_difficulty))+'  '+hex(reg_exp)+' '+hex(reg_difficulty)
```

    <type 'int'>  0x1d 0xd86a


###  Compute BITS Difficulty Change   &nbsp; &nbsp; method-1


```python
##
##  ratio 'trt' of:  actual_time / ideal time
##
##    ratio > 1.0   => difficulty DECREASE; bits_hex INCREASE 
##
##    ratio < 1.0   => difficulty INCREASE; bits_hex DECREASE 
##
##   At least one problem with this solution, this simple
##   code ignores the exponent portion of the BITS format, which will 
##   occassionally lead to a very wrong result, but most often has no effect.
## e.g.  Reg# 36,59,122,147,248
##   (since there are actually only 300-odd times the value changes, a manual
##   conditional test could be used instead of tricky factors methods).
##
##  Read the answer for a given Regimen ID# by  last hex value on the right,
##   compared to first hex value on the right, in the next Regimen below it.
##
##  e.g.  Reg# 220  has predicted change  trt_ratio*BITS: 0x343e4
##        Reg# 221  below has actual  BITS_hex  0x34379
##

tbase = 0
## well-known constant for 2-weeks of mining
dif_interval = 6*24*14    # =>2016 ten minute blocks     

## convenience variables to mark start and end in the interval
sbeg = (dif_interval * tbase) + 2015   ## skip the genesis interval in all calcs
send = sbeg + dif_interval             ## the last block with the same difficulty

elem  = sbeg
cnt   = 0

while cnt < len(df_datachain):
    print 'Reg# '+str(tbase+cnt)+'     blk_ht: '+ str(df_datachain['blk_ht'][sbeg]) 
    print '           seconds: '+str(blk_interval_secs)
    print " from "+datetime.utcfromtimestamp(float(blk_start_ts)).strftime("%Y-%m-%d %H:%M:%S")
    print "   to "+datetime.utcfromtimestamp(float(blk_end_ts)).strftime("%Y-%m-%d %H:%M:%S")
    print " "

    bits_hex = int( df_datachain['bits_hex'][sbeg], 16) 
    reg_difficulty  = bits_hex & 0x00FFFFFF
    reg_exp = (bits_hex & 0xFF000000) >> 24
    print '      bits_hex: '+hex(bits_hex)+'  | '+hex(reg_exp)+'  '+hex(reg_difficulty)

    try:
        blk_start_ts = df_datachain['blk_time_longint'][sbeg]
        blk_end_ts =  df_datachain['blk_time_longint'][send]
    except:
        break
    blk_interval_secs =  blk_end_ts - blk_start_ts

    trt = blk_interval_secs / (2016*600.0)
    
    print 'reg_difficulty:  '+str(reg_difficulty)
    print '     trt_ratio: '+str(trt) +'  trt_ratio*BITS: '+hex( trt*reg_difficulty )[:-1] + '\n'
    # calc new hex value, or

    if trt > 1.0:  tind = 'DECREASE'
    else:  tind = 'INCREASE'  
    print " Difficulty -> " + tind + "\n"
    
    print "-----------------------------"
    
    cnt = cnt + 1
    sbeg = sbeg + 2016
    send = send + 2016

```

    Reg# 0     blk_ht: 2016
               seconds: 1088957
     from 2020-09-20 02:55:33
       to 2020-09-20 03:17:57
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 1.15960565476  trt_ratio*BITS: 0x128da
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 1     blk_ht: 4032
               seconds: 1402659
     from 2009-01-27 13:38:51
       to 2009-02-12 19:16:30
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 1.2403463955  trt_ratio*BITS: 0x13d86
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 2     blk_ht: 6048
               seconds: 1500323
     from 2009-02-12 19:16:30
       to 2009-03-02 04:01:53
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 1.27502728175  trt_ratio*BITS: 0x14666
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 3     blk_ht: 8064
               seconds: 1542273
     from 2009-03-02 04:01:53
       to 2009-03-20 00:26:26
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 1.27866815476  trt_ratio*BITS: 0x14755
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 4     blk_ht: 10080
               seconds: 1546677
     from 2009-03-20 00:26:26
       to 2009-04-06 22:04:23
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 1.27615327381  trt_ratio*BITS: 0x146b0
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 5     blk_ht: 12096
               seconds: 1543635
     from 2009-04-06 22:04:23
       to 2009-04-24 18:51:38
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 1.2395229828  trt_ratio*BITS: 0x13d50
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 6     blk_ht: 14112
               seconds: 1499327
     from 2009-04-24 18:51:38
       to 2009-05-12 03:20:25
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 1.35471230159  trt_ratio*BITS: 0x15acd
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 7     blk_ht: 16128
               seconds: 1638660
     from 2009-05-12 03:20:25
       to 2009-05-31 02:31:25
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 1.91376322751  trt_ratio*BITS: 0x1e9ea
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 8     blk_ht: 18144
               seconds: 2314888
     from 2009-05-31 02:31:25
       to 2009-06-26 21:32:53
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 2.00879877646  trt_ratio*BITS: 0x2023e
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 9     blk_ht: 20160
               seconds: 2429843
     from 2009-06-26 21:32:53
       to 2009-07-25 00:30:16
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 2.96584160053  trt_ratio*BITS: 0x2f73e
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 10     blk_ht: 22176
               seconds: 3587482
     from 2009-07-25 00:30:16
       to 2009-09-04 13:01:38
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 1.97150297619  trt_ratio*BITS: 0x1f8b2
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 11     blk_ht: 24192
               seconds: 2384730
     from 2009-09-04 13:01:38
       to 2009-10-02 03:27:08
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 2.10720238095  trt_ratio*BITS: 0x21b6f
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 12     blk_ht: 26208
               seconds: 2548872
     from 2009-10-02 03:27:08
       to 2009-10-31 15:28:20
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 1.94755869709  trt_ratio*BITS: 0x1f291
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 13     blk_ht: 28224
               seconds: 2355767
     from 2009-10-31 15:28:20
       to 2009-11-27 21:51:07
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 1.46452876984  trt_ratio*BITS: 0x176e9
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 14     blk_ht: 30240
               seconds: 1771494
     from 2009-11-27 21:51:07
       to 2009-12-18 09:56:01
     
          bits_hex: 0x1d00ffff  | 0x1d  0xffff
    reg_difficulty:  65535
         trt_ratio: 0.845984623016  trt_ratio*BITS: 0xd891
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 15     blk_ht: 32256
               seconds: 1023303
     from 2009-12-18 09:56:01
       to 2009-12-30 06:11:04
     
          bits_hex: 0x1d00d86a  | 0x1d  0xd86a
    reg_difficulty:  55402
         trt_ratio: 0.906624503968  trt_ratio*BITS: 0xc434
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 16     blk_ht: 34272
               seconds: 1096653
     from 2009-12-30 06:11:04
       to 2010-01-11 22:48:37
     
          bits_hex: 0x1d00c428  | 0x1d  0xc428
    reg_difficulty:  50216
         trt_ratio: 0.971198743386  trt_ratio*BITS: 0xbe81
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 17     blk_ht: 36288
               seconds: 1174762
     from 2010-01-11 22:48:37
       to 2010-01-25 13:07:59
     
          bits_hex: 0x1d00be71  | 0x1d  0xbe71
    reg_difficulty:  48753
         trt_ratio: 0.73984375  trt_ratio*BITS: 0x8ce5
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 18     blk_ht: 38304
               seconds: 894915
     from 2010-01-25 13:07:59
       to 2010-02-04 21:43:14
     
          bits_hex: 0x1d008cc3  | 0x1d  0x8cc3
    reg_difficulty:  36035
         trt_ratio: 0.72072172619  trt_ratio*BITS: 0x6573
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 19     blk_ht: 40320
               seconds: 871785
     from 2010-02-04 21:43:14
       to 2010-02-14 23:52:59
     
          bits_hex: 0x1c654657  | 0x1c  0x654657
    reg_difficulty:  6637143
         trt_ratio: 0.669051752646  trt_ratio*BITS: 0x43c210
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 20     blk_ht: 42336
               seconds: 809285
     from 2010-02-14 23:52:59
       to 2010-02-24 08:41:04
     
          bits_hex: 0x1c43b3e5  | 0x1c  0x43b3e5
    reg_difficulty:  4436965
         trt_ratio: 0.834994212963  trt_ratio*BITS: 0x388808
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 21     blk_ht: 44352
               seconds: 1010009
     from 2010-02-24 08:41:04
       to 2010-03-08 01:14:33
     
          bits_hex: 0x1c387f6f  | 0x1c  0x387f6f
    reg_difficulty:  3702639
         trt_ratio: 0.993048115079  trt_ratio*BITS: 0x381ae2
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 22     blk_ht: 46368
               seconds: 1201191
     from 2010-03-08 01:14:33
       to 2010-03-21 22:54:24
     
          bits_hex: 0x1c381375  | 0x1c  0x381375
    reg_difficulty:  3674997
         trt_ratio: 0.750643187831  trt_ratio*BITS: 0x2a17d3
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 23     blk_ht: 48384
               seconds: 907978
     from 2010-03-21 22:54:24
       to 2010-04-01 11:07:22
     
          bits_hex: 0x1c2a1115  | 0x1c  0x2a1115
    reg_difficulty:  2756885
         trt_ratio: 0.778392857143  trt_ratio*BITS: 0x20be93
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 24     blk_ht: 50400
               seconds: 941544
     from 2010-04-01 11:07:22
       to 2010-04-12 08:39:46
     
          bits_hex: 0x1c20bca7  | 0x1c  0x20bca7
    reg_difficulty:  2145447
         trt_ratio: 0.682197420635  trt_ratio*BITS: 0x165542
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 25     blk_ht: 52416
               seconds: 825186
     from 2010-04-12 08:39:46
       to 2010-04-21 21:52:52
     
          bits_hex: 0x1c16546f  | 0x1c  0x16546f
    reg_difficulty:  1463407
         trt_ratio: 0.892529761905  trt_ratio*BITS: 0x13ee16
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 26     blk_ht: 54432
               seconds: 1079604
     from 2010-04-21 21:52:52
       to 2010-05-04 09:46:16
     
          bits_hex: 0x1c13ec53  | 0x1c  0x13ec53
    reg_difficulty:  1305683
         trt_ratio: 1.08470486111  trt_ratio*BITS: 0x159c58
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 27     blk_ht: 56448
               seconds: 1312059
     from 2010-05-04 09:46:16
       to 2010-05-19 14:13:55
     
          bits_hex: 0x1c159c24  | 0x1c  0x159c24
    reg_difficulty:  1416228
         trt_ratio: 0.713469742063  trt_ratio*BITS: 0xf6b03
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 28     blk_ht: 58464
               seconds: 863013
     from 2010-05-19 14:13:55
       to 2010-05-29 13:57:28
     
          bits_hex: 0x1c0f675c  | 0x1c  0xf675c
    reg_difficulty:  1009500
         trt_ratio: 0.956793981481  trt_ratio*BITS: 0xebcfb
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 29     blk_ht: 60480
               seconds: 1157338
     from 2010-05-29 13:57:28
       to 2010-06-11 23:26:26
     
          bits_hex: 0x1c0eba64  | 0x1c  0xeba64
    reg_difficulty:  965220
         trt_ratio: 0.895882936508  trt_ratio*BITS: 0xd31d4
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 30     blk_ht: 62496
               seconds: 1083660
     from 2010-06-11 23:26:26
       to 2010-06-24 12:27:26
     
          bits_hex: 0x1c0d3142  | 0x1c  0xd3142
    reg_difficulty:  864578
         trt_ratio: 0.825907738095  trt_ratio*BITS: 0xae54d
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 31     blk_ht: 64512
               seconds: 999018
     from 2010-06-24 12:27:26
       to 2010-07-06 01:57:44
     
          bits_hex: 0x1c0ae493  | 0x1c  0xae493
    reg_difficulty:  713875
         trt_ratio: 0.518165509259  trt_ratio*BITS: 0x5a4f1
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 32     blk_ht: 66528
               seconds: 626773
     from 2010-07-06 01:57:44
       to 2010-07-13 08:03:57
     
          bits_hex: 0x1c05a3f4  | 0x1c  0x5a3f4
    reg_difficulty:  369652
         trt_ratio: 0.239370039683  trt_ratio*BITS: 0x159a3
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 33     blk_ht: 68544
               seconds: 289542
     from 2010-07-13 08:03:57
       to 2010-07-16 16:29:39
     
          bits_hex: 0x1c0168fd  | 0x1c  0x168fd
    reg_difficulty:  92413
         trt_ratio: 0.744691633598  trt_ratio*BITS: 0x10cd3
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 34     blk_ht: 70560
               seconds: 900779
     from 2010-07-16 16:29:39
       to 2010-07-27 02:42:38
     
          bits_hex: 0x1c010c5a  | 0x1c  0x10c5a
    reg_difficulty:  68698
         trt_ratio: 0.693648313492  trt_ratio*BITS: 0xba24
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 35     blk_ht: 72576
               seconds: 839037
     from 2010-07-27 02:42:38
       to 2010-08-05 19:46:35
     
          bits_hex: 0x1c00ba18  | 0x1c  0xba18
    reg_difficulty:  47640
         trt_ratio: 0.688720238095  trt_ratio*BITS: 0x802a
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 36     blk_ht: 74592
               seconds: 833076
     from 2010-08-05 19:46:35
       to 2010-08-15 11:11:11
     
          bits_hex: 0x1c00800e  | 0x1c  0x800e
    reg_difficulty:  32782
         trt_ratio: 0.821537698413  trt_ratio*BITS: 0x6933
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 37     blk_ht: 76608
               seconds: 993732
     from 2010-08-15 11:11:11
       to 2010-08-26 23:13:23
     
          bits_hex: 0x1b692098  | 0x1b  0x692098
    reg_difficulty:  6889624
         trt_ratio: 0.874575066138  trt_ratio*BITS: 0x5bf115
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 38     blk_ht: 78624
               seconds: 1057886
     from 2010-08-26 23:13:23
       to 2010-09-08 05:04:49
     
          bits_hex: 0x1b5bede6  | 0x1b  0x5bede6
    reg_difficulty:  6024678
         trt_ratio: 0.776750992063  trt_ratio*BITS: 0x4767fa
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 39     blk_ht: 80640
               seconds: 939558
     from 2010-09-08 05:04:49
       to 2010-09-19 02:04:07
     
          bits_hex: 0x1b4766ed  | 0x1b  0x4766ed
    reg_difficulty:  4679405
         trt_ratio: 0.696148313492  trt_ratio*BITS: 0x31b4d7
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 40     blk_ht: 82656
               seconds: 842061
     from 2010-09-19 02:04:07
       to 2010-09-28 19:58:28
     
          bits_hex: 0x1b31b2a3  | 0x1b  0x31b2a3
    reg_difficulty:  3256995
         trt_ratio: 0.957173445767  trt_ratio*BITS: 0x2f91c5
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 41     blk_ht: 84672
               seconds: 1157797
     from 2010-09-28 19:58:28
       to 2010-10-12 05:35:05
     
          bits_hex: 0x1b2f8e9d  | 0x1b  0x2f8e9d
    reg_difficulty:  3116701
         trt_ratio: 0.641774140212  trt_ratio*BITS: 0x1e855a
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 42     blk_ht: 86688
               seconds: 776290
     from 2010-10-12 05:35:05
       to 2010-10-21 05:13:15
     
          bits_hex: 0x1b1e7eca  | 0x1b  0x1e7eca
    reg_difficulty:  1998538
         trt_ratio: 0.695710978836  trt_ratio*BITS: 0x153744
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 43     blk_ht: 88704
               seconds: 841532
     from 2010-10-21 05:13:15
       to 2010-10-30 22:58:47
     
          bits_hex: 0x1b153263  | 0x1b  0x153263
    reg_difficulty:  1389155
         trt_ratio: 0.683069609788  trt_ratio*BITS: 0xe7a99
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 44     blk_ht: 90720
               seconds: 826241
     from 2010-10-30 22:58:47
       to 2010-11-09 12:29:28
     
          bits_hex: 0x1b0e7256  | 0x1b  0xe7256
    reg_difficulty:  946774
         trt_ratio: 0.661463293651  trt_ratio*BITS: 0x98e50
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 45     blk_ht: 92736
               seconds: 800106
     from 2010-11-09 12:29:28
       to 2010-11-18 18:44:34
     
          bits_hex: 0x1b098b2a  | 0x1b  0x98b2a
    reg_difficulty:  625450
         trt_ratio: 0.850860615079  trt_ratio*BITS: 0x81eca
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 46     blk_ht: 94752
               seconds: 1029201
     from 2010-11-18 18:44:34
       to 2010-11-30 16:37:55
     
          bits_hex: 0x1b081cd2  | 0x1b  0x81cd2
    reg_difficulty:  531666
         trt_ratio: 0.659827215608  trt_ratio*BITS: 0x55a57
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 47     blk_ht: 96768
               seconds: 798127
     from 2010-11-30 16:37:55
       to 2010-12-09 22:20:02
     
          bits_hex: 0x1b055953  | 0x1b  0x55953
    reg_difficulty:  350547
         trt_ratio: 0.845933366402  trt_ratio*BITS: 0x4865b
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 48     blk_ht: 98784
               seconds: 1023241
     from 2010-12-09 22:20:02
       to 2010-12-21 18:34:03
     
          bits_hex: 0x1b04864c  | 0x1b  0x4864c
    reg_difficulty:  296524
         trt_ratio: 0.888697089947  trt_ratio*BITS: 0x40560
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 49     blk_ht: 100800
               seconds: 1074968
     from 2010-12-21 18:34:03
       to 2011-01-03 05:10:11
     
          bits_hex: 0x1b0404cb  | 0x1b  0x404cb
    reg_difficulty:  263371
         trt_ratio: 0.884718915344  trt_ratio*BITS: 0x38e31
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 50     blk_ht: 102816
               seconds: 1070156
     from 2011-01-03 05:10:11
       to 2011-01-15 14:26:07
     
          bits_hex: 0x1b038dee  | 0x1b  0x38dee
    reg_difficulty:  232942
         trt_ratio: 0.838792989418  trt_ratio*BITS: 0x2fb3e
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 51     blk_ht: 104832
               seconds: 1014604
     from 2011-01-15 14:26:07
       to 2011-01-27 08:16:11
     
          bits_hex: 0x1b02fa29  | 0x1b  0x2fa29
    reg_difficulty:  195113
         trt_ratio: 0.847080853175  trt_ratio*BITS: 0x2859c
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 52     blk_ht: 106848
               seconds: 1024629
     from 2011-01-27 08:16:11
       to 2011-02-08 04:53:20
     
          bits_hex: 0x1b028552  | 0x1b  0x28552
    reg_difficulty:  165202
         trt_ratio: 0.715403439153  trt_ratio*BITS: 0x1cdaa
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 53     blk_ht: 108864
               seconds: 865352
     from 2011-02-08 04:53:20
       to 2011-02-18 05:15:52
     
          bits_hex: 0x1b01cc26  | 0x1b  0x1cc26
    reg_difficulty:  117798
         trt_ratio: 0.656917989418  trt_ratio*BITS: 0x12e47
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 54     blk_ht: 110880
               seconds: 794608
     from 2011-02-18 05:15:52
       to 2011-02-27 09:59:20
     
          bits_hex: 0x1b012dcd  | 0x1b  0x12dcd
    reg_difficulty:  77261
         trt_ratio: 0.730485284392  trt_ratio*BITS: 0xdc76
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 55     blk_ht: 112896
               seconds: 883595
     from 2011-02-27 09:59:20
       to 2011-03-09 15:25:55
     
          bits_hex: 0x1b00dc31  | 0x1b  0xdc31
    reg_difficulty:  56369
         trt_ratio: 1.10485284392  trt_ratio*BITS: 0xf347
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 56     blk_ht: 114912
               seconds: 1336430
     from 2011-03-09 15:25:55
       to 2011-03-25 02:39:45
     
          bits_hex: 0x1b00f339  | 0x1b  0xf339
    reg_difficulty:  62265
         trt_ratio: 0.837807539683  trt_ratio*BITS: 0xcbc6
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 57     blk_ht: 116928
               seconds: 1013412
     from 2011-03-25 02:39:45
       to 2011-04-05 20:09:57
     
          bits_hex: 0x1b00cbbd  | 0x1b  0xcbbd
    reg_difficulty:  52157
         trt_ratio: 0.891847718254  trt_ratio*BITS: 0xb5b4
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 58     blk_ht: 118944
               seconds: 1078779
     from 2011-04-05 20:09:57
       to 2011-04-18 07:49:36
     
          bits_hex: 0x1b00b5ac  | 0x1b  0xb5ac
    reg_difficulty:  46508
         trt_ratio: 0.842430555556  trt_ratio*BITS: 0x990b
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 59     blk_ht: 120960
               seconds: 1019004
     from 2011-04-18 07:49:36
       to 2011-04-30 02:53:00
     
          bits_hex: 0x1b0098fa  | 0x1b  0x98fa
    reg_difficulty:  39162
         trt_ratio: 0.697638888889  trt_ratio*BITS: 0x6ab8
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 60     blk_ht: 122976
               seconds: 843864
     from 2011-04-30 02:53:00
       to 2011-05-09 21:17:24
     
          bits_hex: 0x1a6a93b3  | 0x1a  0x6a93b3
    reg_difficulty:  6984627
         trt_ratio: 0.645207506614  trt_ratio*BITS: 0x44c3a5
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 61     blk_ht: 124992
               seconds: 780443
     from 2011-05-09 21:17:24
       to 2011-05-18 22:04:47
     
          bits_hex: 0x1a44b9f2  | 0x1a  0x44b9f2
    reg_difficulty:  4504050
         trt_ratio: 0.56136656746  trt_ratio*BITS: 0x2694a7
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 62     blk_ht: 127008
               seconds: 679029
     from 2011-05-18 22:04:47
       to 2011-05-26 18:41:56
     
          bits_hex: 0x1a269421  | 0x1a  0x269421
    reg_difficulty:  2528289
         trt_ratio: 0.767021329365  trt_ratio*BITS: 0x1d9733
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 63     blk_ht: 129024
               seconds: 927789
     from 2011-05-26 18:41:56
       to 2011-06-06 12:25:05
     
          bits_hex: 0x1a1d932f  | 0x1a  0x1d932f
    reg_difficulty:  1938223
         trt_ratio: 0.647047784392  trt_ratio*BITS: 0x1322ea
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 64     blk_ht: 131040
               seconds: 782669
     from 2011-06-06 12:25:05
       to 2011-06-15 13:49:34
     
          bits_hex: 0x1a132185  | 0x1a  0x132185
    reg_difficulty:  1253765
         trt_ratio: 0.636697255291  trt_ratio*BITS: 0xc2e3c
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 65     blk_ht: 133056
               seconds: 770149
     from 2011-06-15 13:49:34
       to 2011-06-24 11:45:23
     
          bits_hex: 0x1a0c2a12  | 0x1a  0xc2a12
    reg_difficulty:  797202
         trt_ratio: 0.883451554233  trt_ratio*BITS: 0xabf21
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 66     blk_ht: 135072
               seconds: 1068623
     from 2011-06-24 11:45:23
       to 2011-07-06 20:35:46
     
          bits_hex: 0x1a0abbcf  | 0x1a  0xabbcf
    reg_difficulty:  703439
         trt_ratio: 0.924969411376  trt_ratio*BITS: 0x9eda3
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 67     blk_ht: 137088
               seconds: 1118843
     from 2011-07-06 20:35:46
       to 2011-07-19 19:23:09
     
          bits_hex: 0x1a09ec04  | 0x1a  0x9ec04
    reg_difficulty:  650244
         trt_ratio: 0.895246362434  trt_ratio*BITS: 0x8e1f0
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 68     blk_ht: 139104
               seconds: 1082890
     from 2011-07-19 19:23:09
       to 2011-08-01 08:11:19
     
          bits_hex: 0x1a08e1e5  | 0x1a  0x8e1e5
    reg_difficulty:  582117
         trt_ratio: 1.04630869709  trt_ratio*BITS: 0x94b32
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 69     blk_ht: 141120
               seconds: 1265615
     from 2011-08-01 08:11:19
       to 2011-08-15 23:44:54
     
          bits_hex: 0x1a094a86  | 0x1a  0x94a86
    reg_difficulty:  608902
         trt_ratio: 1.0163764881  trt_ratio*BITS: 0x97179
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 70     blk_ht: 143136
               seconds: 1229409
     from 2011-08-15 23:44:54
       to 2011-08-30 05:15:03
     
          bits_hex: 0x1a096fe3  | 0x1a  0x96fe3
    reg_difficulty:  618467
         trt_ratio: 1.01274222884  trt_ratio*BITS: 0x98eab
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 71     blk_ht: 145152
               seconds: 1225013
     from 2011-08-30 05:15:03
       to 2011-09-13 09:31:56
     
          bits_hex: 0x1a098ea5  | 0x1a  0x98ea5
    reg_difficulty:  626341
         trt_ratio: 1.03944113757  trt_ratio*BITS: 0x9ef24
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 72     blk_ht: 147168
               seconds: 1257308
     from 2011-09-13 09:31:56
       to 2011-09-27 22:47:04
     
          bits_hex: 0x1a09ee5d  | 0x1a  0x9ee5d
    reg_difficulty:  650845
         trt_ratio: 1.15166253307  trt_ratio*BITS: 0xb6ff1
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 73     blk_ht: 149184
               seconds: 1393051
     from 2011-09-27 22:47:04
       to 2011-10-14 01:44:35
     
          bits_hex: 0x1a0b6d4b  | 0x1a  0xb6d4b
    reg_difficulty:  748875
         trt_ratio: 1.22012152778  trt_ratio*BITS: 0xdf136
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 74     blk_ht: 151200
               seconds: 1475859
     from 2011-10-14 01:44:35
       to 2011-10-31 03:42:14
     
          bits_hex: 0x1a0df0ca  | 0x1a  0xdf0ca
    reg_difficulty:  913610
         trt_ratio: 1.00961970899  trt_ratio*BITS: 0xe131e
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 75     blk_ht: 153216
               seconds: 1221236
     from 2011-10-31 03:42:14
       to 2011-11-14 06:56:10
     
          bits_hex: 0x1a0e119a  | 0x1a  0xe119a
    reg_difficulty:  922010
         trt_ratio: 1.09346064815  trt_ratio*BITS: 0xf6235
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 76     blk_ht: 155232
               seconds: 1322650
     from 2011-11-14 06:56:10
       to 2011-11-29 14:20:20
     
          bits_hex: 0x1a0f61b1  | 0x1a  0xf61b1
    reg_difficulty:  1008049
         trt_ratio: 0.944556051587  trt_ratio*BITS: 0xe875e
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 77     blk_ht: 157248
               seconds: 1142535
     from 2011-11-29 14:20:20
       to 2011-12-12 19:42:35
     
          bits_hex: 0x1a0e8668  | 0x1a  0xe8668
    reg_difficulty:  951912
         trt_ratio: 0.997065145503  trt_ratio*BITS: 0xe7b7e
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 78     blk_ht: 159264
               seconds: 1206050
     from 2011-12-12 19:42:35
       to 2011-12-26 18:43:25
     
          bits_hex: 0x1a0e76ba  | 0x1a  0xe76ba
    reg_difficulty:  947898
         trt_ratio: 0.927720734127  trt_ratio*BITS: 0xd6b18
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 79     blk_ht: 161280
               seconds: 1122171
     from 2011-12-26 18:43:25
       to 2012-01-08 18:26:16
     
          bits_hex: 0x1a0d69d7  | 0x1a  0xd69d7
    reg_difficulty:  879063
         trt_ratio: 0.956785714286  trt_ratio*BITS: 0xcd572
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 80     blk_ht: 163296
               seconds: 1157328
     from 2012-01-08 18:26:16
       to 2012-01-22 03:55:04
     
          bits_hex: 0x1a0cd43f  | 0x1a  0xcd43f
    reg_difficulty:  840767
         trt_ratio: 0.948294477513  trt_ratio*BITS: 0xc2a6e
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 81     blk_ht: 165312
               seconds: 1147057
     from 2012-01-22 03:55:04
       to 2012-02-04 10:32:41
     
          bits_hex: 0x1a0c290b  | 0x1a  0xc290b
    reg_difficulty:  796939
         trt_ratio: 1.00255787037  trt_ratio*BITS: 0xc3101
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 82     blk_ht: 167328
               seconds: 1212694
     from 2012-02-04 10:32:41
       to 2012-02-18 11:24:15
     
          bits_hex: 0x1a0c309c  | 0x1a  0xc309c
    reg_difficulty:  798876
         trt_ratio: 0.919709821429  trt_ratio*BITS: 0xb360e
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 83     blk_ht: 169344
               seconds: 1112481
     from 2012-02-18 11:24:15
       to 2012-03-02 08:25:36
     
          bits_hex: 0x1a0b350c  | 0x1a  0xb350c
    reg_difficulty:  734476
         trt_ratio: 0.999221230159  trt_ratio*BITS: 0xb32d0
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 84     blk_ht: 171360
               seconds: 1208658
     from 2012-03-02 08:25:36
       to 2012-03-16 08:09:54
     
          bits_hex: 0x1a0b3287  | 0x1a  0xb3287
    reg_difficulty:  733831
         trt_ratio: 0.921224371693  trt_ratio*BITS: 0xa50b7
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 85     blk_ht: 173376
               seconds: 1114313
     from 2012-03-16 08:09:54
       to 2012-03-29 05:41:47
     
          bits_hex: 0x1a0a507e  | 0x1a  0xa507e
    reg_difficulty:  675966
         trt_ratio: 1.03090443122  trt_ratio*BITS: 0xaa218
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 86     blk_ht: 175392
               seconds: 1246982
     from 2012-03-29 05:41:47
       to 2012-04-12 16:04:49
     
          bits_hex: 0x1a0aa1e3  | 0x1a  0xaa1e3
    reg_difficulty:  696803
         trt_ratio: 1.04636326058  trt_ratio*BITS: 0xb2015
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 87     blk_ht: 177408
               seconds: 1265681
     from 2012-04-12 16:04:49
       to 2012-04-27 07:39:30
     
          bits_hex: 0x1a0b1ef7  | 0x1a  0xb1ef7
    reg_difficulty:  728823
         trt_ratio: 0.87048859127  trt_ratio*BITS: 0x9ae40
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 88     blk_ht: 179424
               seconds: 1052943
     from 2012-04-27 07:39:30
       to 2012-05-09 12:08:33
     
          bits_hex: 0x1a09ae02  | 0x1a  0x9ae02
    reg_difficulty:  634370
         trt_ratio: 1.08938161376  trt_ratio*BITS: 0xa8b7f
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 89     blk_ht: 181440
               seconds: 1317716
     from 2012-05-09 12:08:33
       to 2012-05-24 18:10:29
     
          bits_hex: 0x1a0a8b5f  | 0x1a  0xa8b5f
    reg_difficulty:  691039
         trt_ratio: 1.0057010582  trt_ratio*BITS: 0xa9ac2
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 90     blk_ht: 183456
               seconds: 1216496
     from 2012-05-24 18:10:29
       to 2012-06-07 20:05:25
     
          bits_hex: 0x1a0a98d6  | 0x1a  0xa98d6
    reg_difficulty:  694486
         trt_ratio: 0.917194940476  trt_ratio*BITS: 0x9b833
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 91     blk_ht: 185472
               seconds: 1109439
     from 2012-06-07 20:05:25
       to 2012-06-20 16:16:04
     
          bits_hex: 0x1a09b78a  | 0x1a  0x9b78a
    reg_difficulty:  636810
         trt_ratio: 0.986174768519  trt_ratio*BITS: 0x99525
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 92     blk_ht: 187488
               seconds: 1192877
     from 2012-06-20 16:16:04
       to 2012-07-04 11:37:21
     
          bits_hex: 0x1a099431  | 0x1a  0x99431
    reg_difficulty:  627761
         trt_ratio: 0.938595403439  trt_ratio*BITS: 0x8fd9d
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 93     blk_ht: 189504
               seconds: 1135325
     from 2012-07-04 11:37:21
       to 2012-07-17 14:59:26
     
          bits_hex: 0x1a08fd2e  | 0x1a  0x8fd2e
    reg_difficulty:  589102
         trt_ratio: 0.917998511905  trt_ratio*BITS: 0x8407a
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 94     blk_ht: 191520
               seconds: 1110411
     from 2012-07-17 14:59:26
       to 2012-07-30 11:26:17
     
          bits_hex: 0x1a083cc9  | 0x1a  0x83cc9
    reg_difficulty:  539849
         trt_ratio: 0.930289351852  trt_ratio*BITS: 0x7a9c7
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 95     blk_ht: 193536
               seconds: 1125278
     from 2012-07-30 11:26:17
       to 2012-08-12 12:00:55
     
          bits_hex: 0x1a07a85e  | 0x1a  0x7a85e
    reg_difficulty:  501854
         trt_ratio: 0.898101851852  trt_ratio*BITS: 0x6e09c
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 96     blk_ht: 195552
               seconds: 1086344
     from 2012-08-12 12:00:55
       to 2012-08-25 01:46:39
     
          bits_hex: 0x1a06dfbe  | 0x1a  0x6dfbe
    reg_difficulty:  450494
         trt_ratio: 0.905950727513  trt_ratio*BITS: 0x63a3d
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 97     blk_ht: 197568
               seconds: 1095838
     from 2012-08-25 01:46:39
       to 2012-09-06 18:10:37
     
          bits_hex: 0x1a063a38  | 0x1a  0x63a38
    reg_difficulty:  408120
         trt_ratio: 0.940653108466  trt_ratio*BITS: 0x5db9b
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 98     blk_ht: 199584
               seconds: 1137814
     from 2012-09-06 18:10:37
       to 2012-09-19 22:14:11
     
          bits_hex: 0x1a05db8b  | 0x1a  0x5db8b
    reg_difficulty:  383883
         trt_ratio: 0.937342096561  trt_ratio*BITS: 0x57d95
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 99     blk_ht: 201600
               seconds: 1133809
     from 2012-09-19 22:14:11
       to 2012-10-03 01:11:00
     
          bits_hex: 0x1a057e08  | 0x1a  0x57e08
    reg_difficulty:  359944
         trt_ratio: 0.993310185185  trt_ratio*BITS: 0x574a0
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 100     blk_ht: 203616
               seconds: 1201508
     from 2012-10-03 01:11:00
       to 2012-10-16 22:56:08
     
          bits_hex: 0x1a0575ef  | 0x1a  0x575ef
    reg_difficulty:  357871
         trt_ratio: 0.932562003968  trt_ratio*BITS: 0x517a8
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 101     blk_ht: 205632
               seconds: 1128027
     from 2012-10-16 22:56:08
       to 2012-10-30 00:16:35
     
          bits_hex: 0x1a0513c5  | 0x1a  0x513c5
    reg_difficulty:  332741
         trt_ratio: 0.98130869709  trt_ratio*BITS: 0x4fb79
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 102     blk_ht: 207648
               seconds: 1186991
     from 2012-10-30 00:16:35
       to 2012-11-12 17:59:46
     
          bits_hex: 0x1a04faeb  | 0x1a  0x4faeb
    reg_difficulty:  326379
         trt_ratio: 0.979698247354  trt_ratio*BITS: 0x4e108
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 103     blk_ht: 209664
               seconds: 1185043
     from 2012-11-12 17:59:46
       to 2012-11-26 11:10:29
     
          bits_hex: 0x1a04e0ea  | 0x1a  0x4e0ea
    reg_difficulty:  319722
         trt_ratio: 1.02048941799  trt_ratio*BITS: 0x4fa80
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 104     blk_ht: 211680
               seconds: 1234384
     from 2012-11-26 11:10:29
       to 2012-12-10 18:03:33
     
          bits_hex: 0x1a04fa62  | 0x1a  0x4fa62
    reg_difficulty:  326242
         trt_ratio: 1.13105737434  trt_ratio*BITS: 0x5a166
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 105     blk_ht: 213696
               seconds: 1368127
     from 2012-12-10 18:03:33
       to 2012-12-26 14:05:40
     
          bits_hex: 0x1a05a16b  | 0x1a  0x5a16b
    reg_difficulty:  369003
         trt_ratio: 0.91839781746  trt_ratio*BITS: 0x52bcb
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 106     blk_ht: 215712
               seconds: 1110894
     from 2012-12-26 14:05:40
       to 2013-01-08 10:40:34
     
          bits_hex: 0x1a0529b1  | 0x1a  0x529b1
    reg_difficulty:  338353
         trt_ratio: 1.09528191138  trt_ratio*BITS: 0x5a79f
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 107     blk_ht: 217728
               seconds: 1324853
     from 2013-01-08 10:40:34
       to 2013-01-23 18:41:27
     
          bits_hex: 0x1a05a6b1  | 0x1a  0x5a6b1
    reg_difficulty:  370353
         trt_ratio: 0.906629464286  trt_ratio*BITS: 0x51f9c
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 108     blk_ht: 219744
               seconds: 1096659
     from 2013-01-23 18:41:27
       to 2013-02-05 11:19:06
     
          bits_hex: 0x1a051f3c  | 0x1a  0x51f3c
    reg_difficulty:  335676
         trt_ratio: 0.897258597884  trt_ratio*BITS: 0x49884
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 109     blk_ht: 221760
               seconds: 1085324
     from 2013-02-05 11:19:06
       to 2013-02-18 00:47:50
     
          bits_hex: 0x1a04985c  | 0x1a  0x4985c
    reg_difficulty:  301148
         trt_ratio: 0.836056547619  trt_ratio*BITS: 0x3d780
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 110     blk_ht: 223776
               seconds: 1011294
     from 2013-02-18 00:47:50
       to 2013-03-01 17:42:44
     
          bits_hex: 0x1a03d74b  | 0x1a  0x3d74b
    reg_difficulty:  251723
         trt_ratio: 0.901274801587  trt_ratio*BITS: 0x37637
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 111     blk_ht: 225792
               seconds: 1090182
     from 2013-03-01 17:42:44
       to 2013-03-14 08:32:26
     
          bits_hex: 0x1a0375fa  | 0x1a  0x375fa
    reg_difficulty:  226810
         trt_ratio: 0.724602347884  trt_ratio*BITS: 0x281fb
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 112     blk_ht: 227808
               seconds: 876479
     from 2013-03-14 08:32:26
       to 2013-03-24 12:00:25
     
          bits_hex: 0x1a02816e  | 0x1a  0x2816e
    reg_difficulty:  164206
         trt_ratio: 0.87402281746  trt_ratio*BITS: 0x2309f
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 113     blk_ht: 229824
               seconds: 1057218
     from 2013-03-24 12:00:25
       to 2013-04-05 17:40:43
     
          bits_hex: 0x1a022fbe  | 0x1a  0x22fbe
    reg_difficulty:  143294
         trt_ratio: 0.85523396164  trt_ratio*BITS: 0x1deb5
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 114     blk_ht: 231840
               seconds: 1034491
     from 2013-04-05 17:40:43
       to 2013-04-17 17:02:14
     
          bits_hex: 0x1a01de94  | 0x1a  0x1de94
    reg_difficulty:  122516
         trt_ratio: 0.891482308201  trt_ratio*BITS: 0x1aaa4
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 115     blk_ht: 233856
               seconds: 1078337
     from 2013-04-17 17:02:14
       to 2013-04-30 04:34:31
     
          bits_hex: 0x1a01aa3d  | 0x1a  0x1aa3d
    reg_difficulty:  109117
         trt_ratio: 0.900836640212  trt_ratio*BITS: 0x17ff8
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 116     blk_ht: 235872
               seconds: 1089652
     from 2013-04-30 04:34:31
       to 2013-05-12 19:15:23
     
          bits_hex: 0x1a017fe9  | 0x1a  0x17fe9
    reg_difficulty:  98281
         trt_ratio: 0.920653935185  trt_ratio*BITS: 0x16172
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 117     blk_ht: 237888
               seconds: 1113623
     from 2013-05-12 19:15:23
       to 2013-05-25 16:35:46
     
          bits_hex: 0x1a016164  | 0x1a  0x16164
    reg_difficulty:  90468
         trt_ratio: 0.779242724868  trt_ratio*BITS: 0x11360
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 118     blk_ht: 239904
               seconds: 942572
     from 2013-05-25 16:35:46
       to 2013-06-05 14:25:18
     
          bits_hex: 0x1a011337  | 0x1a  0x11337
    reg_difficulty:  70455
         trt_ratio: 0.807156084656  trt_ratio*BITS: 0xde24
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 119     blk_ht: 241920
               seconds: 976336
     from 2013-06-05 14:25:18
       to 2013-06-16 21:37:34
     
          bits_hex: 0x1a00de15  | 0x1a  0xde15
    reg_difficulty:  56853
         trt_ratio: 0.906970072751  trt_ratio*BITS: 0xc96b
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 120     blk_ht: 243936
               seconds: 1097071
     from 2013-06-16 21:37:34
       to 2013-06-29 14:22:05
     
          bits_hex: 0x1a00c94e  | 0x1a  0xc94e
    reg_difficulty:  51534
         trt_ratio: 0.81550760582  trt_ratio*BITS: 0xa42a
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 121     blk_ht: 245952
               seconds: 986438
     from 2013-06-29 14:22:05
       to 2013-07-11 00:22:43
     
          bits_hex: 0x1a00a429  | 0x1a  0xa429
    reg_difficulty:  42025
         trt_ratio: 0.838015873016  trt_ratio*BITS: 0x8991
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 122     blk_ht: 247968
               seconds: 1013664
     from 2013-07-11 00:22:43
       to 2013-07-22 17:57:07
     
          bits_hex: 0x1a008968  | 0x1a  0x8968
    reg_difficulty:  35176
         trt_ratio: 0.836051587302  trt_ratio*BITS: 0x72e0
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 123     blk_ht: 249984
               seconds: 1011288
     from 2013-07-22 17:57:07
       to 2013-08-03 10:51:55
     
          bits_hex: 0x1972dbf2  | 0x19  0x72dbf2
    reg_difficulty:  7527410
         trt_ratio: 0.736090443122  trt_ratio*BITS: 0x548bf6
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 124     blk_ht: 252000
               seconds: 890375
     from 2013-08-03 10:51:55
       to 2013-08-13 18:11:30
     
          bits_hex: 0x19548732  | 0x19  0x548732
    reg_difficulty:  5539634
         trt_ratio: 0.773668154762  trt_ratio*BITS: 0x41658e
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 125     blk_ht: 254016
               seconds: 935829
     from 2013-08-13 18:11:30
       to 2013-08-24 14:08:39
     
          bits_hex: 0x19415257  | 0x19  0x415257
    reg_difficulty:  4280919
         trt_ratio: 0.756564980159  trt_ratio*BITS: 0x316b89
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 126     blk_ht: 256032
               seconds: 915141
     from 2013-08-24 14:08:39
       to 2013-09-04 04:21:00
     
          bits_hex: 0x1931679c  | 0x19  0x31679c
    reg_difficulty:  3237788
         trt_ratio: 0.771980820106  trt_ratio*BITS: 0x2623b6
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 127     blk_ht: 258048
               seconds: 933788
     from 2013-09-04 04:21:00
       to 2013-09-14 23:44:08
     
          bits_hex: 0x19262222  | 0x19  0x262222
    reg_difficulty:  2499106
         trt_ratio: 0.757190806878  trt_ratio*BITS: 0x1cdfcc
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 128     blk_ht: 260064
               seconds: 915898
     from 2013-09-14 23:44:08
       to 2013-09-25 14:09:06
     
          bits_hex: 0x191cdc20  | 0x19  0x1cdc20
    reg_difficulty:  1891360
         trt_ratio: 0.787372685185  trt_ratio*BITS: 0x16b935
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 129     blk_ht: 262080
               seconds: 952406
     from 2013-09-25 14:09:06
       to 2013-10-06 14:42:32
     
          bits_hex: 0x1916b0ca  | 0x19  0x16b0ca
    reg_difficulty:  1487050
         trt_ratio: 0.707040343915  trt_ratio*BITS: 0x100b0c
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 130     blk_ht: 264096
               seconds: 855236
     from 2013-10-06 14:42:32
       to 2013-10-16 12:16:28
     
          bits_hex: 0x19100ab6  | 0x19  0x100ab6
    reg_difficulty:  1051318
         trt_ratio: 0.684923941799  trt_ratio*BITS: 0xafcc8
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 131     blk_ht: 266112
               seconds: 828484
     from 2013-10-16 12:16:28
       to 2013-10-26 02:24:32
     
          bits_hex: 0x190afc85  | 0x19  0xafc85
    reg_difficulty:  720005
         trt_ratio: 0.766245866402  trt_ratio*BITS: 0x86b14
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 132     blk_ht: 268128
               seconds: 926851
     from 2013-10-26 02:24:32
       to 2013-11-05 19:52:03
     
          bits_hex: 0x190867f3  | 0x19  0x867f3
    reg_difficulty:  550899
         trt_ratio: 0.841911375661  trt_ratio*BITS: 0x713c0
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 133     blk_ht: 270144
               seconds: 1018376
     from 2013-11-05 19:52:03
       to 2013-11-17 14:44:59
     
          bits_hex: 0x19070bfb  | 0x19  0x70bfb
    reg_difficulty:  461819
         trt_ratio: 0.862391699735  trt_ratio*BITS: 0x613bc
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 134     blk_ht: 272160
               seconds: 1043149
     from 2013-11-17 14:44:59
       to 2013-11-29 16:30:48
     
          bits_hex: 0x19061242  | 0x19  0x61242
    reg_difficulty:  397890
         trt_ratio: 0.778801256614  trt_ratio*BITS: 0x4ba75
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 135     blk_ht: 274176
               seconds: 942038
     from 2013-11-29 16:30:48
       to 2013-12-10 14:11:26
     
          bits_hex: 0x1904ba6e  | 0x19  0x4ba6e
    reg_difficulty:  309870
         trt_ratio: 0.770854828042  trt_ratio*BITS: 0x3a510
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 136     blk_ht: 276192
               seconds: 932426
     from 2013-12-10 14:11:26
       to 2013-12-21 09:11:52
     
          bits_hex: 0x1903a30c  | 0x19  0x3a30c
    reg_difficulty:  238348
         trt_ratio: 0.832676917989  trt_ratio*BITS: 0x30742
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 137     blk_ht: 278208
               seconds: 1007206
     from 2013-12-21 09:11:52
       to 2014-01-02 00:58:38
     
          bits_hex: 0x1903071f  | 0x19  0x3071f
    reg_difficulty:  198431
         trt_ratio: 0.792743055556  trt_ratio*BITS: 0x26678
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 138     blk_ht: 280224
               seconds: 958902
     from 2014-01-02 00:58:38
       to 2014-01-13 03:20:20
     
          bits_hex: 0x19026666  | 0x19  0x26666
    reg_difficulty:  157286
         trt_ratio: 0.815894510582  trt_ratio*BITS: 0x1f548
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 139     blk_ht: 282240
               seconds: 986906
     from 2014-01-13 03:20:20
       to 2014-01-24 13:28:46
     
          bits_hex: 0x1901f52c  | 0x19  0x1f52c
    reg_difficulty:  128300
         trt_ratio: 0.838566468254  trt_ratio*BITS: 0x1a444
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 140     blk_ht: 284256
               seconds: 1014330
     from 2014-01-24 13:28:46
       to 2014-02-05 07:14:16
     
          bits_hex: 0x1901a36e  | 0x19  0x1a36e
    reg_difficulty:  107374
         trt_ratio: 0.837783564815  trt_ratio*BITS: 0x15f64
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 141     blk_ht: 286272
               seconds: 1013383
     from 2014-02-05 07:14:16
       to 2014-02-17 00:43:59
     
          bits_hex: 0x19015f53  | 0x19  0x15f53
    reg_difficulty:  89939
         trt_ratio: 0.82072255291  trt_ratio*BITS: 0x12056
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 142     blk_ht: 288288
               seconds: 992746
     from 2014-02-17 00:43:59
       to 2014-02-28 12:29:45
     
          bits_hex: 0x19012026  | 0x19  0x12026
    reg_difficulty:  73766
         trt_ratio: 0.897965443122  trt_ratio*BITS: 0x102bf
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 143     blk_ht: 290304
               seconds: 1086179
     from 2014-02-28 12:29:45
       to 2014-03-13 02:12:44
     
          bits_hex: 0x190102b1  | 0x19  0x102b1
    reg_difficulty:  66225
         trt_ratio: 0.849097222222  trt_ratio*BITS: 0xdba7
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 144     blk_ht: 292320
               seconds: 1027068
     from 2014-03-13 02:12:44
       to 2014-03-24 23:30:32
     
          bits_hex: 0x1900db99  | 0x19  0xdb99
    reg_difficulty:  56217
         trt_ratio: 0.818986441799  trt_ratio*BITS: 0xb3d8
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 145     blk_ht: 294336
               seconds: 990646
     from 2014-03-24 23:30:32
       to 2014-04-05 10:41:18
     
          bits_hex: 0x1900b3aa  | 0x19  0xb3aa
    reg_difficulty:  45994
         trt_ratio: 0.877288359788  trt_ratio*BITS: 0x9d9e
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 146     blk_ht: 296352
               seconds: 1061168
     from 2014-04-05 10:41:18
       to 2014-04-17 17:27:26
     
          bits_hex: 0x19009d8c  | 0x19  0x9d8c
    reg_difficulty:  40332
         trt_ratio: 0.872626488095  trt_ratio*BITS: 0x897a
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 147     blk_ht: 298368
               seconds: 1055529
     from 2014-04-17 17:27:26
       to 2014-04-29 22:39:35
     
          bits_hex: 0x1900896c  | 0x19  0x896c
    reg_difficulty:  35180
         trt_ratio: 0.903716931217  trt_ratio*BITS: 0x7c30
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 148     blk_ht: 300384
               seconds: 1093136
     from 2014-04-29 22:39:35
       to 2014-05-12 14:18:31
     
          bits_hex: 0x187c3053  | 0x18  0x7c3053
    reg_difficulty:  8138835
         trt_ratio: 0.846923776455  trt_ratio*BITS: 0x692dac
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 149     blk_ht: 302400
               seconds: 1024439
     from 2014-05-12 14:18:31
       to 2014-05-24 10:52:30
     
          bits_hex: 0x18692842  | 0x18  0x692842
    reg_difficulty:  6891586
         trt_ratio: 0.889751157407  trt_ratio*BITS: 0x5d9054
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 150     blk_ht: 304416
               seconds: 1076243
     from 2014-05-24 10:52:30
       to 2014-06-05 21:49:53
     
          bits_hex: 0x185d859a  | 0x18  0x5d859a
    reg_difficulty:  6129050
         trt_ratio: 0.873253141534  trt_ratio*BITS: 0x51ab14
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 151     blk_ht: 306432
               seconds: 1056287
     from 2014-06-05 21:49:53
       to 2014-06-18 03:14:40
     
          bits_hex: 0x1851aba2  | 0x18  0x51aba2
    reg_difficulty:  5352354
         trt_ratio: 0.800492724868  trt_ratio*BITS: 0x416068
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 152     blk_ht: 308448
               seconds: 968276
     from 2014-06-18 03:14:40
       to 2014-06-29 08:12:36
     
          bits_hex: 0x18415fd1  | 0x18  0x415fd1
    reg_difficulty:  4284369
         trt_ratio: 0.972501653439  trt_ratio*BITS: 0x3f939b
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 153     blk_ht: 310464
               seconds: 1176338
     from 2014-06-29 08:12:36
       to 2014-07-12 22:58:14
     
          bits_hex: 0x183f6be6  | 0x18  0x3f6be6
    reg_difficulty:  4156390
         trt_ratio: 0.925262896825  trt_ratio*BITS: 0x3aae79
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 154     blk_ht: 312480
               seconds: 1119198
     from 2014-07-12 22:58:14
       to 2014-07-25 21:51:32
     
          bits_hex: 0x183aaea2  | 0x18  0x3aaea2
    reg_difficulty:  3845794
         trt_ratio: 0.949917328042  trt_ratio*BITS: 0x37be42
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 155     blk_ht: 314496
               seconds: 1149020
     from 2014-07-25 21:51:32
       to 2014-08-08 05:01:52
     
          bits_hex: 0x1837ba62  | 0x18  0x37ba62
    reg_difficulty:  3652194
         trt_ratio: 0.827881944444  trt_ratio*BITS: 0x2e22e1
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 156     blk_ht: 316512
               seconds: 1001406
     from 2014-08-08 05:01:52
       to 2014-08-19 19:11:58
     
          bits_hex: 0x182e1c58  | 0x18  0x2e1c58
    reg_difficulty:  3021912
         trt_ratio: 0.869406415344  trt_ratio*BITS: 0x2816c5
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 157     blk_ht: 318528
               seconds: 1051634
     from 2014-08-19 19:11:58
       to 2014-08-31 23:19:12
     
          bits_hex: 0x182815ee  | 0x18  0x2815ee
    reg_difficulty:  2627054
         trt_ratio: 0.919100529101  trt_ratio*BITS: 0x24d7be
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 158     blk_ht: 320544
               seconds: 1111744
     from 2014-08-31 23:19:12
       to 2014-09-13 20:08:16
     
          bits_hex: 0x1824dbe9  | 0x18  0x24dbe9
    reg_difficulty:  2415593
         trt_ratio: 0.860767195767  trt_ratio*BITS: 0x1fba1f
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 159     blk_ht: 322560
               seconds: 1041184
     from 2014-09-13 20:08:16
       to 2014-09-25 21:21:20
     
          bits_hex: 0x181fb893  | 0x18  0x1fb893
    reg_difficulty:  2078867
         trt_ratio: 0.990233134921  trt_ratio*BITS: 0x1f6942
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 160     blk_ht: 324576
               seconds: 1197786
     from 2014-09-25 21:21:20
       to 2014-10-09 18:04:26
     
          bits_hex: 0x181f6973  | 0x18  0x1f6973
    reg_difficulty:  2058611
         trt_ratio: 0.973484623016  trt_ratio*BITS: 0x1e943a
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 161     blk_ht: 326592
               seconds: 1177527
     from 2014-10-09 18:04:26
       to 2014-10-23 09:09:53
     
          bits_hex: 0x181e8dc0  | 0x18  0x1e8dc0
    reg_difficulty:  2002368
         trt_ratio: 0.908761574074  trt_ratio*BITS: 0x1bc41b
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 162     blk_ht: 328608
               seconds: 1099238
     from 2014-10-23 09:09:53
       to 2014-11-05 02:30:31
     
          bits_hex: 0x181bc330  | 0x18  0x1bc330
    reg_difficulty:  1819440
         trt_ratio: 0.984203869048  trt_ratio*BITS: 0x1b52eb
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 163     blk_ht: 330624
               seconds: 1190493
     from 2014-11-05 02:30:31
       to 2014-11-18 21:12:04
     
          bits_hex: 0x181b4861  | 0x18  0x1b4861
    reg_difficulty:  1788001
         trt_ratio: 1.00742476852  trt_ratio*BITS: 0x1b7c3c
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 164     blk_ht: 332640
               seconds: 1218581
     from 2014-11-18 21:12:04
       to 2014-12-02 23:41:45
     
          bits_hex: 0x181b7b74  | 0x18  0x1b7b74
    reg_difficulty:  1801076
         trt_ratio: 1.01464864418  trt_ratio*BITS: 0x1be283
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 165     blk_ht: 334656
               seconds: 1227319
     from 2014-12-02 23:41:45
       to 2014-12-17 04:37:04
     
          bits_hex: 0x181bdd7c  | 0x18  0x1bdd7c
    reg_difficulty:  1826172
         trt_ratio: 0.97103505291  trt_ratio*BITS: 0x1b0edd
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 166     blk_ht: 336672
               seconds: 1174564
     from 2014-12-17 04:37:04
       to 2014-12-30 18:53:08
     
          bits_hex: 0x181b0dca  | 0x18  0x1b0dca
    reg_difficulty:  1773002
         trt_ratio: 0.924673445767  trt_ratio*BITS: 0x190417
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 167     blk_ht: 338688
               seconds: 1118485
     from 2014-12-30 18:53:08
       to 2015-01-12 17:34:33
     
          bits_hex: 0x1819012f  | 0x18  0x19012f
    reg_difficulty:  1638703
         trt_ratio: 1.06553654101  trt_ratio*BITS: 0x1aa4b1
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 168     blk_ht: 340704
               seconds: 1288873
     from 2015-01-12 17:34:33
       to 2015-01-27 15:35:46
     
          bits_hex: 0x181aa3c0  | 0x18  0x1aa3c0
    reg_difficulty:  1745856
         trt_ratio: 0.928793816138  trt_ratio*BITS: 0x18be24
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 169     blk_ht: 342720
               seconds: 1123469
     from 2015-01-27 15:35:46
       to 2015-02-09 15:40:15
     
          bits_hex: 0x1818bb87  | 0x18  0x18bb87
    reg_difficulty:  1620871
         trt_ratio: 0.9528125  trt_ratio*BITS: 0x1790c2
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 170     blk_ht: 344736
               seconds: 1152522
     from 2015-02-09 15:40:15
       to 2015-02-22 23:48:57
     
          bits_hex: 0x18178d3a  | 0x18  0x178d3a
    reg_difficulty:  1543482
         trt_ratio: 0.984811507937  trt_ratio*BITS: 0x1731a6
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 171     blk_ht: 346752
               seconds: 1191228
     from 2015-02-22 23:48:57
       to 2015-03-08 18:42:45
     
          bits_hex: 0x18172ec0  | 0x18  0x172ec0
    reg_difficulty:  1519296
         trt_ratio: 1.01541501323  trt_ratio*BITS: 0x178a3b
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 172     blk_ht: 348768
               seconds: 1228246
     from 2015-03-08 18:42:45
       to 2015-03-22 23:53:31
     
          bits_hex: 0x181788f2  | 0x18  0x1788f2
    reg_difficulty:  1542386
         trt_ratio: 0.944885912698  trt_ratio*BITS: 0x163ce2
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 173     blk_ht: 350784
               seconds: 1142934
     from 2015-03-22 23:53:31
       to 2015-04-05 05:22:25
     
          bits_hex: 0x18163c71  | 0x18  0x163c71
    reg_difficulty:  1457265
         trt_ratio: 1.03882357804  trt_ratio*BITS: 0x171971
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 174     blk_ht: 352800
               seconds: 1256561
     from 2015-04-05 05:22:25
       to 2015-04-19 18:25:06
     
          bits_hex: 0x181717f0  | 0x18  0x1717f0
    reg_difficulty:  1513456
         trt_ratio: 0.99986359127  trt_ratio*BITS: 0x171721
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 175     blk_ht: 354816
               seconds: 1209435
     from 2015-04-19 18:25:06
       to 2015-05-03 18:22:21
     
          bits_hex: 0x181713dd  | 0x18  0x1713dd
    reg_difficulty:  1512413
         trt_ratio: 0.976431878307  trt_ratio*BITS: 0x1688a0
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 176     blk_ht: 356832
               seconds: 1181092
     from 2015-05-03 18:22:21
       to 2015-05-17 10:27:13
     
          bits_hex: 0x181686f5  | 0x18  0x1686f5
    reg_difficulty:  1476341
         trt_ratio: 1.02575396825  trt_ratio*BITS: 0x171b7a
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 177     blk_ht: 358848
               seconds: 1240752
     from 2015-05-17 10:27:13
       to 2015-05-31 19:06:25
     
          bits_hex: 0x18171a8b  | 0x18  0x171a8b
    reg_difficulty:  1514123
         trt_ratio: 0.957819113757  trt_ratio*BITS: 0x16210f
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 178     blk_ht: 360864
               seconds: 1158578
     from 2015-05-31 19:06:25
       to 2015-06-14 04:56:03
     
          bits_hex: 0x18162043  | 0x18  0x162043
    reg_difficulty:  1450051
         trt_ratio: 1.00651703042  trt_ratio*BITS: 0x16452d
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 179     blk_ht: 362880
               seconds: 1217483
     from 2015-06-14 04:56:03
       to 2015-06-28 07:07:26
     
          bits_hex: 0x1816418e  | 0x18  0x16418e
    reg_difficulty:  1458574
         trt_ratio: 0.968121693122  trt_ratio*BITS: 0x158bed
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 180     blk_ht: 364896
               seconds: 1171040
     from 2015-06-28 07:07:26
       to 2015-07-11 20:24:46
     
          bits_hex: 0x181586c8  | 0x18  0x1586c8
    reg_difficulty:  1410760
         trt_ratio: 0.977181712963  trt_ratio*BITS: 0x150908
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 181     blk_ht: 366912
               seconds: 1181999
     from 2015-07-11 20:24:46
       to 2015-07-25 12:44:45
     
          bits_hex: 0x18150815  | 0x18  0x150815
    reg_difficulty:  1378325
         trt_ratio: 0.99259672619  trt_ratio*BITS: 0x14e038
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 182     blk_ht: 368928
               seconds: 1200645
     from 2015-07-25 12:44:45
       to 2015-08-08 10:15:30
     
          bits_hex: 0x1814dd04  | 0x18  0x14dd04
    reg_difficulty:  1367300
         trt_ratio: 0.971935350529  trt_ratio*BITS: 0x14471f
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 183     blk_ht: 370944
               seconds: 1175653
     from 2015-08-08 10:15:30
       to 2015-08-22 00:49:43
     
          bits_hex: 0x181443c4  | 0x18  0x1443c4
    reg_difficulty:  1328068
         trt_ratio: 0.95314484127  trt_ratio*BITS: 0x1350b1
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 184     blk_ht: 372960
               seconds: 1152924
     from 2015-08-22 00:49:43
       to 2015-09-04 09:05:07
     
          bits_hex: 0x18134dc1  | 0x18  0x134dc1
    reg_difficulty:  1265089
         trt_ratio: 0.960562996032  trt_ratio*BITS: 0x128add
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 185     blk_ht: 374976
               seconds: 1161897
     from 2015-09-04 09:05:07
       to 2015-09-17 19:50:04
     
          bits_hex: 0x181287ba  | 0x18  0x1287ba
    reg_difficulty:  1214394
         trt_ratio: 0.976344246032  trt_ratio*BITS: 0x121782
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 186     blk_ht: 376992
               seconds: 1180986
     from 2015-09-17 19:50:04
       to 2015-10-01 11:53:10
     
          bits_hex: 0x18121472  | 0x18  0x121472
    reg_difficulty:  1184882
         trt_ratio: 0.998975694444  trt_ratio*BITS: 0x120fb4
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 187     blk_ht: 379008
               seconds: 1208361
     from 2015-10-01 11:53:10
       to 2015-10-15 11:32:31
     
          bits_hex: 0x18120f14  | 0x18  0x120f14
    reg_difficulty:  1183508
         trt_ratio: 0.978798776455  trt_ratio*BITS: 0x11ad10
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 188     blk_ht: 381024
               seconds: 1183955
     from 2015-10-15 11:32:31
       to 2015-10-29 04:25:06
     
          bits_hex: 0x1811a954  | 0x18  0x11a954
    reg_difficulty:  1157460
         trt_ratio: 0.945755621693  trt_ratio*BITS: 0x10b412
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 189     blk_ht: 383040
               seconds: 1143986
     from 2015-10-29 04:25:06
       to 2015-11-11 10:11:32
     
          bits_hex: 0x1810b289  | 0x18  0x10b289
    reg_difficulty:  1094281
         trt_ratio: 0.905894510582  trt_ratio*BITS: 0xf2047
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 190     blk_ht: 385056
               seconds: 1095770
     from 2015-11-11 10:11:32
       to 2015-11-24 02:34:22
     
          bits_hex: 0x180f1e76  | 0x18  0xf1e76
    reg_difficulty:  990838
         trt_ratio: 0.919469246032  trt_ratio*BITS: 0xde6c5
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 191     blk_ht: 387072
               seconds: 1112190
     from 2015-11-24 02:34:22
       to 2015-12-06 23:30:52
     
          bits_hex: 0x180de64f  | 0x18  0xde64f
    reg_difficulty:  910927
         trt_ratio: 0.847087466931  trt_ratio*BITS: 0xbc632
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 192     blk_ht: 389088
               seconds: 1024637
     from 2015-12-06 23:30:52
       to 2015-12-18 20:08:09
     
          bits_hex: 0x180bc409  | 0x18  0xbc409
    reg_difficulty:  771081
         trt_ratio: 0.900523313492  trt_ratio*BITS: 0xa9868
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 193     blk_ht: 391104
               seconds: 1089273
     from 2015-12-18 20:08:09
       to 2015-12-31 10:42:42
     
          bits_hex: 0x180a9591  | 0x18  0xa9591
    reg_difficulty:  693649
         trt_ratio: 0.916514550265  trt_ratio*BITS: 0x9b35b
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 194     blk_ht: 393120
               seconds: 1108616
     from 2015-12-31 10:42:42
       to 2016-01-13 06:39:38
     
          bits_hex: 0x1809b31b  | 0x18  0x9b31b
    reg_difficulty:  635675
         trt_ratio: 0.94540922619  trt_ratio*BITS: 0x92b8d
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 195     blk_ht: 395136
               seconds: 1143567
     from 2016-01-13 06:39:38
       to 2016-01-26 12:19:05
     
          bits_hex: 0x180928f0  | 0x18  0x928f0
    reg_difficulty:  600304
         trt_ratio: 0.833015046296  trt_ratio*BITS: 0x7a15e
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 196     blk_ht: 397152
               seconds: 1007615
     from 2016-01-26 12:19:05
       to 2016-02-07 04:12:40
     
          bits_hex: 0x1807a114  | 0x18  0x7a114
    reg_difficulty:  499988
         trt_ratio: 0.882023809524  trt_ratio*BITS: 0x6baa9
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 197     blk_ht: 399168
               seconds: 1066896
     from 2016-02-07 04:12:40
       to 2016-02-19 12:34:16
     
          bits_hex: 0x1806b99f  | 0x18  0x6b99f
    reg_difficulty:  440735
         trt_ratio: 1.03232473545  trt_ratio*BITS: 0x6f145
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 198     blk_ht: 401184
               seconds: 1248700
     from 2016-02-19 12:34:16
       to 2016-03-04 23:25:56
     
          bits_hex: 0x1806f0a8  | 0x18  0x6f0a8
    reg_difficulty:  454824
         trt_ratio: 0.957433862434  trt_ratio*BITS: 0x6a507
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 199     blk_ht: 403200
               seconds: 1158112
     from 2016-03-04 23:25:56
       to 2016-03-18 09:07:48
     
          bits_hex: 0x1806a4c3  | 0x18  0x6a4c3
    reg_difficulty:  435395
         trt_ratio: 0.992399966931  trt_ratio*BITS: 0x697d5
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 200     blk_ht: 405216
               seconds: 1200407
     from 2016-03-18 09:07:48
       to 2016-04-01 06:34:35
     
          bits_hex: 0x180696f4  | 0x18  0x696f4
    reg_difficulty:  431860
         trt_ratio: 0.93408234127  trt_ratio*BITS: 0x627c0
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 201     blk_ht: 407232
               seconds: 1129866
     from 2016-04-01 06:34:35
       to 2016-04-14 08:25:41
     
          bits_hex: 0x1806274b  | 0x18  0x6274b
    reg_difficulty:  403275
         trt_ratio: 1.00013971561  trt_ratio*BITS: 0x62783
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 202     blk_ht: 409248
               seconds: 1209769
     from 2016-04-14 08:25:41
       to 2016-04-28 08:28:30
     
          bits_hex: 0x18062776  | 0x18  0x62776
    reg_difficulty:  403318
         trt_ratio: 0.919937169312  trt_ratio*BITS: 0x5a953
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 203     blk_ht: 411264
               seconds: 1112756
     from 2016-04-28 08:28:30
       to 2016-05-11 05:34:26
     
          bits_hex: 0x1805a8fa  | 0x18  0x5a8fa
    reg_difficulty:  370938
         trt_ratio: 0.974619708995  trt_ratio*BITS: 0x58433
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 204     blk_ht: 413280
               seconds: 1178900
     from 2016-05-11 05:34:26
       to 2016-05-24 21:02:46
     
          bits_hex: 0x18058436  | 0x18  0x58436
    reg_difficulty:  361526
         trt_ratio: 1.01682539683  trt_ratio*BITS: 0x59bf8
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 205     blk_ht: 415296
               seconds: 1229952
     from 2016-05-24 21:02:46
       to 2016-06-08 02:41:58
     
          bits_hex: 0x18059ba0  | 0x18  0x59ba0
    reg_difficulty:  367520
         trt_ratio: 0.936359126984  trt_ratio*BITS: 0x54042
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 206     blk_ht: 417312
               seconds: 1132620
     from 2016-06-08 02:41:58
       to 2016-06-21 05:18:58
     
          bits_hex: 0x18053fd6  | 0x18  0x53fd6
    reg_difficulty:  344022
         trt_ratio: 0.981996527778  trt_ratio*BITS: 0x527a4
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 207     blk_ht: 419328
               seconds: 1187823
     from 2016-06-21 05:18:58
       to 2016-07-04 23:16:01
     
          bits_hex: 0x180526fd  | 0x18  0x526fd
    reg_difficulty:  337661
         trt_ratio: 1.00033151455  trt_ratio*BITS: 0x5276c
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 208     blk_ht: 421344
               seconds: 1210001
     from 2016-07-04 23:16:01
       to 2016-07-18 23:22:42
     
          bits_hex: 0x18052669  | 0x18  0x52669
    reg_difficulty:  337513
         trt_ratio: 1.05793650794  trt_ratio*BITS: 0x572cb
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 209     blk_ht: 423360
               seconds: 1279680
     from 2016-07-18 23:22:42
       to 2016-08-02 18:50:42
     
          bits_hex: 0x18057228  | 0x18  0x57228
    reg_difficulty:  356904
         trt_ratio: 0.928994708995  trt_ratio*BITS: 0x50f29
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 210     blk_ht: 425376
               seconds: 1123712
     from 2016-08-02 18:50:42
       to 2016-08-15 18:59:14
     
          bits_hex: 0x18050edc  | 0x18  0x50edc
    reg_difficulty:  331484
         trt_ratio: 0.98570353836  trt_ratio*BITS: 0x4fc58
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 211     blk_ht: 427392
               seconds: 1192307
     from 2016-08-15 18:59:14
       to 2016-08-29 14:11:01
     
          bits_hex: 0x1804fb08  | 0x18  0x4fb08
    reg_difficulty:  326408
         trt_ratio: 0.977584325397  trt_ratio*BITS: 0x4de73
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 212     blk_ht: 429408
               seconds: 1182486
     from 2016-08-29 14:11:01
       to 2016-09-12 06:39:07
     
          bits_hex: 0x1804de5e  | 0x18  0x4de5e
    reg_difficulty:  319070
         trt_ratio: 0.936398809524  trt_ratio*BITS: 0x48f18
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 213     blk_ht: 431424
               seconds: 1132668
     from 2016-09-12 06:39:07
       to 2016-09-25 09:16:55
     
          bits_hex: 0x18048ed4  | 0x18  0x48ed4
    reg_difficulty:  298708
         trt_ratio: 0.933362268519  trt_ratio*BITS: 0x44112
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 214     blk_ht: 433440
               seconds: 1128995
     from 2016-09-25 09:16:55
       to 2016-10-08 10:53:30
     
          bits_hex: 0x180440c4  | 0x18  0x440c4
    reg_difficulty:  278724
         trt_ratio: 1.02130291005  trt_ratio*BITS: 0x457f5
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 215     blk_ht: 435456
               seconds: 1235368
     from 2016-10-08 10:53:30
       to 2016-10-22 18:02:58
     
          bits_hex: 0x180455d2  | 0x18  0x455d2
    reg_difficulty:  284114
         trt_ratio: 0.99623015873  trt_ratio*BITS: 0x451a2
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 216     blk_ht: 437472
               seconds: 1205040
     from 2016-10-22 18:02:58
       to 2016-11-05 16:46:58
     
          bits_hex: 0x18045174  | 0x18  0x45174
    reg_difficulty:  282996
         trt_ratio: 0.903932705026  trt_ratio*BITS: 0x3e741
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 217     blk_ht: 439488
               seconds: 1093397
     from 2016-11-05 16:46:58
       to 2016-11-18 08:30:15
     
          bits_hex: 0x1803e6d4  | 0x18  0x3e6d4
    reg_difficulty:  255700
         trt_ratio: 0.982945601852  trt_ratio*BITS: 0x3d5cb
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 218     blk_ht: 441504
               seconds: 1188971
     from 2016-11-18 08:30:15
       to 2016-12-02 02:46:26
     
          bits_hex: 0x1803d589  | 0x18  0x3d589
    reg_difficulty:  251273
         trt_ratio: 0.924708167989  trt_ratio*BITS: 0x38ba2
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 219     blk_ht: 443520
               seconds: 1118527
     from 2016-12-02 02:46:26
       to 2016-12-15 01:28:33
     
          bits_hex: 0x18038b85  | 0x18  0x38b85
    reg_difficulty:  232325
         trt_ratio: 0.976803902116  trt_ratio*BITS: 0x37677
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 220     blk_ht: 445536
               seconds: 1181542
     from 2016-12-15 01:28:33
       to 2016-12-28 17:40:55
     
          bits_hex: 0x180375ff  | 0x18  0x375ff
    reg_difficulty:  226815
         trt_ratio: 0.943449900794  trt_ratio*BITS: 0x343e4
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 221     blk_ht: 447552
               seconds: 1141197
     from 2016-12-28 17:40:55
       to 2017-01-10 22:40:52
     
          bits_hex: 0x18034379  | 0x18  0x34379
    reg_difficulty:  213881
         trt_ratio: 0.857738095238  trt_ratio*BITS: 0x2cc9d
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 222     blk_ht: 449568
               seconds: 1037520
     from 2017-01-10 22:40:52
       to 2017-01-22 22:52:52
     
          bits_hex: 0x1802cc47  | 0x18  0x2cc47
    reg_difficulty:  183367
         trt_ratio: 0.930850694444  trt_ratio*BITS: 0x29abf
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 223     blk_ht: 451584
               seconds: 1125957
     from 2017-01-22 22:52:52
       to 2017-02-04 23:38:49
     
          bits_hex: 0x18029ab9  | 0x18  0x29ab9
    reg_difficulty:  170681
         trt_ratio: 0.958314318783  trt_ratio*BITS: 0x27eee
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 224     blk_ht: 453600
               seconds: 1159177
     from 2017-02-04 23:38:49
       to 2017-02-18 09:38:26
     
          bits_hex: 0x18027e93  | 0x18  0x27e93
    reg_difficulty:  163475
         trt_ratio: 0.956663359788  trt_ratio*BITS: 0x262e6
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 225     blk_ht: 455616
               seconds: 1157180
     from 2017-02-18 09:38:26
       to 2017-03-03 19:04:46
     
          bits_hex: 0x180262df  | 0x18  0x262df
    reg_difficulty:  156383
         trt_ratio: 0.968823578042  trt_ratio*BITS: 0x24fd3
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 226     blk_ht: 457632
               seconds: 1171889
     from 2017-03-03 19:04:46
       to 2017-03-17 08:36:15
     
          bits_hex: 0x18024fb1  | 0x18  0x24fb1
    reg_difficulty:  151473
         trt_ratio: 0.952523974868  trt_ratio*BITS: 0x23399
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 227     blk_ht: 459648
               seconds: 1152173
     from 2017-03-17 08:36:15
       to 2017-03-30 16:39:08
     
          bits_hex: 0x1802335a  | 0x18  0x2335a
    reg_difficulty:  144218
         trt_ratio: 0.959360119048  trt_ratio*BITS: 0x21c74
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 228     blk_ht: 461664
               seconds: 1160442
     from 2017-03-30 16:39:08
       to 2017-04-13 02:59:50
     
          bits_hex: 0x18021c73  | 0x18  0x21c73
    reg_difficulty:  138355
         trt_ratio: 0.998024966931  trt_ratio*BITS: 0x21b61
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 229     blk_ht: 463680
               seconds: 1207211
     from 2017-04-13 02:59:50
       to 2017-04-27 02:20:01
     
          bits_hex: 0x18021b3e  | 0x18  0x21b3e
    reg_difficulty:  138046
         trt_ratio: 0.93257853836  trt_ratio*BITS: 0x1f6e2
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 230     blk_ht: 465696
               seconds: 1128047
     from 2017-04-27 02:20:01
       to 2017-05-10 03:40:48
     
          bits_hex: 0x1801f6a7  | 0x18  0x1f6a7
    reg_difficulty:  128679
         trt_ratio: 0.939933862434  trt_ratio*BITS: 0x1d875
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 231     blk_ht: 467712
               seconds: 1136944
     from 2017-05-10 03:40:48
       to 2017-05-23 07:29:52
     
          bits_hex: 0x1801d854  | 0x18  0x1d854
    reg_difficulty:  120916
         trt_ratio: 0.878236607143  trt_ratio*BITS: 0x19ed0
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 232     blk_ht: 469728
               seconds: 1062315
     from 2017-05-23 07:29:52
       to 2017-06-04 14:35:07
     
          bits_hex: 0x18019eaf  | 0x18  0x19eaf
    reg_difficulty:  106159
         trt_ratio: 0.954551917989  trt_ratio*BITS: 0x18bd6
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 233     blk_ht: 471744
               seconds: 1154626
     from 2017-06-04 14:35:07
       to 2017-06-17 23:18:53
     
          bits_hex: 0x18018b7e  | 0x18  0x18b7e
    reg_difficulty:  101246
         trt_ratio: 1.00438492063  trt_ratio*BITS: 0x18d39
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 234     blk_ht: 473760
               seconds: 1214904
     from 2017-06-17 23:18:53
       to 2017-07-02 00:47:17
     
          bits_hex: 0x18018d30  | 0x18  0x18d30
    reg_difficulty:  101680
         trt_ratio: 0.880873842593  trt_ratio*BITS: 0x15ddf
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 235     blk_ht: 475776
               seconds: 1065505
     from 2017-07-02 00:47:17
       to 2017-07-14 08:45:42
     
          bits_hex: 0x18015ddc  | 0x18  0x15ddc
    reg_difficulty:  89564
         trt_ratio: 0.935426587302  trt_ratio*BITS: 0x14744
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 236     blk_ht: 477792
               seconds: 1131492
     from 2017-07-14 08:45:42
       to 2017-07-27 11:03:54
     
          bits_hex: 0x18014735  | 0x18  0x14735
    reg_difficulty:  83765
         trt_ratio: 0.933181216931  trt_ratio*BITS: 0x13157
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 237     blk_ht: 479808
               seconds: 1128776
     from 2017-07-27 11:03:54
       to 2017-08-09 12:36:50
     
          bits_hex: 0x180130e0  | 0x18  0x130e0
    reg_difficulty:  78048
         trt_ratio: 1.0397213955  trt_ratio*BITS: 0x13cfc
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 238     blk_ht: 481824
               seconds: 1257647
     from 2017-08-09 12:36:50
       to 2017-08-24 01:57:37
     
          bits_hex: 0x18013ce9  | 0x18  0x13ce9
    reg_difficulty:  81129
         trt_ratio: 0.962581018519  trt_ratio*BITS: 0x1310d
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 239     blk_ht: 483840
               seconds: 1164338
     from 2017-08-24 01:57:37
       to 2017-09-06 13:23:15
     
          bits_hex: 0x1801310b  | 0x18  0x1310b
    reg_difficulty:  78091
         trt_ratio: 0.836707175926  trt_ratio*BITS: 0xff3b
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 240     blk_ht: 485856
               seconds: 1012081
     from 2017-09-06 13:23:15
       to 2017-09-18 06:31:16
     
          bits_hex: 0x1800ff18  | 0x18  0xff18
    reg_difficulty:  65304
         trt_ratio: 0.981969246032  trt_ratio*BITS: 0xfa7e
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 241     blk_ht: 487872
               seconds: 1187790
     from 2017-09-18 06:31:16
       to 2017-10-02 00:27:46
     
          bits_hex: 0x1800fa73  | 0x18  0xfa73
    reg_difficulty:  64115
         trt_ratio: 0.939348544974  trt_ratio*BITS: 0xeb42
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 242     blk_ht: 489888
               seconds: 1136236
     from 2017-10-02 00:27:46
       to 2017-10-15 04:05:02
     
          bits_hex: 0x1800eb30  | 0x18  0xeb30
    reg_difficulty:  60208
         trt_ratio: 0.823792989418  trt_ratio*BITS: 0xc1be
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 243     blk_ht: 491904
               seconds: 996460
     from 2017-10-15 04:05:02
       to 2017-10-26 16:52:42
     
          bits_hex: 0x1800c1bd  | 0x18  0xc1bd
    reg_difficulty:  49597
         trt_ratio: 1.06652529762  trt_ratio*BITS: 0xcea0
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 244     blk_ht: 493920
               seconds: 1290069
     from 2017-10-26 16:52:42
       to 2017-11-10 15:13:51
     
          bits_hex: 0x1800ce4b  | 0x18  0xce4b
    reg_difficulty:  52811
         trt_ratio: 1.0138599537  trt_ratio*BITS: 0xd126
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 245     blk_ht: 495936
               seconds: 1226365
     from 2017-11-10 15:13:51
       to 2017-11-24 19:53:16
     
          bits_hex: 0x1800d0f6  | 0x18  0xd0f6
    reg_difficulty:  53494
         trt_ratio: 0.846730324074  trt_ratio*BITS: 0xb0ee
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 246     blk_ht: 497952
               seconds: 1024205
     from 2017-11-24 19:53:16
       to 2017-12-06 16:23:21
     
          bits_hex: 0x1800b0ed  | 0x18  0xb0ed
    reg_difficulty:  45293
         trt_ratio: 0.849800760582  trt_ratio*BITS: 0x965a
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 247     blk_ht: 499968
               seconds: 1027919
     from 2017-12-06 16:23:21
       to 2017-12-18 13:55:20
     
          bits_hex: 0x18009645  | 0x18  0x9645
    reg_difficulty:  38469
         trt_ratio: 0.970279431217  trt_ratio*BITS: 0x91cd
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 248     blk_ht: 501984
               seconds: 1173650
     from 2017-12-18 13:55:20
       to 2018-01-01 03:56:10
     
          bits_hex: 0x180091c1  | 0x18  0x91c1
    reg_difficulty:  37313
         trt_ratio: 0.866884920635  trt_ratio*BITS: 0x7e5a
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 249     blk_ht: 504000
               seconds: 1048584
     from 2018-01-01 03:56:10
       to 2018-01-13 07:12:34
     
          bits_hex: 0x177e578c  | 0x17  0x7e578c
    reg_difficulty:  8279948
         trt_ratio: 0.855940806878  trt_ratio*BITS: 0x6c2429
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 250     blk_ht: 506016
               seconds: 1035346
     from 2018-01-13 07:12:34
       to 2018-01-25 06:48:20
     
          bits_hex: 0x176c2146  | 0x17  0x6c2146
    reg_difficulty:  7086406
         trt_ratio: 0.905743220899  trt_ratio*BITS: 0x61f020
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 251     blk_ht: 508032
               seconds: 1095587
     from 2018-01-25 06:48:20
       to 2018-02-06 23:08:07
     
          bits_hex: 0x1761e9f8  | 0x17  0x61e9f8
    reg_difficulty:  6416888
         trt_ratio: 0.95599537037  trt_ratio*BITS: 0x5d9af3
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 252     blk_ht: 510048
               seconds: 1156372
     from 2018-02-06 23:08:07
       to 2018-02-20 08:20:59
     
          bits_hex: 0x175d97dc  | 0x17  0x5d97dc
    reg_difficulty:  6133724
         trt_ratio: 0.916682374339  trt_ratio*BITS: 0x55cb94
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 253     blk_ht: 512064
               seconds: 1108819
     from 2018-02-20 08:20:59
       to 2018-03-05 04:21:18
     
          bits_hex: 0x175589a3  | 0x17  0x5589a3
    reg_difficulty:  5605795
         trt_ratio: 0.950347222222  trt_ratio*BITS: 0x514a5b
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 254     blk_ht: 514080
               seconds: 1149540
     from 2018-03-05 04:21:18
       to 2018-03-18 11:40:18
     
          bits_hex: 0x17514a49  | 0x17  0x514a49
    reg_difficulty:  5327433
         trt_ratio: 0.986391369048  trt_ratio*BITS: 0x502f15
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 255     blk_ht: 516096
               seconds: 1193139
     from 2018-03-18 11:40:18
       to 2018-04-01 07:05:57
     
          bits_hex: 0x17502ab7  | 0x17  0x502ab7
    reg_difficulty:  5253815
         trt_ratio: 0.914827215608  trt_ratio*BITS: 0x4956bc
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 256     blk_ht: 518112
               seconds: 1106575
     from 2018-04-01 07:05:57
       to 2018-04-14 02:28:52
     
          bits_hex: 0x1749500d  | 0x17  0x49500d
    reg_difficulty:  4804621
         trt_ratio: 0.955126488095  trt_ratio*BITS: 0x4605dc
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 257     blk_ht: 520128
               seconds: 1155321
     from 2018-04-14 02:28:52
       to 2018-04-27 11:24:13
     
          bits_hex: 0x1745fb53  | 0x17  0x45fb53
    reg_difficulty:  4586323
         trt_ratio: 0.971727843915  trt_ratio*BITS: 0x4400d1
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 258     blk_ht: 522144
               seconds: 1175402
     from 2018-04-27 11:24:13
       to 2018-05-11 01:54:15
     
          bits_hex: 0x1743eca9  | 0x17  0x43eca9
    reg_difficulty:  4451497
         trt_ratio: 0.962630621693  trt_ratio*BITS: 0x4162db
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 259     blk_ht: 524160
               seconds: 1164398
     from 2018-05-11 01:54:15
       to 2018-05-24 13:20:53
     
          bits_hex: 0x17415a49  | 0x17  0x415a49
    reg_difficulty:  4282953
         trt_ratio: 0.871885747354  trt_ratio*BITS: 0x38fae5
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 260     blk_ht: 526176
               seconds: 1054633
     from 2018-05-24 13:20:53
       to 2018-06-05 18:18:06
     
          bits_hex: 0x1738f841  | 0x17  0x38f841
    reg_difficulty:  3733569
         trt_ratio: 0.973345734127  trt_ratio*BITS: 0x377385
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 261     blk_ht: 528192
               seconds: 1177359
     from 2018-06-05 18:18:06
       to 2018-06-19 09:20:45
     
          bits_hex: 0x17376f56  | 0x17  0x376f56
    reg_difficulty:  3632982
         trt_ratio: 0.947103174603  trt_ratio*BITS: 0x3480a8
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 262     blk_ht: 530208
               seconds: 1145616
     from 2018-06-19 09:20:45
       to 2018-07-02 15:34:21
     
          bits_hex: 0x17347a28  | 0x17  0x347a28
    reg_difficulty:  3439144
         trt_ratio: 1.03653108466  trt_ratio*BITS: 0x3664eb
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 263     blk_ht: 532224
               seconds: 1253788
     from 2018-07-02 15:34:21
       to 2018-07-17 03:50:49
     
          bits_hex: 0x17365a17  | 0x17  0x365a17
    reg_difficulty:  3562007
         trt_ratio: 0.870471230159  trt_ratio*BITS: 0x2f4fd0
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 264     blk_ht: 534240
               seconds: 1052922
     from 2018-07-17 03:50:49
       to 2018-07-29 08:19:31
     
          bits_hex: 0x172f4f7b  | 0x17  0x2f4f7b
    reg_difficulty:  3100539
         trt_ratio: 0.932612433862  trt_ratio*BITS: 0x2c1f51
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 265     blk_ht: 536256
               seconds: 1128088
     from 2018-07-29 08:19:31
       to 2018-08-11 09:40:59
     
          bits_hex: 0x172c0da7  | 0x17  0x2c0da7
    reg_difficulty:  2887079
         trt_ratio: 0.949877645503  trt_ratio*BITS: 0x29d863
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 266     blk_ht: 538272
               seconds: 1148972
     from 2018-08-11 09:40:59
       to 2018-08-24 16:50:31
     
          bits_hex: 0x1729d72d  | 0x17  0x29d72d
    reg_difficulty:  2742061
         trt_ratio: 0.95953042328  trt_ratio*BITS: 0x2825b2
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 267     blk_ht: 540288
               seconds: 1160648
     from 2018-08-24 16:50:31
       to 2018-09-07 03:14:39
     
          bits_hex: 0x172819a1  | 0x17  0x2819a1
    reg_difficulty:  2628001
         trt_ratio: 0.982192460317  trt_ratio*BITS: 0x2762d2
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 268     blk_ht: 542304
               seconds: 1188060
     from 2018-09-07 03:14:39
       to 2018-09-20 21:15:39
     
          bits_hex: 0x17275a1f  | 0x17  0x275a1f
    reg_difficulty:  2578975
         trt_ratio: 0.960005787037  trt_ratio*BITS: 0x25c736
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 269     blk_ht: 544320
               seconds: 1161223
     from 2018-09-20 21:15:39
       to 2018-10-04 07:49:22
     
          bits_hex: 0x1725c191  | 0x17  0x25c191
    reg_difficulty:  2474385
         trt_ratio: 1.03811590608  trt_ratio*BITS: 0x2731fa
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 270     blk_ht: 546336
               seconds: 1255705
     from 2018-10-04 07:49:22
       to 2018-10-18 20:37:47
     
          bits_hex: 0x17272fbd  | 0x17  0x272fbd
    reg_difficulty:  2568125
         trt_ratio: 1.00081762566  trt_ratio*BITS: 0x2737f0
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 271     blk_ht: 548352
               seconds: 1210589
     from 2018-10-18 20:37:47
       to 2018-11-01 20:54:16
     
          bits_hex: 0x17272d92  | 0x17  0x272d92
    reg_difficulty:  2567570
         trt_ratio: 1.08021494709  trt_ratio*BITS: 0x2a5217
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 272     blk_ht: 550368
               seconds: 1306628
     from 2018-11-01 20:54:16
       to 2018-11-16 23:51:24
     
          bits_hex: 0x172a4e2f  | 0x17  0x2a4e2f
    reg_difficulty:  2772527
         trt_ratio: 1.17897156085  trt_ratio*BITS: 0x31e07a
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 273     blk_ht: 552384
               seconds: 1426084
     from 2018-11-16 23:51:24
       to 2018-12-03 11:59:28
     
          bits_hex: 0x1731d97c  | 0x17  0x31d97c
    reg_difficulty:  3266940
         trt_ratio: 1.10581762566  trt_ratio*BITS: 0x371fdf
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 274     blk_ht: 554400
               seconds: 1337597
     from 2018-12-03 11:59:28
       to 2018-12-18 23:32:45
     
          bits_hex: 0x17371ef4  | 0x17  0x371ef4
    reg_difficulty:  3612404
         trt_ratio: 0.910089285714  trt_ratio*BITS: 0x322a3a
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 275     blk_ht: 556416
               seconds: 1100844
     from 2018-12-18 23:32:45
       to 2018-12-31 17:20:09
     
          bits_hex: 0x173218a5  | 0x17  0x3218a5
    reg_difficulty:  3283109
         trt_ratio: 0.955344742063  trt_ratio*BITS: 0x2fdbf4
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 276     blk_ht: 558432
               seconds: 1155585
     from 2018-12-31 17:20:09
       to 2019-01-14 02:19:54
     
          bits_hex: 0x172fd633  | 0x17  0x2fd633
    reg_difficulty:  3135027
         trt_ratio: 1.0126645172  trt_ratio*BITS: 0x30714a
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 277     blk_ht: 560448
               seconds: 1224919
     from 2019-01-14 02:19:54
       to 2019-01-28 06:35:13
     
          bits_hex: 0x17306835  | 0x17  0x306835
    reg_difficulty:  3172405
         trt_ratio: 0.959546957672  trt_ratio*BITS: 0x2e72e7
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 278     blk_ht: 562464
               seconds: 1160668
     from 2019-01-28 06:35:13
       to 2019-02-10 16:59:41
     
          bits_hex: 0x172e6f88  | 0x17  0x2e6f88
    reg_difficulty:  3043208
         trt_ratio: 0.998724371693  trt_ratio*BITS: 0x2e605d
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 279     blk_ht: 564480
               seconds: 1208057
     from 2019-02-10 16:59:41
       to 2019-02-24 16:33:58
     
          bits_hex: 0x172e5b50  | 0x17  0x2e5b50
    reg_difficulty:  3038032
         trt_ratio: 1.00055059524  trt_ratio*BITS: 0x2e61d8
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 280     blk_ht: 566496
               seconds: 1210266
     from 2019-02-24 16:33:58
       to 2019-03-10 16:45:04
     
          bits_hex: 0x172e6117  | 0x17  0x2e6117
    reg_difficulty:  3039511
         trt_ratio: 0.951379794974  trt_ratio*BITS: 0x2c1fd1
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 281     blk_ht: 568512
               seconds: 1150789
     from 2019-03-10 16:45:04
       to 2019-03-24 00:24:53
     
          bits_hex: 0x172c1f6c  | 0x17  0x2c1f6c
    reg_difficulty:  2891628
         trt_ratio: 0.997958002646  trt_ratio*BITS: 0x2c085b
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 282     blk_ht: 570528
               seconds: 1207130
     from 2019-03-24 00:24:53
       to 2019-04-06 23:43:43
     
          bits_hex: 0x172c071d  | 0x17  0x2c071d
    reg_difficulty:  2885405
         trt_ratio: 1.00648561508  trt_ratio*BITS: 0x2c5036
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 283     blk_ht: 572544
               seconds: 1217445
     from 2019-04-06 23:43:43
       to 2019-04-21 01:54:28
     
          bits_hex: 0x172c4e11  | 0x17  0x2c4e11
    reg_difficulty:  2903569
         trt_ratio: 0.94830109127  trt_ratio*BITS: 0x2a03b1
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 284     blk_ht: 574560
               seconds: 1147065
     from 2019-04-21 01:54:28
       to 2019-05-04 08:32:13
     
          bits_hex: 0x1729ff38  | 0x17  0x29ff38
    reg_difficulty:  2752312
         trt_ratio: 0.999969411376  trt_ratio*BITS: 0x29fee3
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 285     blk_ht: 576576
               seconds: 1209563
     from 2019-05-04 08:32:13
       to 2019-05-18 08:31:36
     
          bits_hex: 0x1729fb45  | 0x17  0x29fb45
    reg_difficulty:  2751301
         trt_ratio: 0.899378306878  trt_ratio*BITS: 0x25c1dc
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 286     blk_ht: 578592
               seconds: 1087888
     from 2019-05-18 08:31:36
       to 2019-05-30 22:43:04
     
          bits_hex: 0x1725bb76  | 0x17  0x25bb76
    reg_difficulty:  2472822
         trt_ratio: 1.00698247354  trt_ratio*BITS: 0x25fee8
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 287     blk_ht: 580608
               seconds: 1218046
     from 2019-05-30 22:43:04
       to 2019-06-14 01:03:50
     
          bits_hex: 0x1725fd03  | 0x17  0x25fd03
    reg_difficulty:  2489603
         trt_ratio: 0.934308862434  trt_ratio*BITS: 0x237e2a
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 288     blk_ht: 582624
               seconds: 1130140
     from 2019-06-14 01:03:50
       to 2019-06-27 02:59:30
     
          bits_hex: 0x1723792c  | 0x17  0x23792c
    reg_difficulty:  2324780
         trt_ratio: 0.875907738095  trt_ratio*BITS: 0x1f1244
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 289     blk_ht: 584640
               seconds: 1059498
     from 2019-06-27 02:59:30
       to 2019-07-09 09:17:48
     
          bits_hex: 0x171f0d9b  | 0x17  0x1f0d9b
    reg_difficulty:  2035099
         trt_ratio: 1.00617476852  trt_ratio*BITS: 0x1f3eb1
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 290     blk_ht: 586656
               seconds: 1217069
     from 2019-07-09 09:17:48
       to 2019-07-23 11:22:17
     
          bits_hex: 0x171f3a08  | 0x17  0x1f3a08
    reg_difficulty:  2046472
         trt_ratio: 0.903266369048  trt_ratio*BITS: 0x1c34bd
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 291     blk_ht: 588672
               seconds: 1092591
     from 2019-07-23 11:22:17
       to 2019-08-05 02:52:08
     
          bits_hex: 0x171c3039  | 0x17  0x1c3039
    reg_difficulty:  1847353
         trt_ratio: 0.982022156085  trt_ratio*BITS: 0x1bae7d
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 292     blk_ht: 590688
               seconds: 1187854
     from 2019-08-05 02:52:08
       to 2019-08-18 20:49:42
     
          bits_hex: 0x171ba3d1  | 0x17  0x1ba3d1
    reg_difficulty:  1811409
         trt_ratio: 0.945700231481  trt_ratio*BITS: 0x1a2399
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 293     blk_ht: 592704
               seconds: 1143919
     from 2019-08-18 20:49:42
       to 2019-09-01 02:35:01
     
          bits_hex: 0x171a213e  | 0x17  0x1a213e
    reg_difficulty:  1712446
         trt_ratio: 0.906324404762  trt_ratio*BITS: 0x17ae9f
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 294     blk_ht: 594720
               seconds: 1096290
     from 2019-09-01 02:35:01
       to 2019-09-13 19:06:31
     
          bits_hex: 0x1717abf5  | 0x17  0x17abf5
    reg_difficulty:  1551349
         trt_ratio: 0.932878637566  trt_ratio*BITS: 0x161534
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 295     blk_ht: 596736
               seconds: 1128410
     from 2019-09-13 19:06:31
       to 2019-09-26 20:33:21
     
          bits_hex: 0x17160f24  | 0x17  0x160f24
    reg_difficulty:  1445668
         trt_ratio: 0.980930886243  trt_ratio*BITS: 0x15a374
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 296     blk_ht: 598752
               seconds: 1186534
     from 2019-09-26 20:33:21
       to 2019-10-10 14:08:55
     
          bits_hex: 0x1715a35c  | 0x17  0x15a35c
    reg_difficulty:  1418076
         trt_ratio: 0.951089616402  trt_ratio*BITS: 0x14946d
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 297     blk_ht: 600768
               seconds: 1150438
     from 2019-10-10 14:08:55
       to 2019-10-23 21:42:53
     
          bits_hex: 0x17148edf  | 0x17  0x148edf
    reg_difficulty:  1347295
         trt_ratio: 1.07677166005  trt_ratio*BITS: 0x1622e9
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 298     blk_ht: 602784
               seconds: 1302463
     from 2019-10-23 21:42:53
       to 2019-11-07 23:30:36
     
          bits_hex: 0x171620d1  | 0x17  0x1620d1
    reg_difficulty:  1450193
         trt_ratio: 0.981064814815  trt_ratio*BITS: 0x15b58d
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 299     blk_ht: 604800
               seconds: 1186696
     from 2019-11-07 23:30:36
       to 2019-11-21 17:08:52
     
          bits_hex: 0x1715b23e  | 0x17  0x15b23e
    reg_difficulty:  1421886
         trt_ratio: 1.00860863095  trt_ratio*BITS: 0x15e20e
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 300     blk_ht: 606816
               seconds: 1220013
     from 2019-11-21 17:08:52
       to 2019-12-05 20:02:25
     
          bits_hex: 0x1715dbd2  | 0x17  0x15dbd2
    reg_difficulty:  1432530
         trt_ratio: 0.994771825397  trt_ratio*BITS: 0x15be90
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 301     blk_ht: 608832
               seconds: 1203276
     from 2019-12-05 20:02:25
       to 2019-12-19 18:17:01
     
          bits_hex: 0x1715bcd0  | 0x17  0x15bcd0
    reg_difficulty:  1424592
         trt_ratio: 0.939356812169  trt_ratio*BITS: 0x146b58
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 302     blk_ht: 610848
               seconds: 1136246
     from 2019-12-19 18:17:01
       to 2020-01-01 21:54:27
     
          bits_hex: 0x171465f2  | 0x17  0x1465f2
    reg_difficulty:  1336818
         trt_ratio: 0.933936838624  trt_ratio*BITS: 0x130cf7
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 303     blk_ht: 612864
               seconds: 1129690
     from 2020-01-01 21:54:27
       to 2020-01-14 23:42:37
     
          bits_hex: 0x17130c78  | 0x17  0x130c78
    reg_difficulty:  1248376
         trt_ratio: 0.955428240741  trt_ratio*BITS: 0x12331d
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 304     blk_ht: 614880
               seconds: 1155686
     from 2020-01-14 23:42:37
       to 2020-01-28 08:44:03
     
          bits_hex: 0x171232ff  | 0x17  0x1232ff
    reg_difficulty:  1192703
         trt_ratio: 0.995354662698  trt_ratio*BITS: 0x121d5a
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 305     blk_ht: 616896
               seconds: 1203981
     from 2020-01-28 08:44:03
       to 2020-02-11 07:10:24
     
          bits_hex: 0x17121ad4  | 0x17  0x121ad4
    reg_difficulty:  1186516
         trt_ratio: 1.00388392857  trt_ratio*BITS: 0x122cd4
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 306     blk_ht: 618912
               seconds: 1214298
     from 2020-02-11 07:10:24
       to 2020-02-25 08:28:42
     
          bits_hex: 0x17122cbc  | 0x17  0x122cbc
    reg_difficulty:  1191100
         trt_ratio: 0.936338458995  trt_ratio*BITS: 0x110488
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 307     blk_ht: 620928
               seconds: 1132595
     from 2020-02-25 08:28:42
       to 2020-03-09 11:05:17
     
          bits_hex: 0x17110119  | 0x17  0x110119
    reg_difficulty:  1114393
         trt_ratio: 1.1898057209  trt_ratio*BITS: 0x143b57
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 308     blk_ht: 622944
               seconds: 1439189
     from 2020-03-09 11:05:17
       to 2020-03-26 02:51:46
     
          bits_hex: 0x17143b41  | 0x17  0x143b41
    reg_difficulty:  1325889
         trt_ratio: 0.946544312169  trt_ratio*BITS: 0x132664
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 309     blk_ht: 624960
               seconds: 1144940
     from 2020-03-26 02:51:46
       to 2020-04-08 08:54:06
     
          bits_hex: 0x171320bc  | 0x17  0x1320bc
    reg_difficulty:  1253564
         trt_ratio: 0.923179563492  trt_ratio*BITS: 0x11a890
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 310     blk_ht: 626976
               seconds: 1116678
     from 2020-04-08 08:54:06
       to 2020-04-21 07:05:24
     
          bits_hex: 0x1711a333  | 0x17  0x11a333
    reg_difficulty:  1155891
         trt_ratio: 0.991068948413  trt_ratio*BITS: 0x117adf
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 311     blk_ht: 628992
               seconds: 1198797
     from 2020-04-21 07:05:24
       to 2020-05-05 04:05:21
     
          bits_hex: 0x17117a39  | 0x17  0x117a39
    reg_difficulty:  1145401
         trt_ratio: 1.06555472884  trt_ratio*BITS: 0x129f87
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 312     blk_ht: 631008
               seconds: 1288895
     from 2020-05-05 04:05:21
       to 2020-05-20 02:06:56
     
          bits_hex: 0x171297f6  | 0x17  0x1297f6
    reg_difficulty:  1218550
         trt_ratio: 1.1023776455  trt_ratio*BITS: 0x147f46
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 313     blk_ht: 633024
               seconds: 1333436
     from 2020-05-20 02:06:56
       to 2020-06-04 12:30:52
     
          bits_hex: 0x17147f35  | 0x17  0x147f35
    reg_difficulty:  1343285
         trt_ratio: 0.870052083333  trt_ratio*BITS: 0x11d557
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 314     blk_ht: 635040
               seconds: 1052415
     from 2020-06-04 12:30:52
       to 2020-06-16 16:51:07
     
          bits_hex: 0x1711d4f2  | 0x17  0x11d4f2
    reg_difficulty:  1168626
         trt_ratio: 1.00137400794  trt_ratio*BITS: 0x11db37
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 315     blk_ht: 637056
               seconds: 1211262
     from 2020-06-16 16:51:07
       to 2020-06-30 17:18:49
     
          bits_hex: 0x1711d519  | 0x17  0x11d519
    reg_difficulty:  1168665
         trt_ratio: 0.912310681217  trt_ratio*BITS: 0x1044c9
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 316     blk_ht: 639072
               seconds: 1103531
     from 2020-06-30 17:18:49
       to 2020-07-13 11:51:00
     
          bits_hex: 0x17103a15  | 0x17  0x103a15
    reg_difficulty:  1063445
         trt_ratio: 1.02984126984  trt_ratio*BITS: 0x10b60b
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 317     blk_ht: 641088
               seconds: 1245696
     from 2020-07-13 11:51:00
       to 2020-07-27 21:52:36
     
          bits_hex: 0x1710b4f8  | 0x17  0x10b4f8
    reg_difficulty:  1094904
         trt_ratio: 0.994430390212  trt_ratio*BITS: 0x109d25
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 318     blk_ht: 643104
               seconds: 1202863
     from 2020-07-27 21:52:36
       to 2020-08-10 20:00:19
     
          bits_hex: 0x17109bac  | 0x17  0x109bac
    reg_difficulty:  1088428
         trt_ratio: 0.965972222222  trt_ratio*BITS: 0x100aff
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 319     blk_ht: 645120
               seconds: 1168440
     from 2020-08-10 20:00:19
       to 2020-08-24 08:34:19
     
          bits_hex: 0x171007ea  | 0x17  0x1007ea
    reg_difficulty:  1050602
         trt_ratio: 1.01261656746  trt_ratio*BITS: 0x103bb0
    
     Difficulty -> DECREASE
    
    -----------------------------
    Reg# 320     blk_ht: 647136
               seconds: 1224861
     from 2020-08-24 08:34:19
       to 2020-09-07 12:48:40
     
          bits_hex: 0x17103a12  | 0x17  0x103a12
    reg_difficulty:  1063442
         trt_ratio: 0.899150958995  trt_ratio*BITS: 0xe9722
    
     Difficulty -> INCREASE
    
    -----------------------------
    Reg# 321     blk_ht: 649152
               seconds: 1087613
     from 2020-09-07 12:48:40
       to 2020-09-20 02:55:33
     
          bits_hex: 0x170e92aa  | 0x17  0xe92aa



```python
##-- SCRATCH vv -------------------------------------------------
```


```python
## EXTRACT only blks when difficulty changes

cnt = 1
cur_bits = df_datachain['bits_hex'][cnt]

seq_bitsL  = [ ]
seq_bitsL.append(cur_bits)
seq_indexL = [ ]
seq_indexL.append(1)
ind_seq = 0

while True:
    if seq_bitsL[-1] != df_datachain['bits_hex'][cnt] :
        seq_bitsL.append( df_datachain['bits_hex'][cnt])
        # pesky convert from list index to blk_ht +1
        seq_indexL.append( cnt+1 )
    
    cnt = cnt + 1
    if cnt == len(df_datachain):
        break

print " seq_bitsL len: " + str(len(seq_bitsL))
print " seq_indexL len: " + str(len(seq_indexL))
```

     seq_bitsL len: 308
     seq_indexL len: 308



```python
544.0 / 600.0
0xd86a * 0.906
```




    50194.212




```python
hex(50194)
```




    '0xc412'




```python
cnt = 0
while True:
    print str(seq_indexL[cnt])+','+str(seq_bitsL[cnt])
    cnt = cnt+1
    if cnt == len(seq_indexL):  break
```

    1,0x1d00ffff
    32256,0x1d00d86a
    34272,0x1d00c428
    36288,0x1d00be71
    38304,0x1d008cc3
    40320,0x1c654657
    42336,0x1c43b3e5
    44352,0x1c387f6f
    46368,0x1c381375
    48384,0x1c2a1115
    50400,0x1c20bca7
    52416,0x1c16546f
    54432,0x1c13ec53
    56448,0x1c159c24
    58464,0x1c0f675c
    60480,0x1c0eba64
    62496,0x1c0d3142
    64512,0x1c0ae493
    66528,0x1c05a3f4
    68544,0x1c0168fd
    70560,0x1c010c5a
    72576,0x1c00ba18
    74592,0x1c00800e
    76608,0x1b692098
    78624,0x1b5bede6
    80640,0x1b4766ed
    82656,0x1b31b2a3
    84672,0x1b2f8e9d
    86688,0x1b1e7eca
    88704,0x1b153263
    90720,0x1b0e7256
    92736,0x1b098b2a
    94752,0x1b081cd2
    96768,0x1b055953
    98784,0x1b04864c
    100800,0x1b0404cb
    102816,0x1b038dee
    104832,0x1b02fa29
    106848,0x1b028552
    108864,0x1b01cc26
    110880,0x1b012dcd
    112896,0x1b00dc31
    114912,0x1b00f339
    116928,0x1b00cbbd
    118944,0x1b00b5ac
    120960,0x1b0098fa
    122976,0x1a6a93b3
    124992,0x1a44b9f2
    127008,0x1a269421
    129024,0x1a1d932f
    131040,0x1a132185
    133056,0x1a0c2a12
    135072,0x1a0abbcf
    137088,0x1a09ec04
    139104,0x1a08e1e5
    141120,0x1a094a86
    143136,0x1a096fe3
    145152,0x1a098ea5
    147168,0x1a09ee5d
    149184,0x1a0b6d4b
    151200,0x1a0df0ca
    153216,0x1a0e119a
    155232,0x1a0f61b1
    157248,0x1a0e8668
    159264,0x1a0e76ba
    161280,0x1a0d69d7
    163296,0x1a0cd43f
    165312,0x1a0c290b
    167328,0x1a0c309c
    169344,0x1a0b350c
    171360,0x1a0b3287
    173376,0x1a0a507e
    175392,0x1a0aa1e3
    177408,0x1a0b1ef7
    179424,0x1a09ae02
    181440,0x1a0a8b5f
    183456,0x1a0a98d6
    185472,0x1a09b78a
    187488,0x1a099431
    189504,0x1a08fd2e
    191520,0x1a083cc9
    193536,0x1a07a85e
    195552,0x1a06dfbe
    197568,0x1a063a38
    199584,0x1a05db8b
    201600,0x1a057e08
    203616,0x1a0575ef
    205632,0x1a0513c5
    207648,0x1a04faeb
    209664,0x1a04e0ea
    211680,0x1a04fa62
    213696,0x1a05a16b
    215712,0x1a0529b1
    217728,0x1a05a6b1
    219744,0x1a051f3c
    221760,0x1a04985c
    223776,0x1a03d74b
    225792,0x1a0375fa
    227808,0x1a02816e
    229824,0x1a022fbe
    231840,0x1a01de94
    233856,0x1a01aa3d
    235872,0x1a017fe9
    237888,0x1a016164
    239904,0x1a011337
    241920,0x1a00de15
    243936,0x1a00c94e
    245952,0x1a00a429
    247968,0x1a008968
    249984,0x1972dbf2
    252000,0x19548732
    254016,0x19415257
    256032,0x1931679c
    258048,0x19262222
    260064,0x191cdc20
    262080,0x1916b0ca
    264096,0x19100ab6
    266112,0x190afc85
    268128,0x190867f3
    270144,0x19070bfb
    272160,0x19061242
    274176,0x1904ba6e
    276192,0x1903a30c
    278208,0x1903071f
    280224,0x19026666
    282240,0x1901f52c
    284256,0x1901a36e
    286272,0x19015f53
    288288,0x19012026
    290304,0x190102b1
    292320,0x1900db99
    294336,0x1900b3aa
    296352,0x19009d8c
    298368,0x1900896c
    300384,0x187c3053
    302400,0x18692842
    304416,0x185d859a
    306432,0x1851aba2
    308448,0x18415fd1
    310464,0x183f6be6
    312480,0x183aaea2
    314496,0x1837ba62
    316512,0x182e1c58
    318528,0x182815ee
    320544,0x1824dbe9
    322560,0x181fb893
    324576,0x181f6973
    326592,0x181e8dc0
    328608,0x181bc330
    330624,0x181b4861
    332640,0x181b7b74
    334656,0x181bdd7c
    336672,0x181b0dca
    338688,0x1819012f
    340704,0x181aa3c0
    342720,0x1818bb87
    344736,0x18178d3a
    346752,0x18172ec0
    348768,0x181788f2
    350784,0x18163c71
    352800,0x181717f0
    354816,0x181713dd
    356832,0x181686f5
    358848,0x18171a8b
    360864,0x18162043
    362880,0x1816418e
    364896,0x181586c8
    366912,0x18150815
    368928,0x1814dd04
    370944,0x181443c4
    372960,0x18134dc1
    374976,0x181287ba
    376992,0x18121472
    379008,0x18120f14
    381024,0x1811a954
    383040,0x1810b289
    385056,0x180f1e76
    387072,0x180de64f
    389088,0x180bc409
    391104,0x180a9591
    393120,0x1809b31b
    395136,0x180928f0
    397152,0x1807a114
    399168,0x1806b99f
    401184,0x1806f0a8
    403200,0x1806a4c3
    405216,0x180696f4
    407232,0x1806274b
    409248,0x18062776
    411264,0x1805a8fa
    413280,0x18058436
    415296,0x18059ba0
    417312,0x18053fd6
    419328,0x180526fd
    421344,0x18052669
    423360,0x18057228
    425376,0x18050edc
    427392,0x1804fb08
    429408,0x1804de5e
    431424,0x18048ed4
    433440,0x180440c4
    435456,0x180455d2
    437472,0x18045174
    439488,0x1803e6d4
    441504,0x1803d589
    443520,0x18038b85
    445536,0x180375ff
    447552,0x18034379
    449568,0x1802cc47
    451584,0x18029ab9
    453600,0x18027e93
    455616,0x180262df
    457632,0x18024fb1
    459648,0x1802335a
    461664,0x18021c73
    463680,0x18021b3e
    465696,0x1801f6a7
    467712,0x1801d854
    469728,0x18019eaf
    471744,0x18018b7e
    473760,0x18018d30
    475776,0x18015ddc
    477792,0x18014735
    479808,0x180130e0
    481824,0x18013ce9
    483840,0x1801310b
    485856,0x1800ff18
    487872,0x1800fa73
    489888,0x1800eb30
    491904,0x1800c1bd
    493920,0x1800ce4b
    495936,0x1800d0f6
    497952,0x1800b0ed
    499968,0x18009645
    501984,0x180091c1
    504000,0x177e578c
    506016,0x176c2146
    508032,0x1761e9f8
    510048,0x175d97dc
    512064,0x175589a3
    514080,0x17514a49
    516096,0x17502ab7
    518112,0x1749500d
    520128,0x1745fb53
    522144,0x1743eca9
    524160,0x17415a49
    526176,0x1738f841
    528192,0x17376f56
    530208,0x17347a28
    532224,0x17365a17
    534240,0x172f4f7b
    536256,0x172c0da7
    538272,0x1729d72d
    540288,0x172819a1
    542304,0x17275a1f
    544320,0x1725c191
    546336,0x17272fbd
    548352,0x17272d92
    550368,0x172a4e2f
    552384,0x1731d97c
    554400,0x17371ef4
    556416,0x173218a5
    558432,0x172fd633
    560448,0x17306835
    562464,0x172e6f88
    564480,0x172e5b50
    566496,0x172e6117
    568512,0x172c1f6c
    570528,0x172c071d
    572544,0x172c4e11
    574560,0x1729ff38
    576576,0x1729fb45
    578592,0x1725bb76
    580608,0x1725fd03
    582624,0x1723792c
    584640,0x171f0d9b
    586656,0x171f3a08
    588672,0x171c3039
    590688,0x171ba3d1
    592704,0x171a213e
    594720,0x1717abf5
    596736,0x17160f24
    598752,0x1715a35c
    600768,0x17148edf
    602784,0x171620d1
    604800,0x1715b23e
    606816,0x1715dbd2
    608832,0x1715bcd0
    610848,0x171465f2
    612864,0x17130c78
    614880,0x171232ff
    616896,0x17121ad4
    618912,0x17122cbc
    620928,0x17110119
    622944,0x17143b41
    624960,0x171320bc
    626976,0x1711a333
    628992,0x17117a39
    631008,0x171297f6
    633024,0x17147f35
    635040,0x1711d4f2
    637056,0x1711d519
    639072,0x17103a15
    641088,0x1710b4f8
    643104,0x17109bac
    645120,0x171007ea
    647136,0x17103a12
    649152,0x170e92aa



```python
54431 - 52415


print 34271 - 32255
print 36287 - 34271
print 38303 - 36287
print 40319 - 38303
print 42335 - 40319
print 44351 - 42335
print 46367 - 44351
print 48383 - 46367 
print 50399 - 48383
print 52415 - 50399
```


```python
(32255-2015) / 2016.0

```


```python
#seq_bitsL[0:12]
#seq_bitsL
seq_indexL
#len(df_datachain)
```


```python
tbase = 16

## convenience variables to mark start and end in the interval
sbeg = (dif_interval * tbase) + 1   ## skip the genesis block in all calcs
send = sbeg + dif_interval 

##  df_datachain[ sbeg:send ]

##  a full interval is too long to show at once inline, so..
##  instead, show the first four rows, 
##    then the last four plus the changed next one
df_datachain[ sbeg:(sbeg+4) ]
```


```python
df_datachain[ (send-4):(send+4) ]
```


```python
##  Show the accumulated average code with a simple example

test_d = [ 5, 5, 6, 6, 7, 8, 2]        ## avg = 39/7 = 5.571428571428571
# avgs in order:   5    5   5.33   5.5   5.8   6.16    5.571428571428571

elem = 0
cnt = 1

print 'elem'+' '+'t_avg'+'         '+'dx'+'     '+'new_avg'

for val in test_d:
  if elem == 0:
    t_avg = test_d[0]
    elem = elem+1
    cnt = cnt+1
    continue

  if elem == 1:
    t_avg = (test_d[0]+test_d[1]) / 2.0
    elem = elem+1
    cnt = cnt+1
    continue

  if cnt == len(test_d):
    new_d = test_d[elem]
    new_avg = (t_avg + ( new_d -t_avg )/ float(cnt) )
    print 'Final Avg: '+str(new_avg)
    break

  new_d = test_d[elem]
  dx = new_d - t_avg
  new_avg = (t_avg + ( new_d -t_avg )/ float(cnt) )
  print ' '+str(elem)+'    '+str(t_avg)+'           '+str(dx)+'     '+str(new_avg)
  t_avg = new_avg
  elem = elem + 1
  cnt = cnt+1

```


```python
#s1 = Series( df_datachain[''], index = df_datachain[''] )

s1 = pd.Series( df_datachain['difficulty'], index = df_datachain['blk_ht'] )

#s1[1:2016].plot()

```


```python
tbase = 15
dif_interval = 2016

sbeg = 2016 * tbase
send = sbeg + 2016 

##  plot one two-week interval; does not show the change in difficulty
## s1[ sbeg:send ].plot()

##  plot one two-week interval, plus four on either side to show change direction
s1[ (sbeg-4):(4+send) ].plot()
```

--=======================================================


```python
## take a float value and make a histogram
##  hist() is not a sequence, it is a summary, grouped by value
df_datachain['difficulty'].hist()
```


```python
##  Not Great - height_str gets sorted in alpha order
# df_datachain.set_index('height_str')
```


```python
## extract all rows where a value is less than MAX
df_smd = df_datachain[   df_datachain['difficulty'] < 80000   ]
df_smd.info()
```


```python
df_smd['difficulty' ].plot()
```


```python
## start over .. do the conversion in SQL
tSQL = 'select \
  height_str::integer as blk_height, \
  difficulty, \
  chainwork_hex, time_str::bigint as time_b, \
  get_fmt_date(time_str) as date_str, \
  totaltime_str::bigint as ttime_b \
  from data_chain order by height_str::integer'
df_dc_prep = pd.read_sql_query( tSQL,con=conn )
df_dc_prep.set_index('blk_height')
df_dc_prep.info()
```


```python
tbase_win = 0
tdif_interval = 2016

#df_dc_prep[32000:34000]

ttime = 322   # 322  max using this setup
tbase_win = ttime*tdif_interval + 1  # skip the genesis block
tend_win  = tbase_win + tdif_interval

df_dc_prep[ tbase_win:tend_win ]

```
