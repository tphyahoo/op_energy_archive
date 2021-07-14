

```python

'''
        Table "public.data_chain"
       Column      |       Type       | 
  -----------------+------------------+-
   blkheight       | integer          | 
   blkhash         | text             | 
   bits_hex        | text             | 
   difficulty      | double precision | 
   chainwork_hex   | text             | 
   median_time     | bigint           |
   chain_reward    | bigint           | 
   time_str        | text             | 
   median_time_str | text             | 
  Indexes:
     "dc_pkey" btree (blkheight)

   ---
blkheight       | 1
blkhash         | 0x00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048
bits_hex        | 0x1d00ffff
difficulty      | 1
chainwork_hex   | 0x200020002
median_time     | 1231469665
chain_reward    | 5000000000
time_str        | 1231469665
median_time_str | 1231469665

blkheight       | 649600
blkhash         | 0x0000000000000000000de228233696f11b6e1eb8b4c2703ae84ad1e3d6749787
bits_hex        | 0x170e92aa
difficulty      | 19314656404097
chainwork_hex   | 0x13d39e98be678505f99a738d
median_time     | 1600848688
chain_reward    | 1871970395825350
time_str        | 1600853476
median_time_str | 1600848688


create table data_chain as (
 SELECT b.height_str::integer as blkheight, 
        fix_quoted_numbers(b.hash_str) as blkhash,  
    uintstr_to_hexstr(bits_str) as bits_hex, 
        b.difficulty_str::float as difficulty, 
        uintstr_to_hexstr(chainwork_str) as chainwork_hex, 
        in_btc_raw.median_time,  
        in_btc_raw.chain_reward,
        time_str, 
        totaltime_str as median_time_str
 FROM in_bits_raw as b 
LEFT JOIN 
    in_stats_raw on (b.height_str = in_stats_raw.height_str) 
LEFT JOIN 
    in_btc_raw on (b.height_str::integer = in_btc_raw.block_num) );
    

'''

'''
btc_hist
               Table "public.in_bits_raw"
     Column     | Type | Collation | Nullable | Default 
----------------+------+-----------+----------+---------
 height_str     | text |           |          | 
 hash_str       | text |           |          | 
 bits_str       | text |           |          | 
 difficulty_str | text |           |          | 
 chainwork_str  | text |           |          | 
   ---
 height_str     | 1
 hash_str       | "00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048"
 bits_str       | "1d00ffff"
 difficulty_str | 1
 chainwork_str  | "0000000000000000000000000000000000000000000000000000000200020002"


              Table "public.in_stats_raw"
    Column     | Type | Collation | Nullable | Default 
---------------+------+-----------+----------+---------
 height_str    | text |           |          | 
 hash_str      | text |           |          | 
 subsidy_str   | text |           |          | 
 totalfee_str  | text |           |          | 
 time_str      | text |           |          | 
 totaltime_str | text |           |          | 
   ---
 height_str    | 1
 hash_str      | "00000000839a8e6886ab5951d76f411475428afc90947ee320161bbf18eb6048"
 subsidy_str   | 5000000000
 totalfee_str  | 0
 time_str      | 1231469665
 totaltime_str | 1231469665


               Table "public.in_btc_raw"
    Column    |  Type  | Collation | Nullable | Default 
--------------+--------+-----------+----------+---------
 block_num    | bigint |           |          | 
 median_time  | bigint |           |          | 
 chain_work   | text   |           |          | 
 chain_reward | bigint |           |          | 
   ---
 block_num    | 1
 median_time  | 1231469665
 chain_work   | 8590065666
 chain_reward | 5000000000

'''


'''
  function fix_quoted_numbers()
  uintstr_to_hexstr()

database: btc_hist
--
CREATE FUNCTION uintstr_to_hexstr(text) RETURNS text
 AS
'tStr = args[0]
tStr = tStr.strip( "\"")
tStr = tStr.lstrip("0")
return "0x"+tStr
' LANGUAGE plpython2u;
--
CREATE FUNCTION fix_quoted_numbers(text) RETURNS text
 AS
'tStr = args[0]
return "0x"+tStr.strip("\"")'
LANGUAGE plpython2u;
--
CREATE FUNCTION hexstr_to_bigint(text) RETURNS bigint
 AS
' return int( args[0], base=16)'
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
--
CREATE FUNCTION get_fmt_date( text) returns text
 AS 
'from datetime import datetime
ts = args[0]
return (datetime.utcfromtimestamp(float(ts)).strftime("%Y-%m-%d %H:%M:%S"))
' LANGUAGE plpython2u;

'''
```


```python
!ls -lh ~/Documents/thartman_2020/oep_work/*
```

    -rw-r--r-- 1 dbb dbb  98M Nov  9  2020 /home/dbb/Documents/thartman_2020/oep_work/op_energy_prices1601504062.tar.gz
    
    /home/dbb/Documents/thartman_2020/oep_work/import_blockbits_12nov20:
    total 176M
    -rw-rw-r-- 1 dbb dbb   49 Nov 12  2020 blockbits_header.txt
    -rw-r--r-- 1 dbb dbb 105M Nov 12  2020 blockbits.txt
    -rw-rw-r-- 1 dbb dbb   62 Nov 12  2020 blockstats_header.txt
    -rw-r--r-- 1 dbb dbb  71M Nov 12  2020 blockstats.txt
    -rw-rw-r-- 1 dbb dbb 3.2K Nov 15  2020 calc_difficulty_v0a0.py
    -rw-rw-r-- 1 dbb dbb  16K Nov 13  2020 calc_out.csv
    -rw-rw-r-- 1 dbb dbb 1.5K Nov 12  2020 import_blockbits.py
    -rw-rw-r-- 1 dbb dbb 1.4K Nov 12  2020 import_blockstats.py
    
    /home/dbb/Documents/thartman_2020/oep_work/op_energy_prices:
    total 271M
    -rw-r--r-- 1 dbb dbb 148K Sep 30  2020 blockbitsTest.txt
    -rw-r--r-- 1 dbb dbb 105M Sep 24  2020 blockbits.txt
    -rw-r--r-- 1 dbb dbb  49M Sep 24  2020 blockhashes.csv
    -rw-r--r-- 1 dbb dbb  46M Sep 24  2020 blockhashes.txt
    -rw-r--r-- 1 dbb dbb 104K Sep 30  2020 blockstatsTest.txt
    -rw-r--r-- 1 dbb dbb  71M Sep 24  2020 blockstats.txt
    -rw-r--r-- 1 dbb dbb 293K Sep 30  2020 op_energy_report500.csv
    -rw-r--r-- 1 dbb dbb 293K Sep 30  2020 op_energy_report500_scratch.csv



```python
!cat ~/Documents/thartman_2020/oep_work/import_blockbits_12nov20/import_*
```

    # -*- coding: utf-8 -*-
    """
    Created on Thu Nov 12 15:12:20 2020
    
    @author: dbb
    """
    
    # IMPORT 
    
    
    import psycopg2
    
    ## setup globals
    
    #DSN = 'dbname=btc_hist host=ol13h.local'
    DSN = 'dbname=btc_hist host=192.168.1.4 user=dbb password=dbb'
    
    SRC_TABLE = 'in_bits_raw'
    
    gconn = None
    gcurs = None
    
    
    def do_setup():
        # probably should THROW here
        return True
    
    #  "height","hash", "bits","difficulty","chainwork"
    #  btc_hist=# create table 
    #   in_bits_raw( height_str text, hash_str text, bits_str text, 
    #                 difficulty_str text, chainwork_str text);
    #
    
    
    ##=======================================================================
    def main():
        
        global DSN, gconn, gcurs
        contB = True 
        
        gconn = psycopg2.connect( DSN )
        gconn.set_session(readonly=False, autocommit=False)
        gcurs = gconn.cursor()
    
        do_setup()
        tSQL = "insert into in_bits_raw values ( %s,%s,%s,%s,%s) "
    
        rdF = open( 'blockbits.txt', 'r' )
        while True:
            ln0 = rdF.readline()
            if (cmp( ln0, '') == 0):
                break
     
            ln0 = ln0.strip()
            ln1 = rdF.readline().strip()
            ln2 = rdF.readline().strip()
            ln3 = rdF.readline().strip()
            ln4 = rdF.readline().strip()
            
            #print ln4
            gcurs.execute( tSQL, (ln0, ln1, ln2, ln3, ln4))
            if (  int(ln0) % 128 == 0):
                gconn.commit()
    
        print "Done"
    
    
    ##==================================================
    
    main()
    
    # -*- coding: utf-8 -*-
    """
    Created on Thu Nov 12 17:02:00 2020
    
    @author: dbb
    """
    
    # IMPORT
    
    
    import psycopg2
    
    ## setup globals
    
    #DSN = 'dbname=btc_hist host=ol13h.local'
    DSN = 'dbname=btc_hist host=192.168.1.4 user=dbb password=dbb'
    
    SRC_TABLE = 'in_stats_raw'
    
    gconn = None
    gcurs = None
    
    
    def do_setup():
        # probably should THROW here
        return True
    
    #
    #  create table in_stats_raw(
    #       height_str text, hash_str text, subsidy_str text,
    #       totalfee_str text, time_str text, totaltime_str text);
    #
    #
    ##=======================================================================
    def main():
    
        global DSN, gconn, gcurs
        contB = True
    
        gconn = psycopg2.connect( DSN )
        gconn.set_session(readonly=False, autocommit=False)
        gcurs = gconn.cursor()
    
        do_setup()
        tSQL = "insert into in_stats_raw values ( %s,%s,%s,%s,%s,%s) "
    
        rdF = open( 'blockstats.txt', 'r' )
        while True:
            ln0 = rdF.readline()
            if (cmp( ln0, '') == 0):
                break
    
            ln0 = ln0.strip()
            ln1 = rdF.readline().strip()
            ln2 = rdF.readline().strip()
            ln3 = rdF.readline().strip()
            ln4 = rdF.readline().strip()
            ln5 = rdF.readline().strip()
            #print ln4
            gcurs.execute( tSQL, (ln0, ln1, ln2, ln3, ln4, ln5))
            if (  int(ln0) % 128 == 0):
                gconn.commit()
    
        print "Done"
    
    
    ##==================================================
    
    main()
    


    dbb@ol13h:~/Documents/thartman_2020/bip-thomashartman-op_energy$ ls
    bip-op_energy.mediawiki            OP_ENERGY_onboarding_executive_summary.mediawiki
    Op_Energy_Parsing.hs               bip-prevent-manipulation-of-txfeerate
    chainwork_out.csv                  op_energyprice.hs
    ch.csv                             op_energy_scratch.hs
    datafetch/                         OP_ENERGY_Whitepaper.mediawiki
    fast_log_demo.cpp                  README
    OP_ENERGY_is_hiring.mediawiki      spec/
    op_energy_notes.txt                uint256_demo.py*

    dbb@ol13h:~/Documents/thartman_2020/bip-thomashartman-op_energy$ tree datafetch/
    datafetch/
     ├── getblockbits.sh
     ├── getblockhashesCsv.sh
     ├── getblockhashes.sh
     ├── getblockstats.sh
     └── README.md



    $locate blockbits.txt
    /home/dbb/Documents/thartman_2020/oep_work/import_blockbits_12nov20/blockbits.txt
    /home/dbb/Documents/thartman_2020/oep_work/op_energy_prices/blockbits.txt

    $ ls -lh /home/dbb/Documents/thartman_2020/oep_work/op_energy_prices
    total 271M
    -rw-r--r-- 1 dbb dbb 148K Sep 30  2020 blockbitsTest.txt
    -rw-r--r-- 1 dbb dbb 105M Sep 24  2020 blockbits.txt
    -rw-r--r-- 1 dbb dbb  49M Sep 24  2020 blockhashes.csv
    -rw-r--r-- 1 dbb dbb  46M Sep 24  2020 blockhashes.txt
    -rw-r--r-- 1 dbb dbb 104K Sep 30  2020 blockstatsTest.txt
    -rw-r--r-- 1 dbb dbb  71M Sep 24  2020 blockstats.txt
    -rw-r--r-- 1 dbb dbb 293K Sep 30  2020 op_energy_report500.csv
    -rw-r--r-- 1 dbb dbb 293K Sep 30  2020 op_energy_report500_scratch.csv
    
    $ ls -lh /home/dbb/Documents/thartman_2020/oep_work/import_blockbits_12nov20/
    total 176M
    -rw-rw-r-- 1 dbb dbb   49 Nov 12  2020 blockbits_header.txt
    -rw-r--r-- 1 dbb dbb 105M Nov 12  2020 blockbits.txt
    -rw-rw-r-- 1 dbb dbb   62 Nov 12  2020 blockstats_header.txt
    -rw-r--r-- 1 dbb dbb  71M Nov 12  2020 blockstats.txt
    -rw-rw-r-- 1 dbb dbb 3.2K Nov 15  2020 calc_difficulty_v0a0.py
    -rw-rw-r-- 1 dbb dbb  16K Nov 13  2020 calc_out.csv
    -rw-rw-r-- 1 dbb dbb 1.5K Nov 12  2020 import_blockbits.py
    -rw-rw-r-- 1 dbb dbb 1.4K Nov 12  2020 import_blockstats.py

