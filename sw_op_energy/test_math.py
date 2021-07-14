# adapted from https://en.bitcoin.it/wiki/Difficulty#How_is_difficulty_calculated.3F_What_is_the_difference_between_bdiff_and_pdiff.3F
#  notes added nov20  -dbb

import decimal, math

def unpack_exp(x):
  return 8*(x - 3)

block_1_packed_exp = 0x1d
block_1_exp =        unpack_exp( block_1_packed_exp )
block_1_mantissa =   float(0x00ffff)

packed_exp  =        0x1b
exp  =               unpack_exp( packed_exp )
mantissa  =          float(0x0404cb)

print  block_1_mantissa * 2**(block_1_exp)  / (mantissa * 2**(exp))
print (block_1_mantissa * 2**(block_1_exp)) / (mantissa * 2**(exp))
#  == 16307.420938523983

print math.log(block_1_mantissa *  2**(block_1_exp) / (mantissa * 2**(exp)))
#  ==  9.69937555549195

print math.log(block_1_mantissa *    2**(block_1_exp)) - math.log(mantissa * 2**(exp))
#  ==  9.69937555549194

print math.log(block_1_mantissa) + math.log(2**(block_1_exp)) - math.log(mantissa) - math.log(2**(exp))
#  ==  9.69937555549

print math.log(block_1_mantissa) + (block_1_exp)* math.log(2) - math.log(mantissa) - (exp)* math.log(2)
#  ==  9.69937555549194

print math.log(block_1_mantissa / mantissa) + (block_1_exp)* math.log(2) - (exp)* math.log(2)
#  ==  9.69937555549

print math.log(block_1_mantissa / mantissa) +(unpack_exp (block_1_packed_exp))* math.log(2) -(unpack_exp(packed_exp))* math.log(2)
#  ==  9.69937555549

print math.log(block_1_mantissa / mantissa) + (8*(block_1_packed_exp - 3))* math.log(2) - (8*(packed_exp - 3))* math.log(2)
#  ==  9.69937555549

print math.log(block_1_mantissa / mantissa) +((block_1_packed_exp - 3))* math.log(2**8) - ((packed_exp - 3))* math.log(2**8)
#  ==  9.69937555549

print math.log(block_1_mantissa / mantissa) + block_1_packed_exp * math.log(2**8) - packed_exp * math.log(2**8)
#  ==  9.69937555549

print math.log(block_1_mantissa / mantissa) + (block_1_packed_exp - packed_exp)* math.log(2**8)
#  ==  9.69937555549

#=====================================
# example -- convert Block Difficulty from compact encoding to full form
# Given that a block contains compact difficulty   0x1b0404cb
#    0x1b => number of bytes in the expanded value  (27-3 by the compact formula)
#    0x0404cb => difficulty; must be less than  0x7FFFFF
#
#  2 to-the-power-of (bits-per-byte * number of bytes)  => expanded constant
#  expanded-constant * diffulty-short-form  => difficulty-long-form
#
#  0x0404cb == 263371
#  0x00ffff ==  65535
#
#  8*(0x1d - 3)  ==  8* 0x1A  == 208  == 0xD0
#
#  8*(0x1b - 3)  ==  8* 0x18  == 192  == 0xC0
#  
#  2**192 == 6277101735386680763835789423207666416102355444464034512896L
#   hex(6277101735386680763835789423207666416102355444464034512896L)
#     0x1000000000000000000000000000000000000000000000000L
# 
#  2**208 == 411376139330301510538742295639337626245683966408394965837152256L
#    hex(2**208)   ----   208 - 192 = 16  ==  2bytes  ==  0x0000
#     0x10000000000000000000000000000000000000000000000000000L
#
#  0x0404cb * 2**192  ==
#  0x0404cb * 0x1000000000000000000000000000000000000000000000000L  ==
#         0x404cb000000000000000000000000000000000000000000000000L  ==
#  0x0404cb *  6277101735386680763835789423207666416102355444464034512896L   ==
#    1653206561150525499452195696179626311675293455763937233695932416L
#
#  0x00ffff * 2**208  ==
#  0x00ffff * 0x10000000000000000000000000000000000000000000000000000L  ==
#          0xffff0000000000000000000000000000000000000000000000000000L
#  0x00ffff * 411376139330301510538742295639337626245683966408394965837152256L  ==
#    26959535291011309493156476344723991336010898738574164086137773096960L
#
#  0x00ffff * 2**208  /  0x0404cb * 2**192   <--  does not bind the way it appears
#  (0x00ffff * 2**208)  /  (0x0404cb * 2**192)  <-- explicit parens
#   ==  16307L  ==  0x3FB3
#
#  0x00ffff * 2**0xD0 / float( 0x0404cb * 2**0xC0 )
#   == (0x00ffff * 2**0xD0) / float( 0x0404cb * 2**0xC0 )
#   ==  16307.420938523983
#
print 0x00ffff * 2**(8*(0x1d - 3)) / float(0x0404cb * 2**(8*(0x1b - 3)))
#   ==  16307.420938523983

