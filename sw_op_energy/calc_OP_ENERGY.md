
### BTC OP_ENERGY
####    &nbsp;  OE Skunk Works    &nbsp;  &nbsp;       13mar21        -dbb


```python
import psycopg2 as pg
import pandas.io.sql as psql

conn  = pg.connect("dbname=btc_hist")
curs = conn.cursor()
```


```python
tSQL = '''
SELECT   blkheight, median_time, chain_reward, chainwork_hex
 FROM    data_chain   ORDER BY  blkheight
'''

res0 = curs.execute( tSQL )
rData = curs.fetchall()
```


```python
divmod? # divmod(x, y, /)  Return the tuple (x//y, x%y).  Invariant: div*y + mod == x.
```

      blkheight, median_time, chain_reward, chainwork_hex
    [ type(e) for e in rData[0] ]  =>  [int, int, int, str]

    btc_hist=# \d data_chain
                      Table "public.data_chain"
         Column      |       Type     
    -----------------+------------------
     blkheight       | integer     
     blkhash         | text             
     bits_hex        | text              
     difficulty      | double precision 
     chainwork_hex   | text             
     median_time     | bigint           
     chain_reward    | bigint           
     time_str        | text              
     median_time_str | text              



```python
def unpack_exp(x):
  return 8*(x - 3)

def expand_cbits(x):
    
    bits_hex = int( x, 16)
    reg_difficulty  = bits_hex & 0x007FFFFF
    reg_exp_enc = (bits_hex & 0xFF000000) >> 24
    
    exp_const = 1 * 2**unpack_exp(reg_exp_enc)
    exp_var = reg_difficulty * 2**unpack_exp(reg_exp_enc)
    bCnt = (exp_var.bit_length() +7)/8
    
    #print( 'bits_hex,reg_difficulty,reg_exp_enc,exp_const,exp_var,bCnt' )
    #print( hex(bits_hex), hex(reg_difficulty),bCnt )
    #print( hex(reg_exp_enc), hex(exp_const) )
    #print('  ',hex(exp_var))
    
    return exp_var
    

```


```python
def compact2_cbits(in_str):
    if isinstance(in_str, str):
        if in_str[0:2] == '0x':
            res = int( in_str, base=16)
        else:
            res = int( in_str)
    elif isinstance( in_str, int):
        res = in_str
    else:
        return 0
    
    cnt  = 1
    res2 = res
    while True:
        cnt = cnt + 1
        res2,elem = divmod(res2,0x100)
        if res2 < 0x100: 
            break

    ## test for high bit, and shift here   
    if res2 > 127:
        cnt = cnt + 1
            
    res2  = res >> ((cnt-3)*8)
    res2b = res2 & 0x007FFFFF
    #print( hex(res2), hex(res2b) )
    cnt2 = cnt << 24
    #print( hex(cnt2) )
    return (hex(cnt2 | res2))


def compact_cbits(in_str):
    res  = int( in_str, base=16)
    res2 = res
    cnt  = 1
    while True:
        cnt = cnt + 1
        res2 = int(res2 / 0x100)
        if res2 < 0x10: 
            break

    res2  = res >> ((cnt-3)*8)
    res2b = res2 & 0x007FFFFF
    #print( hex(res2), hex(res2b) )
    cnt2 = cnt << 24
    #print( hex(cnt2) )
    return (hex(cnt2 | res2))

```


```python
tEpoch = 302 * 2016
blk0    = rData[ tEpoch ]
blk01   = rData[ tEpoch+144 ]
```


```python
hash_cnt      = int( blk01[3],base=16) - int( blk0[3],base=16)
expected_secs = 600 * ( blk01[0] - blk0[0] )
actual_secs   = blk01[1] - blk0[1]
sats          = blk01[2] - blk0[2]

# price_prime  = (hash_cnt * expected_secs)/(actual_secs*sats)
# price_prime_int = int(price_prime)
## 22mar21  - use DIVMOD() builtin for python
price_prime,elem  = divmod( (hash_cnt * expected_secs),(actual_secs*sats) )

print('hash_cnt      = ',hex(hash_cnt))
print('expected_secs = ',expected_secs)
print('actual_secs   = ',actual_secs)
print('sats          = ',hex(sats))
print('-')
print('price_prime  = ',price_prime,' = ',hex(price_prime))
#print('price_prime_int = ', price_prime_int, '   ',str(hex(price_prime_int) ))
print('cBITS(price_prime) = ', str(compact_cbits(str(hex( price_prime ) ))))
```

    hash_cnt      =  0x69fde0cadff04450a1e20
    expected_secs =  86400
    actual_secs   =  81035
    sats          =  0x2a82c83a7f
    -
    price_prime  =  46766282572136  =  0x2a889fa90168
    cBITS(price_prime) =  0x7002a88



```python
tEpoch = 120 * 2016
blk0    = rData[ tEpoch ]
blk01   = rData[ tEpoch+1 ]
```

      blkheight median_time    chain_reward      chainwork_hex
    ( 241921,  1371417533,  1131625885833039, '0x52e4d969fee4dd57fa')
    ( 241922,  1371417786,  1131628392433133, '0x52e600832043b9d907')
    
    compact_cbits( blk0[3])     =>  '0x952e4d9'
    compact_cbits( blk01[3])    =>  '0x952e600'


```python
hash_cnt      = int( blk01[3],base=16) - int( blk0[3],base=16)
expected_secs = 600 * ( blk01[0] - blk0[0] )
actual_secs   = blk01[1] - blk0[1]
sats          = blk01[2] - blk0[2]

# price_prime  = (hash_cnt * expected_secs)/(actual_secs*sats)
# price_prime_int = int(price_prime)
## 22mar21  - use DIVMOD() builtin for python
price_prime,elem  = divmod( (hash_cnt * expected_secs),(actual_secs*sats) )

print('hash_cnt      = ',hex(hash_cnt))
print('expected_secs = ',expected_secs)
print('actual_secs   = ',actual_secs)
print('sats          = ',hex(sats))
print('-')
print('price_prime  = ',price_prime,' = ',hex(price_prime))
#print('price_prime_int = ', price_prime_int, '   ',str(hex(price_prime_int) ))
print('cBITS(price_prime) = ', str(compact_cbits(str(hex( price_prime ) ))))
```

    hash_cnt      =  0x12719215edc810d
    expected_secs =  600
    actual_secs   =  253
    sats          =  0x9567ae9e
    -
    price_prime  =  78587229  =  0x4af255d
    cBITS(price_prime) =  0x404af25



```python
compact_cbits( blk01[3])
```




    '0xa0052e6'




```python
test_val = '0x404af25'
res_t0 = expand_cbits( test_val)
print(res_t0, hex(res_t0))
```

    78587136 0x4af2500



```python
## -- chain 241924 : 
##    cBITS  0x1a00de15
##    cwDiff 0x000000000000000000000000000000000000000000000000012719215edc810d
##   cwTargetValue 0x00000000000000de150000000000000000000000000000000000000000000000
##                               0xde150000000000000000000000000000000000000000000000

## btc_hist=# select bits_hex from data_chain where blkheight = (2016*120);
cbits_str = '0x1a00de15'
res_exp = hex(expand_cbits( cbits_str ))
res_cmpct = compact_cbits( str(res_exp) )

print('cbits_str      = ',cbits_str)
print('res_exp        = ',res_exp)
print('res_cmpct      = ',res_cmpct)
```

    cbits_str      =  0x1a00de15
    res_exp        =  0xde150000000000000000000000000000000000000000000000
    res_cmpct      =  0x1a00de15



```python
## btc_hist=# select bits_hex from data_chain where blkheight = (2016*120);
cbits_str = '0x1a0375fa'
res_exp = hex(expand_cbits( cbits_str ))
res_cmpct = compact_cbits( str(res_exp) )

print('cbits_str      = ',cbits_str)
print('res_exp        = ',res_exp)
print('res_cmpct      = ',res_cmpct)
```

    cbits_str      =  0x1a0375fa
    res_exp        =  0x375fa0000000000000000000000000000000000000000000000
    res_cmpct      =  0x1a0375fa



```python
## btc_hist=# select bits_hex from data_chain where blkheight = (2016*120);
cbits_str = '0x1800b0ed'
res_exp = hex(expand_cbits( cbits_str ))
res_cmpct = compact_cbits( str(res_exp) )

print('cbits_str      = ',cbits_str)
print('res_exp        = ',res_exp)
print('res_cmpct      = ',res_cmpct)
```

    cbits_str      =  0x1800b0ed
    res_exp        =  0xb0ed000000000000000000000000000000000000000000
    res_cmpct      =  0x1800b0ed



```python
in_val_str = '0xde150000000000000000000000000000000000000000000000'
res0 = compact2_cbits(in_val_str)
print( 'str res0 = ',res0)

in_val_str = '0xb0ed000000000000000000000000000000000000000000'
res1 = compact2_cbits(in_val_str)
print( 'str res1 = ',res1)

in_val_str = '0x375fa0000000000000000000000000000000000000000000000'
res2 = compact2_cbits(in_val_str)
print( 'str res2 = ',res2)


```

    str res0 =  0x1a00de15
    str res1 =  0x1800b0ed
    str res2 =  0x1a0375fa



```python
in_val_str[0:1]
```




    '0'




```python
int( '0x52e4d969fee4dd57fa', base=16)
```




    1529123342098259335162



    0x13d39e98be678505f99a738d
    
    0x52e4d969fee4dc000
    0x52e4d969fee4dc00
    0x52e4d969fee4dc0
    0x52e4d969fee4dc



```python
hex( int(0x52e4d969fee4dc / 0x10) )
```




    '0x52e4d969fee4d'


