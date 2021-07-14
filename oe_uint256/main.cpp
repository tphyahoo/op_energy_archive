#include <iostream>
#include <iomanip>
#include <cstdint>

#include "crypto/common.h"
#include "endian.h"
#include "prevector.h"

#include "hash.h"
#include "uint256.h"
#include "arith_uint256.h"

int res_test_DRAGON_TWO();
int do_sizesTest();
int do_test_GetCompact();
int do_test_Chainwork_calc();
int do_TwoBlocks_Compact();
int do_container_test();

bool is_verbose;

/*
 *    Demo  oe_uint256
 *    10nov20  -dbb
 *
 *    A C++ command line tool that imports  bitcoind  math directly
 *    Demonstrate basic types, conversion and compare those results.
 *    Conversions done with this code MUST match actual BTC, and alt methods
 *      specifically  python and Jupyter Notebook outputs
 *
 */
using namespace std;

int main() {
    is_verbose = true;

    int res_test_DT = res_test_DRAGON_TWO();
    int res_test_gc = do_test_GetCompact();
    int res_test_cw = do_test_Chainwork_calc();
    int res_test_TB = do_TwoBlocks_Compact();
    int res_test_sz = do_sizesTest();

    return 0;
}

//************************************************************************************************
int res_test_DRAGON_TWO()
{
    //-------------------------------------------------------------------------
    const string str_maxtarg = "0xffff0000000000000000000000000000000000000000000000000000";
    string str_tmaxt;
    arith_uint256  u256_maxtarg;
    uint32_t u32_maxtarg, u32_maxtarg_simpleflag, u32_maxtarg_noflag;
    uint64_t u64_maxtarg, u64_low64;
    double   d_maxtarg;
    bool     isNeg;

    u256_maxtarg.SetHex( str_maxtarg );
    str_tmaxt = u256_maxtarg.GetHex();
    d_maxtarg   = u256_maxtarg.getdouble();
    u64_low64 = u256_maxtarg.GetLow64();
    u64_maxtarg = (uint64_t)d_maxtarg;

    u32_maxtarg_noflag = u256_maxtarg.GetCompact();
    u32_maxtarg = u256_maxtarg.GetCompact(&isNeg);
    isNeg = false;
    u32_maxtarg_simpleflag = u256_maxtarg.GetCompact(isNeg);
    if (is_verbose) {
        cout<<"----- MAX_TARG uint256 to uint64 --"<<endl;
        cout<<"--  given MAX_TARGET uint256,  extract and show string, uint64 values"<<endl;
        cout<<hex<<"  u256_maxtarg 0x"<<u256_maxtarg.ToString()<<endl;
        cout<<"  u256_maxtarg 0x"<<str_tmaxt<<"  GetHex()"<<endl;
        cout <<"  uint64_maxtarg "<<hex<<u64_maxtarg<<endl;
        cout<<hex<<"   u64_maxtarg GetLow64() 0x"<<u64_low64<<endl;
        cout <<"     d_maxtarg "<<d_maxtarg<<"  getdouble()"<<endl;
        cout<< std::fixed << std::setw(11)<<"     d_maxtarg "<<d_maxtarg<<endl<<endl;

        cout<<"----- DRAGON_TWO--"<<endl;
        cout<<"--  show that malformed input to cBITS conversion can result in a false neg flag"<<endl;
        cout<<"  #u32_maxtarg = u256_maxtarg.GetCompact(&isNeg)  // c++11 arith_uint256 "<<endl;
        cout<<hex<<"   u32_maxtarg cBITS 0x"<<u32_maxtarg<<"     bool isNeg "<<isNeg<<endl;
        cout<<hex<<" u32_maxtarg_simpleflag cBITS 0x"<<u32_maxtarg_simpleflag<<endl;
        cout<<hex<<" u32_maxtarg_noflag cBITS 0x"<<u32_maxtarg_noflag<<endl;
        cout<<" "<<endl;
    }
  return 0;
}

//-----------------------------------------------------------------------------------------------
int do_test_Chainwork_calc() {
    arith_uint256 t_Res;

    // hex( 2**48 )  python
    t_Res.SetHex( "0x1000000000000");
    t_Res = 1.0 / t_Res;

    cout << "--------------------------------------------" << endl;
    cout << " test chainwork calcs" << endl;
    cout << " hex(2**48) == "<< "0x1000000000000" << " == 281,474,976,710,656 " << endl;
    //cout << " 1.0 / 0x1000000000000 = " <<  t_Res.ToString() << endl;

    t_Res.SetHex( "0x1000000000000");
    t_Res = t_Res / 0xFFFF ;
    cout << " 0x1000000000000 / 0xFFFF = " <<  t_Res.ToString() << endl;

    t_Res.SetHex("0x100010001");
    t_Res *= 0xFFFF;
    cout << " 0x100010001 * 0xFFFF = " << t_Res.ToString() << endl;

    t_Res.SetHex("0x1000000000000");
    t_Res -= 0xffffffffffff;
    cout << " 0x1000000000000 - (0x100010001 * 0xFFFF) = " << t_Res.ToString() << endl;
    cout << endl;

    //------------------------------------------
    uint32_t  cBits_655171 = 0x170e134e;

    bool isNeg, isOvr;
    t_Res.SetCompact( cBits_655171, &isNeg, &isOvr);
    cout << "------------------"<<endl;
    cout << " cBits_655171 = 0x170e134e = " << t_Res.ToString() << endl;
    t_Res = (~t_Res / (t_Res + 1)) + 1;
    cout << " (~t_Res / (t_Res + 1)) + 1 = " << t_Res.ToString() << endl;
    cout <<endl;

    return 0;
}
/*
 * Monday work-session with Thomas .. looking all over for the chainwork calcs in both go and c++
 *       found this:
 *   arith_uint256 GetBlockProof(const CBlockIndex& block)   {
        arith_uint256 bnTarget;
        bool fNegative;
        bool fOverflow;
        bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
        if (fNegative || fOverflow || bnTarget == 0)
             return 0;
        // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
        // as it's too large for an arith_uint256. However, as 2**256 is at least as large
        // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
        // or ~bnTarget / (bnTarget+1) + 1.
        return (~bnTarget / (bnTarget + 1)) + 1;
}
 */

//-------------------------
int do_test_GetCompact() {
    //------------------------------------------------------------------------------------------
    const char *sBITS_inputA = { "0xbe71317c8f406b7e2f5a6d1e495c0d384afc2739eb1628da0517c8f4" };
    arith_uint256   arithBITS_A = UintToArith256( uint256S( sBITS_inputA ));
    uint32_t cBITS_resA = arithBITS_A.GetCompact( 0 );
    arith_uint256   arithBITS_B;
    arithBITS_B.SetCompact( cBITS_resA );
    //Reg# 17   bits_hex: 0x1d00be71  | 0x1d  0xbe71
    //0xbe710000000000000000000000000000000000000000000000000000
    if (is_verbose) {
        cout<<"----- uint256 convert --"<<endl;
        cout<<"   str chars to uint256, compress cBITS, expand to uint256"<<endl;
        std::cout<<hex<<" sBITS_inputA         "<<sBITS_inputA<<endl;
        std::cout<<hex<<"  arithBITS_A 0x"<<arithBITS_A.GetHex()<<endl;
        std::cout<<hex<<"   cBITS_resA     0x"<<cBITS_resA<<endl;
        std::cout<<hex<<"  arithBITS_B 0x"<<arithBITS_B.GetHex()<<endl<<endl;
    }

    const char *sBITS_inputB = { "0x8cc30f97a647313fe0cad97a647313fe0cad97a647313fe0cad97a64" };
    arithBITS_B = UintToArith256( uint256S( sBITS_inputB ));
    uint32_t cBITS_resB = arithBITS_B.GetCompact( 0 );
    //Reg# 18   bits_hex: 0x1d008cc3  | 0x1d  0x8cc3
    //0x8cc30000000000000000000000000000000000000000000000000000
    if (is_verbose) {
        cout<<"----- uint256 convert --"<<endl;
        cout<<"--  str chars to uint256, compress cBITS"<<endl;
        std::cout<<std::hex<<" sBITS_inputB         "<<sBITS_inputB<<std::endl;
        std::cout<<std::hex<<"  arithBITS_B 0x"<<arithBITS_B.GetHex()<<std::endl;
        std::cout<<std::hex<<"   cBITS_resB     0x"<<cBITS_resB<<std::endl;
    }

    const char *sBITS_inputC = { "0x654657a76a76a76a76a76a76a76a76a76a76a76a76a76a76a76a76a7" };
    arith_uint256   arithBITS_C = UintToArith256( uint256S( sBITS_inputC ));
    uint32_t cBITS_resC = arithBITS_C.GetCompact( 0 );
    //Reg# 19   bits_hex: 0x1c654657  | 0x1c  0x654657
    //0x65465700000000000000000000000000000000000000000000000000
    if (is_verbose) {
        std::cout<<std::hex<<" sBITS_inputC         "<<sBITS_inputC<<std::endl;
        std::cout<<std::hex<<"  arithBITS_C 0x"<<arithBITS_C.GetHex()<<std::endl;
        std::cout<<std::hex<<"   cBITS_resC      0x"<<cBITS_resC<<std::endl;
    }

    const char *sBITS_inputD = { "0x43b3e53670cd733d9a400a670cd733d9a400a670cd733d9a400a670c" };
    arith_uint256   arithBITS_D = UintToArith256( uint256S( sBITS_inputD ));
    uint32_t cBITS_resD = arithBITS_D.GetCompact( 0 );
    //Reg# 20   bits_hex: 0x1c43b3e5  | 0x1c  0x43b3e5
    //0x43b3e500000000000000000000000000000000000000000000000000
    if (is_verbose) {
        std::cout<<std::hex<<" sBITS_inputD         "<<sBITS_inputD<<std::endl;
        std::cout<<std::hex<<"  arithBITS_D 0x"<<arithBITS_D.GetHex()<<std::endl;
        std::cout<<std::hex<<"   cBITS_resD      0x"<<cBITS_resD<<endl<<endl;
    }

    // ex from cBITS whitepaper
    //const char *sBITS_inputW = { "0x000000008cc30f97a647313fe0cad97a647313fe0cad97a647313fe0cad97a64" };
    const char *sBITS_inputW = { "0x0000000128a0e4b1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1fb1" };
    arith_uint256   arithBITS_W = UintToArith256( uint256S( sBITS_inputW ));
    uint32_t cBITS_resW = arithBITS_W.GetCompact( 0 );
    arithBITS_W.SetCompact(cBITS_resW);
    if (is_verbose) {
        std::cout<<std::hex<<" sBITS_inputW "<<sBITS_inputW<<std::endl;
        std::cout<<std::hex<<"  arithBITS_W 0x"<<arithBITS_W.GetHex()<<std::endl;
        std::cout<<std::hex<<"   cBITS_resW     0x"<<cBITS_resW<<endl<<endl;
    }


    cout<<"----- uint256 convert --"<<endl;
    cout<<"--  Show coversions of a constant value with increasing left-shift"<<endl;
    cout<<"--  0xFACE00FF << (32+64+64+64)"<<endl;
    cout<<"--  0xface00ff00000000"<<endl;
    cout<<"--  0xface00ff000000000000000000000000"<<endl;
    cout<<"--  0xface00ff0000000000000000000000000000000000000000"<<endl;
    cout<<"--  0xface00ff00000000000000000000000000000000000000000000000000000000"<<endl<<endl;

    //  0x000000008cc30f97a647313fe0cad97a647313fe0cad97a647313fe0cad97a64

    const char *sBITS_inputF0 = { "0xface00ff00000000" };
    arith_uint256   arithBITS_F0 = UintToArith256( uint256S( sBITS_inputF0 ));
    uint32_t cBITS_resF0 = arithBITS_F0.GetCompact( 0 );
    arithBITS_F0.SetCompact(cBITS_resF0);
    if (is_verbose) {
        std::cout<<std::hex<<" sBITS_inputF0     "<<sBITS_inputF0<<std::endl;
        std::cout<<std::hex<<"   cBITS_resF0  0x"<<cBITS_resF0<<endl;
        std::cout<<std::hex<<"  arithBITS_F0 0x"<<arithBITS_F0.GetHex()<<std::endl;
    }

    const char *sBITS_inputF1 = { "0xface00ff000000000000000000000000" };
    arith_uint256   arithBITS_F1 = UintToArith256( uint256S( sBITS_inputF1 ));
    uint32_t cBITS_resF1 = arithBITS_F1.GetCompact( 0 );
    arithBITS_F1.SetCompact(cBITS_resF1);
    if (is_verbose) {
        std::cout<<std::hex<<" sBITS_inputF1     "<<sBITS_inputF1<<std::endl;
        std::cout<<std::hex<<"   cBITS_resF1 0x"<<cBITS_resF1<<endl;
        std::cout<<std::hex<<"  arithBITS_F1 0x"<<arithBITS_F1.GetHex()<<std::endl;
    }

    const char *sBITS_inputF2 = { "0xface00ff0000000000000000000000000000000000000000" };
    arith_uint256   arithBITS_F2 = UintToArith256( uint256S( sBITS_inputF2 ));
    uint32_t cBITS_resF2 = arithBITS_F2.GetCompact( 0 );
    arithBITS_F2.SetCompact(cBITS_resF2);
    if (is_verbose) {
        std::cout<<std::hex<<" sBITS_inputF2     "<<sBITS_inputF2<<endl;
        std::cout<<std::hex<<"   cBITS_resF2 0x"<<cBITS_resF2<<endl;
        std::cout<<std::hex<<"  arithBITS_F2 0x"<<arithBITS_F2.GetHex()<<std::endl;
    }

    const char *sBITS_inputF3 = { "0xface00ff00000000000000000000000000000000000000000000000000000000" };
    arith_uint256   arithBITS_F3 = UintToArith256( uint256S( sBITS_inputF3 ));
    uint32_t cBITS_resF3 = arithBITS_F3.GetCompact( 0 );
    arithBITS_F3.SetCompact(cBITS_resF3);
    if (is_verbose) {
        std::cout<<std::hex<<" sBITS_inputF3     "<<sBITS_inputF3<<endl;
        std::cout<<std::hex<<"   cBITS_resF3 0x"<<cBITS_resF3<<endl;
        std::cout<<std::hex<<"  arithBITS_F3 0x"<<arithBITS_F3.GetHex()<<endl<<endl;
    }


    //-------------------------------------------------------------------------
    arith_uint256  tat256;
    uint32_t  cBits_647136 = 0x17103a12;
    uint32_t  cBits_647136_r;
    tat256.SetCompact( cBits_647136  );
    cBits_647136_r = tat256.GetCompact();
    if (is_verbose) {
        cout<<"----- cBITS convert --"<<endl;
        std::cout<<std::hex<<"  cBits_647136 0x"<<cBits_647136<<endl;
        std::cout<<std::hex<<"        tat256 0x"<<tat256.GetHex()<<endl;
        std::cout<<std::hex<<"cBits_647136_r 0x"<<cBits_647136_r<<endl<<endl;
    }

    //-------------------------------------------------------------------------
    uint32_t  cBits_MAX = 0x1d80ffff;
    arith_uint256 cBits_MAX_r;
    tat256.SetCompact( cBits_MAX  );
    cBits_MAX_r = tat256.GetCompact();
    if (is_verbose) {
        cout<<"----- cBITS MAX --"<<endl;
        std::cout<<std::hex<<"  cBits_MAX 0x"<<cBits_MAX<<endl;
        std::cout<<std::hex<<"     tat256 0x"<<tat256.GetHex()<<endl;
        std::cout<<std::hex<<"cBits_MAX_r 0x"<<cBits_MAX_r.ToString()<<endl<<endl;
    }

    //-------------------------------------------------------------------------
    uint32_t  cBits_EXT = 0x2A84ffff;
    arith_uint256 cBits_EXT_r;
    tat256.SetCompact( cBits_EXT  );
    cBits_EXT_r = tat256.GetCompact();
    if (is_verbose) {
        cout<<"----- cBITS EXT --"<<endl;
        std::cout<<std::hex<<"  cBits_EXT 0x"<<cBits_EXT<<endl;
        std::cout<<std::hex<<"     tat256 0x"<<tat256.GetHex()<<endl;
        std::cout<<std::hex<<"cBits_EXT_r 0x"<<cBits_EXT_r.ToString()<<endl<<endl;
    }

    //-------------------------------------------------------------------------
    uint32_t  cBits_ZER = 0x00000000;
    arith_uint256 cBits_ZER_r;
    tat256.SetCompact( cBits_ZER  );
    cBits_ZER_r = tat256.GetCompact();
    if (is_verbose) {
        cout<<"----- cBITS ZER --"<<endl;
        std::cout<<std::hex<<"  cBits_ZER 0x"<<cBits_ZER<<endl;
        std::cout<<std::hex<<"     tat256 0x"<<tat256.GetHex()<<std::endl;
        std::cout<<std::hex<<"cBits_ZER_r 0x"<<cBits_ZER_r.ToString()<<endl<<std::endl;
    }

    //-------------------------------------------------------------------------
    const string str_maxtarg = "0xffff0000000000000000000000000000000000000000000000000000";
    string str_tmaxt;
    arith_uint256  u256_maxtarg;
    uint32_t u32_maxtarg;
    uint64_t u64_maxtarg, u64_low64;
    double   d_maxtarg;
    bool     isNeg;

    u256_maxtarg.SetHex( str_maxtarg );
    str_tmaxt = u256_maxtarg.GetHex();
    d_maxtarg   = u256_maxtarg.getdouble();
    u64_low64 = u256_maxtarg.GetLow64();
    u64_maxtarg = (uint64_t)d_maxtarg;
    u32_maxtarg = u256_maxtarg.GetCompact(&isNeg);
    if (is_verbose) {
        cout<<"----- MAX Target --"<<endl;
        cout<<hex<<"  u256_maxtarg 0x"<<u256_maxtarg.ToString()<<endl;
        cout<<"  u256_maxtarg 0x"<<str_tmaxt<<"  GetHex()"<<endl;
        cout <<"     d_maxtarg "<<d_maxtarg<<"  getdouble()"<<endl;
        cout<< std::fixed << std::setw(11)<<"     d_maxtarg "<<d_maxtarg<<endl;
        cout<<"  #u32_maxtarg = u256_maxtarg.GetCompact(&isNeg)  // c++11 arith_uint256 "<<endl;
        cout<<hex<<"   u32_maxtarg cBITS 0x"<<u32_maxtarg<<"     bool isNeg "<<isNeg<<endl;
        cout <<"  long_maxtarg "<<hex<<(long)d_maxtarg<<"  (long)d_maxtarg cast"<<endl;
        cout <<"  uint64_maxtarg "<<hex<<u64_maxtarg<<endl;
        cout<<hex<<"   u64_maxtarg GetLow64() 0x"<<u64_low64<<endl<<endl;
    }

    return 0;

    //Reg# 20   bits_hex: 0x1c43b3e5  | 0x1c  0x43b3e5
    //0x43b3e500000000000000000000000000000000000000000000000000

}

int do_TwoBlocks_Compact() {
    /*
 * from   https://en.bitcoin.it/wiki/Difficulty#How_is_difficulty_stored_in_blocks.3F
 *      difficulty = genesis target / current_target
 *      genesis target = unpack_uint256 ( 0x1d00ffff )
 *
 *    in general:
 *      next_difficulty = (previous_difficulty * 2016 * 10 minutes) / (time to mine last 2016 blocks)
 *
 *       Princeton Bitcoin Book   page 134
 */

    //-------------------------------------------------------------------------
    // convert BTC Block 655171 difficulty from String to C++ class
    // extract short format TARGET from the same block, convert, compare
    //
    // NOTE storing in uint256 or arith_uint256 does not change the value
    //  choose arith_uint256 for divide, multiply and other operators

    const char *bMAX_DIFFICULTY = { "0x00000000FFFF0000000000000000000000000000000000000000000000000000" };
    arith_uint256   bd00_MAX = UintToArith256( uint256S( bMAX_DIFFICULTY ));

    // block hash
    const char *bh_655171 = { "0x0000000000000000000073280c6c10e891cff6ac303d1b0d1a13bf661e01ca88" };
    // declared difficulty in long format
    const char *dfS_655171 = { "0x122ffe1bc04e" };
    //  BITS format TARGET for Block 655171
    uint32_t  cBits_655171 = 0x170e134e;
    bool negB, ovrB;

    uint256         bd00_655171 = uint256S( dfS_655171 );
    arith_uint256   bd01_655171 = UintToArith256( uint256S( dfS_655171 ));
    arith_uint256   bd05_655171;

    if (is_verbose) {
        cout<<"----- TwoBlocks_Compact --"<<endl;
        std::cout <<std::hex<< " cBITS_655171  0x" << cBits_655171 <<std::endl;
        std::cout << bh_655171 << " # Block Hash " <<std::endl;
        std::cout << "0x" << bd00_655171.ToString() << " # difficulty (uint256)  " <<std::endl;
        std::cout << "0x" << bd01_655171.ToString() << " # difficulty (arith_uint256) " <<std::endl;
    }
    bd05_655171.SetCompact( cBits_655171, &negB, &ovrB );  // demonstrate BITS compact conversion
    // TODO check return flags for unexpected values
    if (is_verbose)
        std::cout << "0x" << bd05_655171.ToString() << " # expanded SetCompact( cBits_655171) " <<std::endl;

    arith_uint256   bd03_655171(  bd00_MAX / bd05_655171 );
    if (is_verbose)
        std::cout << "0x" << bd03_655171.ToString() << " #   target calc MAX/cur " <<std::endl<<std::endl;

    //-----------------------------------------------------------
    // Convert a block hash, from block 100,000. to arith_uint256
    const char *b100 = { "0x000000000003ba27aa200b1cecaad478d2b00432346c3f1f3986da1afd33e506" };
    arith_uint256  v00_100000 = UintToArith256( uint256S( b100 ));
    uint32_t       cBits_100000 = 0x1b04864c;
    const char    *d100 = {"0x1b04864c"};
    arith_uint256  v04_100000;
    arith_uint256  v01_100000  = UintToArith256( uint256S( d100 ));

    v01_100000.SetCompact( cBits_100000, &negB, &ovrB );  // demonstrate BITS compact conversion

    if (is_verbose) {
        std::cout <<std::hex<< " cBITS_100000  0x" << cBits_100000 <<std::endl;
        std::cout << "0x" << v00_100000.ToString() << " # Block Hash  arith_uint256" <<std::endl;
        std::cout << "0x" << v01_100000.ToString() << " # TARGET arith_uint256 from cBITS" <<std::endl;
    }
    arith_uint256       bd02_100000 = bd00_MAX / v01_100000;
    if (is_verbose)
        std::cout << "0x" << bd02_100000.ToString() << " # difficulty (MAX/cur) " <<std::endl;

    if (is_verbose)
        std::cout << bMAX_DIFFICULTY << " # MAX difficulty  " <<std::endl<<std::endl;


    //arith_uint256 cBitsCnv_100000 = bd02_100000.GetCompact(false );
    //if (is_verbose)
    //    std::cout << "0x" << cBitsCnv_100000.ToString() << " # difficulty (uint256)" <<std::endl<<std::endl;

    return 0;
}

//--------------------------------------------------------------------------------------
#define OneL     0x10000001L
#define FourLLU  4LLU           // show unsigned long long type

int do_sizesTest() {
    // initializing BTC types with a constant expression is not easy
    //  due to the complexity of the C++ base class definitions
    unsigned long resLong;
    uint32_t res32;
    const arith_uint256 aoHL = (OneL << 12);
    const unsigned long long afLLU = (FourLLU << 2);

    uint256  ui256 ;                   // class uint256 : 32 bytes uchar
    arith_uint256 tres256;             // class arith_uint256 : 8 unsigned longs

    tres256 = UintToArith256( ui256 ); // show a trivial all-zero constant, converted
    if (is_verbose) {
        cout<<"----- sizes --"<<endl;
        std::cout << "0x" << ui256.ToString() << " #  uninitialized (uint256)"<<std::endl;
        std::cout << "0x" << tres256.ToString() << " # uninitialized (arith_uint256)"<<std::endl;
    }

    // size of basic types, in bytes
    int res0 = sizeof(resLong);   // unsigned long long   8 bytes
    int res1 = sizeof(res32);   // unsigned long long   8 bytes
    int resA = sizeof(afLLU);   // unsigned long long   8 bytes
    int resB = sizeof(ui256);   // class data blob 32 bytes
    int resC = sizeof(aoHL);    // class data blob 32 bytes
    int resD = sizeof(tres256); // class  32 bytes
    uint64_t res64 = std::numeric_limits<uint64_t>::max();

    if (is_verbose) {
        double ad_epsilon = std::numeric_limits<double>::epsilon();
        cout<<"sizeof(uint64_t) "<<sizeof(uint64_t)<<"    sizeof(double) "<<sizeof(double)<<endl;
        cout << " unsigned long 0x" << res0 << "    uint32_t 0x" << res1<< endl;
        cout << "uint64_t MAX 0x" << res64<< endl;
        cout << "sizeof(afLLU) unsigned long long " << resA<< endl;
        cout << "sizeof(ui256) uint256         0x" << resB<< endl;
        cout << "sizeof(aoHL)    arith_uint256 0x" << resC<<"  "<< aoHL.GetHex()<<endl;
        cout << "sizeof(tres256) arith_uint256 0x" << resD<< endl<< endl;
    }
        return 0;
}


int do_container_test() {
    {
        std::vector<const char*> cats {"Heathcliff", "Snagglepuss", "Hobbes", "Garfield"};
        std::sort(cats.begin(), cats.end(), [](const char *strA, const char *strB) {
            return std::strcmp(strA, strB) < 0;
        });

        for (const char *cat : cats) {
            std::cout << cat << '\n';
        }
    }
    return 0;
}
 /*
  *  oe_uint256  ELF executable, debug build
  *    no external libraries other than runtime C++
  *
    thartman_2020/srcs_thartman/oe_uint256$    ldd  oe_uint256
       libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007f57c8c96000)
       libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f57c8a7e000)
       libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f57c86b4000)
       libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f57c83ab000)
       linux-vdso.so.1 =>  (0x00007ffd16edd000)
       /lib64/ld-linux-x86-64.so.2 (0x00007f57c9079000)

     oe_uint256    -rwxrwxr-x 1 dbb dbb   339504 bytes    Nov 10 19:05
  */
