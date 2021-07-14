
# coding: utf-8

# In[ ]:
##  EXPORT of working Jupyter Notebook
##   main use is to iterate through data_chain rows
##    select a subset at Regimen change, add an accum diff per-line
##

get_ipython().magic('matplotlib inline')
import numpy as np
import pandas as pd
#import matplotlib.pyplot as plt
import seaborn 


# ## Demo of BTC Chainwork
# ####    &nbsp;  OE Skunk Works    &nbsp;  &nbsp;       29dec20        -dbb

# In[ ]:


import psycopg2 as pg
import pandas.io.sql as psql

conn  = pg.connect("dbname=btc_hist")
curs = conn.cursor()


# ### Gather chainwork values

# In[ ]:


tSQL = '''
copy (select height_str::integer, bits_hex, chainwork_hex from data_chain 
   order by height_str::integer) 
to '/tmp/tmp_chaingrease.csv' with CSV header delimiter E'\|';

'''
# copy (select height_str::integer as blk_ht, bits_hex, chainwork_hex from data_chain ORDER BY height_str::integer) 
#  to '/tmp/tmp_chain.csv' with csv header;
#


# In[ ]:


res0 = curs.execute(tSQL)


# In[ ]:


dfile = open('/tmp/tmp_chaingrease.csv','r')
tline = dfile.readline()
#dfile.close()


# ### Extract a small series of rows per Difficulty Regimen

# In[ ]:


hts = []
bitsL = []
cworks = []

for tline in dfile:
    tAr = tline.split('|')
    hts.append( tAr[0])
    bitsL.append( tAr[1])
    cworks.append( int( tAr[2],16))


# In[ ]:



##-- SETUP data file with sampled lines-per-Regimen
##     emits 1540 lines for 308 Regimen
##
##-- experiment ZERO chainwork diffs column --
##     compute diffs for all lines, emit only at difficulty change

## loop iterations
cnt = 0
## samples-per-Regimen
ROWS_SAMPLED = 2016  # 4
subset_cnt = ROWS_SAMPLED
## track values by cur_cell + prev_cell
val_prev = 0
difc_prev = 1.0
# list of difference values, one-per-line of input
cdiffs = [ val_prev ]

for elem in cworks:
    if cnt == 0:
        val_prev = elem
        cnt = 1 
        continue
    
    cdiffs.append( elem - val_prev )
    val_prev = elem

    if difc_prev != bitsL[cnt]:
        print( int(hts[cnt]), bitsL[cnt], hex(cworks[cnt]), hex(cdiffs[cnt]) )
        subset_cnt = ROWS_SAMPLED
        difc_prev =  bitsL[cnt]

    elif subset_cnt > 0:
        print( int(hts[cnt]), bitsL[cnt], hex(cworks[cnt]), hex(cdiffs[cnt]) )
        subset_cnt = subset_cnt - 1  
    
    cnt = cnt + 1


# In[ ]:


## try the last two chainwork vars, check the result

tt0 = 0x13b51559ae0fba70aa9e0d34
# tt0.bit_length()  => 93

##  kalctool_bin$ ./kalctool SUB 0x13b526eacadcb9beaa383801 0x13b51559ae0fba70aa9e0d34
##   0x11911cccff4dff9a2acd   => MATCH difference 

tr9 = 0xFFFF0000000000000000000000000000000000000000000000000000
# tr9.bit_length()   => 224

hex(tr9 - 1)  ## => 0xfffeffffffffffffffffffffffffffffffffffffffffffffffffffff

MAX_VAL = tr9


# In[ ]:


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
    

print( 'Height: 1')
print( 'cBits ',hex( expand_cbits( '0x1d00ffff' )))
intRes = MAX_VAL / expand_cbits( '0x1d00ffff' )
print( hex( intRes * 0x100010001 ) , '0x100010001' )
print('-')

##  (42336, '0x1c43b3e5', '0xbe6c876ab1ca', '0x3c7ff250a')
print( 'Height: 42336')
print( 'cBits ',hex( expand_cbits( '0x1c43b3e5' )))
intRes = MAX_VAL / expand_cbits( '0x1c43b3e5' )
print( hex( intRes * 0x100010001 ) , '0x3c7ff250a' )
print('-')

##  (84672, '0x1b2f8e9d', '0x9c0b06bbd7ad42', '0x5620c97df82')
print( 'Height: 84672')
print( 'cBits ', hex( expand_cbits( '0x1b2f8e9d' )))
intRes = MAX_VAL / expand_cbits( '0x1b2f8e9d' )
print( hex( intRes * 0x100010001 ) , '0x5620c97df82' )
print('-')


## (635040, '0x1711d4f2', '0x106a77887e89686182bb3f0bL', '0xe5b3a6efdb4660e552bL')
print( 'Height: 635040')
print( 'cBits ',hex( expand_cbits( '0x1711d4f2' )))
intRes = MAX_VAL / expand_cbits( '0x1711d4f2' )
print( hex( intRes * 0x100010001 ) , '0xe5b3a6efdb4660e552bL' )
print('-')

    


#     ------------------------------------
#     Reg# 20     blk_ht: 42336
#             seconds: 1009409
#      from 2010-02-24 08:41:04
#        to 2010-03-08 01:04:33
#  
#      bits_hex: 0x1c43b3e5  | 0x1c  0x43b3e5
#         exp_const: 0x100000000000000000000000000000000000000000000000000
#           exp_var: 0x43b3e500000000000000000000000000000000000000000000000000
# 
#      exp_var bits: 222
#     exp_var bytes:  28  0x1c
#      calc_diff: 0x387f6f     Difficulty -> INCREASE
#      
#     ------------------------------------
#     Reg# 41     blk_ht: 84672
#             seconds: 775638
#      from 2010-10-12 05:35:05
#        to 2010-10-21 05:02:23
#  
#      bits_hex: 0x1b2f8e9d  | 0x1b  0x2f8e9d
#         exp_const: 0x1000000000000000000000000000000000000000000000000
#           exp_var: 0x2f8e9d000000000000000000000000000000000000000000000000
# 
#      exp_var bits: 213
#     exp_var bytes:  27  0x1b
#      calc_diff: 0x1e7eca     Difficulty -> INCREASE
#  

# In[ ]:


get_ipython().magic('whos')


# ### Chainwork -- method1  floats

# In[ ]:


tSQL = '''
copy (select height_str::integer as blk_ht, bits_hex, chainwork_hex from data_chain
  ORDER BY height_str::integer) to '/tmp/tmp_chain.csv' with csv header
'''

tSQL = '''
copy (select height_str::integer, difficulty,chainwork_hex from data_chain 
   order by height_str::integer) 
to '/tmp/tmp_chain.csv' with CSV header delimiter E'\|';

'''

curs = conn.cursor()


# In[ ]:


res0 = curs.execute(tSQL)


# In[ ]:


dfile = open('/tmp/tmp_chain.csv','r')
tline = dfile.readline()
#dfile.close()


# In[ ]:



hts = []
difficultyL = []
cworks = []

for tline in dfile:
    tAr = tline.split('|')
    hts.append( tAr[0])
    difficultyL.append( tAr[1])
    cworks.append( int( tAr[2],16))

##-- experimental chainwork diffs column --
cnt = 0
subset_cnt = 4
val_prev = 0
difc_prev = 1.0
cdiffs = [ val_prev ]
for elem in cworks:
    if cnt == 0:
        val_prev = elem
        cnt = 1 
        continue
    
    cdiffs.append( elem - val_prev )
    val_prev = elem

    if difc_prev != difficultyL[cnt]:
        print( int(hts[cnt]), float(difficultyL[cnt]), hex(cworks[cnt]), hex(cdiffs[cnt]) )
        subset_cnt = 4
        difc_prev =  difficultyL[cnt]

        resT = 0x100010001 * float(difficultyL[cnt])
        print( hex(int(resT)), "  ", hex(cdiffs[cnt]) )

    elif subset_cnt > 0:
        print( int(hts[cnt]), float(difficultyL[cnt]), hex(cworks[cnt]), hex(cdiffs[cnt]) )
        subset_cnt = subset_cnt - 1  
        
        resT = 0x100010001 * float(difficultyL[cnt])
        print( hex(int(resT)), "  ", hex(cdiffs[cnt]) )
    
    cnt = cnt + 1


# In[ ]:


hex(cdiffs[ 320000])


# In[ ]:


#cdiffs[-4:-1]
res33 = cdiffs[-1]
res33


# In[ ]:


#82957083413710630431437L
res33.bit_length()


# ###  Pandas Dataframe Experiments

# In[ ]:




# https://stackoverflow.com/questions/30522724/take-multiple-lists-into-dataframe
df = pd.DataFrame( list( map(list,zip( hts,difficultyL,cworks))))
#df = pd.DataFrame( list( map(list,zip( hts,bits,cdiffs))))

# https://stackoverflow.com/questions/15891038/change-column-type-in-pandas
#  chainwork overflows np.uint64 here
#df = df.astype({ 0:'int', 2:long} )

df.set_index( 0)

# https://pandas.pydata.org/pandas-docs/stable/user_guide/basics.html#reindexing-and-altering-labels
df.rename({ 0: 'blk_ht', 1: 'target_difficulty', 2:'chainwork' }, axis='columns', inplace=True)


# In[ ]:


df.describe()


# In[ ]:


df.info()


# In[ ]:


df[2016:-1:2016]['chainwork'].plot()


# In[ ]:


df


# ----------------------------------------------------------------

# In[ ]:


res = 6136047486073716719945937805L
print(type(res))


# In[ ]:


tSQL = '''  SELECT
 height_str::integer as blk_ht, bits_hex, 
 chainwork_hex
from data_chain
'''

df = pd.read_sql_query( tSQL ,con=conn )


# In[ ]:


df


# In[ ]:


#df = pd.read_csv('/tmp/tmp_chain.csv', index_col='blk_ht', dtype={'chainwork_hex':np.int64})

