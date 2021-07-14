
## Calculation Notes on Difficulty in BTC
### Jupyter Notebook in Python 2  &nbsp; &nbsp;   nov20 -dbb


update: clarify the use of TARGET and DIFFICULTY terminology


```python
import os,sys

sys.version
```




    '2.7.17 (default, Jul 20 2020, 15:37:01) \n[GCC 7.5.0]'




```python
# adapted from https://en.bitcoin.it/wiki/Difficulty#How_is_difficulty_calculated.3F_What_is_the_difference_between_bdiff_and_pdiff.3F
#  notes added nov20  -dbb

import decimal, math

def unpack_exp(x):
  return 8*(x - 3)
```

  
**Context**:  Bitcoin mining uses a TARGET value shared across all participating mining nodes, to regulate the mathematical difficulty associated with mining a new, valid BLOCK as a candidate to add to the public BTC BLOCKCHAIN. 

The current target is defined as a 256-bit number that all Bitcoin clients share. The SHA-256 hash of a block's header must be lower than or equal to the current target for the block to be accepted by the network. The lower the target, the more difficult it is to generate a block.

There is a maximum TARGET which is an upper bound, and a current TARGET which is adjusted periodically and communicted through the working system.  Difficulty is a ratio of two TARGETS:  MAX_TARGET and CURRENT_TARGET. 
Each block stores a packed representation for its actual hexadecimal target -- BITS. 

The following constants are defined:

    /**
      Pooled mining defines a maximum target 28 bytes of 0xFF, and four bytes of leading zero
      target MAX  0x00000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF  # 32 bytes
            
       however, compact format MAX is 0x1d00FFFF => 0x00FFFF padded with (0x1d-3) 26 bytes of zero
      target MAXp 0x00000000FFFF0000000000000000000000000000000000000000000000000000  # 32 BYTES
   
      Standard Calc of new difficulty using target and previous target, executed periodically in real-time
        difficulty_NEW = difficulty_1_target / current_target
     */
 
    /**           code comments from:   arith_uint256.h  / bitcoind  v18
     *
     * The "compact" format is a representation of a whole
     * number N using an unsigned 32bit number, similar to a
     * floating point format.
     * The most significant 8 bits are the unsigned exponent of base 256.
     * This exponent can be thought of as "number of bytes of N".
     * The lower 23 bits are the mantissa.
     * Bit number 24 (0x800000) represents the sign of N.
     * N = (-1^sign) * mantissa * 256^(exponent-3)
     *    
     * Thus:
     *  compact(0x1234560000) = 0x05123456  ## five bytes with value of 0x123456 right-padded with zero
     *  compact(0xc0de000000) = 0x0600c0de  ##  sign bit set, not used for target difficulty in practice
     *
     * Bitcoin nodes only use this "compact" format for encoding difficulty
     * targets, which are unsigned 256bit quantities.  Thus, all the
     * complexities of the sign bit and using base 256 are probably an
     * implementation accident.
     */
 
    /* 
     * conversion macros in C/C++ use bit shifting to encode and decode compact values
     *    arith_uint256 bn = *this >> 8 * (8 - 3);  // right-shift 5 bytes
     *    
     *    uint32_t pn[8];  // a block of eight 32 bit values to get 256 bits
     *    GetLow64(pn) { return pn[0] | (uint64_t)pn[1] << 32; }
     *
     */
   


```python
# example -- expand target  0x1b0404cb from the compact BITS form
##  mantissa is the body of the stored value, always INTEGER
##  exponent is a count of bytes in the result, always a positive INTEGER

# compact format MAX TARGET   0x1d00ffff
block_1_packed_exp = 0x1d
block_1_exp =        unpack_exp( block_1_packed_exp )
block_1_mantissa =   float(0x00ffff)

# example TARGET
packed_exp  =        0x1b     ## first byte of the compact value
exp  =               unpack_exp( packed_exp )
mantissa  =          float(0x000404cb)

```


```python
## calculate 'bdiff', a block difficulty value 

print '-- bdiff -- difficulty measure '
print  block_1_mantissa * 2**(block_1_exp)  / (mantissa * 2**(exp))

##   add explicit parens for clarity and show that the results match

print (block_1_mantissa * 2**(block_1_exp)) / (mantissa * 2**(exp))


##  calculate 'pdiff' by using the larger max target
print '-- pdiff -- (unused) difficulty with a slightly larger upper bound'
print 0x00000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFL / (mantissa * 2**(exp))

```

    -- bdiff -- difficulty measure 
    16307.4209385
    16307.4209385
    -- pdiff -- (unused) difficulty with a slightly larger upper bound
    16307.6697738



```python
## use 32bit Long Integers and base 10 logarithms to calculate difficulty
##  note this log() technique is not needed in Python, due to 
##   built-in support for Very Long Integer math in the core language
##  but the log() approach could be used in a low-level language like C/C++

## mantissa is an INT in compact format, exponent is effectively a count of right-pad bytes
print math.log( block_1_mantissa *  2**(block_1_exp) / (mantissa * 2**(exp)))
#  ==  9.69937555549195

##    subtracting two log values is equivalent to division
print math.log(block_1_mantissa *  2**(block_1_exp)) - math.log(mantissa * 2**(exp))
#  ==  9.69937555549194

##    adding two log values is equivalent to multiplication
print math.log(block_1_mantissa) + math.log(2**(block_1_exp)) - math.log(mantissa) - math.log(2**(exp))
#  ==  9.69937555549

##    log(2 to the Nth power) is equivalent to N * log(2)
print math.log(block_1_mantissa) + (block_1_exp)* math.log(2) - math.log(mantissa) - (exp)* math.log(2)
#  ==  9.69937555549194

##    subtraction of log values is division, use algebra to combine terms
print math.log(block_1_mantissa / mantissa) + (block_1_exp)* math.log(2) - (exp)* math.log(2)
#  ==  9.69937555549

##    show the compact format unpack_exp() expplicitly]
print math.log(block_1_mantissa / mantissa) +(unpack_exp (block_1_packed_exp))* math.log(2) -(unpack_exp(packed_exp))* math.log(2)
#  ==  9.69937555549

##    show the extaction of bytes from compact form explicitly
print math.log(block_1_mantissa / mantissa) + (8*(block_1_packed_exp - 3))* math.log(2) - (8*(packed_exp - 3))* math.log(2)
#  ==  9.69937555549

```

    9.69937555549
    9.69937555549
    9.69937555549
    9.69937555549
    9.69937555549
    9.69937555549
    9.69937555549



```python
## Example of python native long integer handling

n0 = 2**192
print hex( n0 )   ## 24 bytes
print "result is of type: %s" % type(n0)
print "bits to store for value: %s" % str(n0.bit_length())
```

    0x1000000000000000000000000000000000000000000000000L
    result is of type: <type 'long'>
    bits to store for value: 193

