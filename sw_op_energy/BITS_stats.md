

```python
import os, sys

sys.version
```




    '2.7.17 (default, Jul 20 2020, 15:37:01) \n[GCC 7.5.0]'



## BITS stats    
###  12nov20  --dbb

Import `datafetch` files using a small python tool directly into PostgreSQL (~2000 recs/sec)

    btc_hist=# select count(*) from in_bits_raw ;    =>  649600   126 MB
    btc_hist=# select count(*) from in_stats_raw ;   =>  649600    91 MB
    
    ---------------------------------------------------------------
    btc_hist=# \d in_bits_raw 
                   Table "public.in_bits_raw"
     Column     | Type | Collation | Nullable | Default 
    ----------------+------+-----------+----------+---------
     height_str     | text |           |          | 
     hash_str       | text |           |          | 
     bits_str       | text |           |          | 
     difficulty_str | text |           |          | 
     chainwork_str  | text |           |          | 

    Some simple inspection:
      height_str      =>  1 to 649600  integer unique
      hash_str        =>  unique
      bits_str        =>  4byte hex, 308 unique values
             range 0x170e92aa -0x1d00d86a   plus special value 0x1d00ffff 
             
      difficulty_str  =>  text as float, 308 unique values
             range   1, 1.18289953431284 - 19314656404097
             
      chainwork_str   =>  uint256, range  0x0200020002 - 0x13d39e98be678505f99a738d

    --=====================================================================================
      btc_hist=# \d in_stats_raw 
  
     height_str  | hash_str | subsidy_str | totalfee_str |  time_str  | totaltime_str
      ----------------------------------------------------------------------------------
          1   | "00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048" | 5000000000  |
         0  | 1231469665 | 1231469665
     ....
       649600 | "0000000000000000000de228233696f11b6e1eb8b4c2703ae84ad1e3d6749787" | 625000000   | 
          85858229     | 1600853476 | 1600848688

    Some simple inspection:
      height_str      =>  1 to 649600  integer unique
      hash_str        =>  unique
      subsidy_str     =>  4 unique values  [ 5000000000, 2500000000, 1250000000, 625000000]
             range 000001 -209999       5000000000    0x12a05f200
             range 210000 -419999       2500000000     0x9502f900
             range 420000 -629999       1250000000     0x4a817c80
             range 630000 -649600        625000000     0x2540be40
             
      totalfee_str  =>  text as bigint, 437315 unique values
             range   0, 1 - 29153275103
      
      chainwork  =>  
      time_str   =>  test as bigint, 649291 unique values
             range   1231469665  (0x4966bc61)  2009-01-09 02:54:25  
                     1600853476  (0x5f6b0330)  2020-09-23 09:31:16

      totaltime_str   =>  test as bigint, 649157 unique values
             range   1231469665  (0x4966bc61)  2009-01-09 02:54:25 
                     1600848688  (0x5f6b0330)  2020-09-23 09:31:16

    -----------------------------------------------------------------------------------
    btc_hist=# create table data_chain as (
      SELECT b.height_str, b.hash_str, b.bits_str, b.difficulty_str, chainwork_str, time_str, totaltime_str
      FROM in_bits_raw as b LEFT JOIN in_stats_raw on (b.height_str = in_stats_raw.height_str) );
    SELECT 649600
    
    -- add  uintstr_to_hexstr() 
    drop table data_chain;
    create table data_chain as (
      SELECT b.height_str, fix_quoted_numbers(b.hash_str) as blkhash,  uintstr_to_hexstr(bits_str) as bits_hex, 
             b.difficulty_str::float as difficulty, uintstr_to_hexstr(chainwork_str) as chainwork_hex, 
             time_str, totaltime_str
      FROM in_bits_raw as b LEFT JOIN in_stats_raw on (b.height_str = in_stats_raw.height_str) );
      

    btc_hist=# \d data_chain 
                     Table "public.data_chain"
        Column     |       Type       | Collation | Nullable | Default 
    ---------------+------------------+-----------+----------+---------
     height_str    | text             |           |          | 
     blkhash       | text             |           |          | 
     bits_hex      | text             |           |          | 
     difficulty    | double precision |           |          | 
     chainwork_hex | text             |           |          | 
     time_str      | text             |           |          | 
     totaltime_str | text             |           |          | 


    -[ RECORD 1 ]-+-------------------------------------------------------------------
    height_str    | 1
    blkhash       | 0x00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048
    bits_hex      | 0x1d00ffff
    difficulty    | 1
    chainwork_hex | 0x200020002
    time_str      | 1231469665
    totaltime_str | 1231469665
    -[ RECORD 2 ]-+-------------------------------------------------------------------
    height_str    | 10
    blkhash       | 0x000000002c05cc2e78923c34df87fd108b22221ac6076c18f3ade378a4d915e9
    bits_hex      | 0x1d00ffff
    difficulty    | 1
    chainwork_hex | 0xb000b000b
    time_str      | 1231473952
    totaltime_str | 1231471428
    ----
    -[ RECORD 649599 ]----------------------------------------------------------------
    height_str    | 101848
    blkhash       | 0x000000000000e7ab4cd1d43e8ab503fa1e7f3400f58fc473ef1a1cfce148cf2a
    bits_hex      | 0x1b0404cb
    difficulty    | 16307.420938524
    chainwork_hex | 0x7fa69d73b137ae0
    time_str      | 1294615380
    totaltime_str | 1294613403
    -[ RECORD 649600 ]----------------------------------------------------------------
    height_str    | 101849
    blkhash       | 0x000000000002cdaf2dd6a8442dc55246695afba166b8a9ae70defff4fd1c4315
    bits_hex      | 0x1b0404cb
    difficulty    | 16307.420938524
    chainwork_hex | 0x7faa98ae689c6e0
    time_str      | 1294615594
    totaltime_str | 1294614064




```python
## next_difficulty = (previous_difficulty * 2016 * 10 minutes) / (time to mine last 2016 blocks)
```

    PostgreSQL DateTime conversion
    btc_hist=# select height_str, to_timestamp(time_str::float) from data_chain order by height_str::bigint;

     height_str |      to_timestamp      
    ------------+------------------------
     1          | 2009-01-08 18:54:25-08
     2          | 2009-01-08 18:55:44-08
     3          | 2009-01-08 19:02:53-08

     649598     | 2020-09-23 01:52:43-07
     649599     | 2020-09-23 02:09:22-07
     649600     | 2020-09-23 02:31:16-07
      (649600 rows)


## chainwork values for OP_ENERGY






### install pl/Python into PostgreSQL

    postgresql-plpython-10/bionic-pgdg,now 10.15-1.pgdg18.04+1 amd64 [installed]
    
    btc_hist=# create language plpython2u;
    CREATE LANGUAGE

### make a conversion utility

    btc_hist=# 
    CREATE FUNCTION get_fmt_date( text) returns text
     AS 
    'from datetime import datetime
    ts = args[0]
    return (datetime.utcfromtimestamp(float(ts)).strftime("%Y-%m-%d %H:%M:%S"))
    ' LANGUAGE plpython2u;
    --
    CREATE FUNCTION uintstr_to_hexstr(text) RETURNS text
     AS
    'tStr = args[0]
    tStr = tStr.strip( "\"")
    tStr = tStr.lstrip("0")
    return "0x"+tStr
    ' LANGUAGE plpython2u;
    --
    CREATE FUNCTION hexstr_to_bigint(text) RETURNS bigint
     AS
    ' return int( args[0], base=16)'
    LANGUAGE plpython2u;
    --
    CREATE FUNCTION fix_quoted_numbers(text) RETURNS text
     AS
    'tStr = args[0]
    return "0x"+tStr.strip("\"")'
    LANGUAGE plpython2u;
    --
    CREATE FUNCTION bigint_to_hexstr(bigint) RETURNS text
     AS
    'tBInt = args[0]
    return  hex(tBInt)
    ' LANGUAGE plpython2u;
    --
    CREATE FUNCTION int_to_hexstr(integer) RETURNS text
     AS
    'tInt = args[0]
    return hex(tInt)
    ' LANGUAGE plpython2u;
    ---


### test character handling

    input:  "0000000000000000000000000000000000000000000000000644cb7f5234089e"

    btc_hist=# SELECT  height_str::integer, uintstr_to_hexstr(chainwork_str) as chainwork 
       from data_chain order by height_str::integer;
     height_str |         chainwork          
    ------------+----------------------------
              1 | 0x200020002
              2 | 0x300030003
              3 | 0x400040004
              4 | 0x500050005
              5 | 0x600060006
              6 | 0x700070007
              7 | 0x800080008
    ....
         649596 | 0x13d358544b3387cdfb31c859
         649597 | 0x13d369e56800871bfacbf326
         649598 | 0x13d37b7684cd8669fa661df3
         649599 | 0x13d38d07a19a85b7fa0048c0
         649600 | 0x13d39e98be678505f99a738d
    (649600 rows)

    btc_hist=# select int_to_hexstr( 123456 ); ==>  0x1e240

    btc_hist=# select bigint_to_hexstr( 9224080739067477486::bigint );
    ERROR:  bigint out of range
    btc_hist=# select bigint_to_hexstr(  224080739067477486::bigint );
    -[ RECORD 1 ]----+-------------------
    bigint_to_hexstr | 0x31c183eb51fbdeeL
    
    btc_hist=# select get_fmt_date(time_str) from data_chain  order by random() limit 4;
        get_fmt_date     
    ---------------------
     2019-09-11 04:32:57
     2010-05-19 02:18:15
     2010-01-23 18:07:07
     2014-02-28 13:39:12


    0x8002848f97a3bdeeL
    0x13d39e98be678505f99a738d
    
    btc_hist=# create table in_oe_price ( 
      start_blk integer, finish_blk integer, startblktime bigint, finishblktime bigint, oe_price text);
    CREATE TABLE

    FPSTR = '/home/dbb/Documents/thartman_2020/tmp_oep_work/op_energy_prices/op_energy_report500.csv'
    btc_hist=# copy in_oe_price from FPSTR with CSV header delimiter E'\,';
    COPY 6492








```python



```


```python
import pandas as pd
pd.__version__
```




    '0.23.3'


