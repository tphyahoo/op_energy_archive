###   oe_test_boost
###    nov20  -dbb


    oe_test_boost$ ./oe_test_boost 
    Running 9 test cases...
    oe_fixture c'tor
    oe_fixture d'tor
    oe_fixture c'tor
    oe_fixture d'tor
    oe_fixture c'tor
    oe_fixture d'tor
    oe_fixture c'tor
    TEST_CASE( basics ) 
      R1L.ToString() 0x7d1de5eaf9b156d53208f033b5aa8122d2d2355d5e12292b121156cfdb4a529c
      R1S.ToString() 0xb5aa8122d2d2355d5e12292b121156cfdb4a529c
      R2L.ToString() 0xd781cab4f072134971da2d19a3473013bfb69ca6c30a7e26406ba5477c1d3270
      R2S.ToString() 0xa3473013bfb69ca6c30a7e26406ba5477c1d3270
      ZeroL.ToString() 0x0000000000000000000000000000000000000000000000000000000000000000
      ZeroS.ToString() 0x0000000000000000000000000000000000000000
      MaxL.ToString()  0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
      MaxS.ToString()  0xffffffffffffffffffffffffffffffffffffffff
      OneL.ToString()  0x0000000000000000000000000000000000000000000000000000000000000001
      OneS.ToString()  0x0000000000000000000000000000000000000001
    oe_fixture d'tor
    oe_fixture c'tor
    TEST_CASE( comparison ) // <= >= < >
     (ZeroL < R1L) = 1
     (R2L < R1L) = 1
     (ZeroL < OneL) = 1
     (OneL < MaxL) = 1
     (R1L < MaxL) = 1
     (R2L < MaxL) = 1
     (ZeroS < R1S) = 1
     (R2S < R1S) = 1
     (ZeroS < OneS) = 1
     (OneS < MaxS) = 1
     (R1S < MaxS) = 1
     (R2S < MaxS) = 1
    oe_fixture d'tor
    oe_fixture c'tor
    TEST_CASE( methods )
      // GetHex SetHex begin() end() size() GetLow64 GetSerializeSize, Serialize, Unserialize
    oe_fixture d'tor
    oe_fixture c'tor
    TEST_CASE( conversion ) // 
     ArithToUint256(UintToArith256(ZeroL)) 
       0x0000000000000000000000000000000000000000000000000000000000000000
     ArithToUint256(UintToArith256(OneL))  
       0x0000000000000000000000000000000000000000000000000000000000000001
     ArithToUint256(UintToArith256(R1L))   
       0x7d1de5eaf9b156d53208f033b5aa8122d2d2355d5e12292b121156cfdb4a529c
     ArithToUint256(UintToArith256(R2L))   
       0xd781cab4f072134971da2d19a3473013bfb69ca6c30a7e26406ba5477c1d3270
     arith_uint256(R1L.GetHex())           
       0x7d1de5eaf9b156d53208f033b5aa8122d2d2355d5e12292b121156cfdb4a529c
    oe_fixture d'tor
    oe_fixture c'tor
    oe_fixture d'tor
    oe_fixture c'tor
    oe_fixture d'tor
