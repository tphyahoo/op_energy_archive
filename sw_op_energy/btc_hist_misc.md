

```python
%matplotlib inline
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

```


```python
#df = pd.read_csv("data/titanic.csv")

# how to get pandas data from postgree sql using python 
import psycopg2 as pg
import pandas.io.sql as psql

conn  = pg.connect("dbname=btc_hist")
#df = pd.read_sql_query('select * from in_tradetree',con=conn )
df = pd.read_sql_query('select * from in_mtgox',con=conn )
#print(df)
```

             timestamp7  interval7    open7    high7     low7   close7  volume7
    0        1279408140         60  0.04951  0.04951  0.04951  0.04951     20.0
    1        1279408200         60  0.04951  0.04951  0.04951  0.04951      0.0
    2        1279408260         60  0.04951  0.04951  0.04951  0.04951      0.0
    3        1279408320         60  0.04951  0.04951  0.04951  0.04951      0.0
    4        1279408380         60  0.04951  0.04951  0.04951  0.04951      0.0
    5        1279408440         60  0.04951  0.04951  0.04951  0.04951      0.0
    6        1279408500         60  0.04951  0.04951  0.04951  0.04951      0.0
    7        1279408560         60  0.04951  0.04951  0.04951  0.04951      0.0
    8        1279408620         60  0.04951  0.04951  0.04951  0.04951      0.0
    9        1279408680         60  0.04951  0.04951  0.04951  0.04951      0.0
    10       1279408740         60  0.04951  0.04951  0.04951  0.04951      0.0
    11       1279408800         60  0.04951  0.04951  0.04951  0.04951      0.0
    12       1279408860         60  0.04951  0.04951  0.04951  0.04951      0.0
    ...             ...        ...      ...      ...      ...      ...      ...
    1898086  1279459200         60  0.05941  0.05941  0.05941  0.05941      0.0
    1898087  1279459260         60  0.05941  0.05941  0.05941  0.05941      0.0
    1898088  1279459320         60  0.05941  0.05941  0.05941  0.05941      0.0
    1898089  1279459380         60  0.05941  0.05941  0.05941  0.05941      0.0
    1898090  1279459440         60  0.05941  0.05941  0.05941  0.05941      0.0
    
    [1898091 rows x 7 columns]



```python
df.info()
```

    <class 'pandas.core.frame.DataFrame'>
    RangeIndex: 1898091 entries, 0 to 1898090
    Data columns (total 7 columns):
    timestamp7    int64
    interval7     int64
    open7         float64
    high7         float64
    low7          float64
    close7        float64
    volume7       float64
    dtypes: float64(5), int64(2)
    memory usage: 101.4 MB



```python
#df['low7'].hist()
df[ df['close7'] > 500]['close7'].hist()
```




    <matplotlib.axes._subplots.AxesSubplot at 0x7fe1ef3ec240>




![png](img/output_3_1.png)



```python
resW = df['high7'].rolling(144, win_type='boxcar')
type(resW)
```




    pandas.core.window.Window




```python
df['close7'].describe()
```




    count    1.898091e+06
    mean     9.048209e+01
    std      2.199692e+02
    min      1.000000e-02
    25%      3.062185e+00
    50%      9.049870e+00
    75%      7.712500e+01
    max      1.241100e+03
    Name: close7, dtype: float64




```python
#df[ df['close7'] > 400 ].describe()
df[ df['close7'] > 400 ]['low7'].plot()
```




    <matplotlib.axes._subplots.AxesSubplot at 0x7fe1ef735b00>




![png](img/output_6_1.png)



```python
# btc_hist=# 
#  create table tt_difs as select timestamp7, open7, close7-open7 from in_tradetree  order by timestamp7 ;
#    SELECT 360135

df_qry1 = pd.read_sql_query('select * from tt_diffs',con=conn )

```


```python
df_qry1.describe()
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
      <th>timestamp7</th>
      <th>open7</th>
      <th>diff</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <th>count</th>
      <td>3.601350e+05</td>
      <td>360135.000000</td>
      <td>360135.000000</td>
    </tr>
    <tr>
      <th>mean</th>
      <td>1.318368e+09</td>
      <td>7.550463</td>
      <td>-0.000068</td>
    </tr>
    <tr>
      <th>std</th>
      <td>6.237730e+06</td>
      <td>5.022082</td>
      <td>0.041823</td>
    </tr>
    <tr>
      <th>min</th>
      <td>1.307564e+09</td>
      <td>2.020000</td>
      <td>-7.895000</td>
    </tr>
    <tr>
      <th>25%</th>
      <td>1.312966e+09</td>
      <td>3.500000</td>
      <td>0.000000</td>
    </tr>
    <tr>
      <th>50%</th>
      <td>1.318368e+09</td>
      <td>5.688122</td>
      <td>0.000000</td>
    </tr>
    <tr>
      <th>75%</th>
      <td>1.323770e+09</td>
      <td>10.976199</td>
      <td>0.000000</td>
    </tr>
    <tr>
      <th>max</th>
      <td>1.329172e+09</td>
      <td>34.895000</td>
      <td>4.240000</td>
    </tr>
  </tbody>
</table>
</div>




```python
df_qry1[20000:21440]['open7'].plot()
```




    <matplotlib.axes._subplots.AxesSubplot at 0x7fc4a69a6240>




![png](img/output_9_1.png)

