#include <iostream>
#include <cstdint>
#include <string>
#include <exception>

#include "crypto/common.h"

#include "hash.h"
#include "uint256.h"
#include "arith_uint256.h"

char gdata[12];
const char *bMAX_DIFFICULTY = { "0x00000000FFFF0000000000000000000000000000000000000000000000000000" };

const bool is_verbose = false;

using namespace std;

arith_uint256  cwMAXTARG;          // (0xFFFF*pow(2,208) )
arith_uint256  cwMAX256;           //  2**256 -1
arith_uint256  cwPOW208;           //  pow( 2, 208 );  (2**208)

//---------------------------------------------------------------------------
//  v01 from  cg_uint256/main.cpp   march 2021  -dbb
//
arith_uint256 calc_chainwork_delta( uint32_t targ_nBits )
{
    /*
     *  calc_chainwork_delta
     *    input a compact BITS target value
     *    return arith_uint256 of the expected chainwork via formula
     */

    arith_uint256  res_u256,  bnTarg,  b_res256;

    // use the algebraic identity
    //   For all a, for all b!=0, a/b=(a-b)/b+1
    //
    // comment from bitcoind core :
    //   (2**256) / (val + 1)   ==
    //  ((2**256 - val - 1) / (val + 1)) + 1  ==
    //  (~val / (val+1)) + 1

    bool fNegative, fOverflow;
    bnTarg.SetCompact( targ_nBits, &fNegative, &fOverflow );

    b_res256 =  ~bnTarg;  // bitwise negation, AKA one's compliment

    if ( is_verbose) {
        cout<< "      --------------------------------------------"<<endl;
        cout<< "          bnTarg 0x"<<bnTarg.ToString()<<endl;
        cout<< "        b_res256 0x"<<b_res256.ToString()<<endl;
        cout<< "       cwMAXTARG 0x"<<cwMAXTARG.ToString()<<endl;
    }

    //res_u256 = (b_res256 / (cwMAXTARG+1)) + 1;
    res_u256 = (b_res256 / (bnTarg+1)) + 1;

    if ( is_verbose)
        cout<< "        res_u256 0x"<<res_u256.ToString()<<endl;

    return res_u256;
}


//-------------------------------------------------------------------------------------------
void init_constants() {
    cwMAXTARG.SetHex(        "0xffff0000000000000000000000000000000000000000000000000000" );

    cwPOW208.SetHex(            "0x10000000000000000000000000000000000000000000000000000" );
    //                          (0xFFFF*pow(2,208) );  // ( 16**52) == ( 2**208 )

    cwMAX256.SetHex( "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" );
    //                          (pow( 2, 256 ));  // ( 2**256 )  //  (1 << 256)
    //cwMAX256.SetHex( "0x1000000000000000000000000000000000000000000000000000000000000000" );

    //cwChainworkAmount.SetHex( "0x100010001");
    //cwUNIT0 = cwMAX256 / cwTARG0;  //( cwMAX256 / cwTARG0 );


    if (is_verbose) {
        cout<<"---------------------------------------------------------"<<endl;
        cout<<"  Initialize CONSTANTS "<<endl;
        cout<< hex << " 0x"<<cwMAXTARG.ToString() << "  cwTARG0" << endl;
        cout<< hex << " 0x"<< cwPOW208.ToString() << "  cwPOW208" << endl;
        cout<< hex << " 0x"<< cwMAX256.ToString() << "  cwMAX256" << endl;
        //cout<< hex << " 0x"<<cwChainworkAmount.ToString() << "  cwChainworkAmount" << endl;
        cout<< endl;
    }

    return;
}

int test_bad_div() {
    // test exception handling in c++11
    int badDivResult;
    try {
        badDivResult = 5 - (4+1);
        badDivResult = 1.0 / 0;
    }
    catch (const std::exception& e) {
        //return error("%s: Deserialize or I/O error - %s at %s", __func__, e.what(), 'badDivResult');
        cerr<< __func__ << "divide caught " << e.what() ;
        return 0;
    }

    cout<< "badDivResult " << badDivResult;
    return 0;
}

//===============================================================================

int count_p(char* in_ptr)
// p is assumed to point to a zero-ter minated array of char (or to nothing)
{
    if ( in_ptr==nullptr) return 0;
    int count = 0;
    for ( ; in_ptr!=nullptr ; ++in_ptr)
        ++count;
    return count;
}

//-------------------------------------------------
int parse(int argc, char** argv )
{
    //char *tptr  = argc > 1 ? argv[1] : (char*)"0x00100010\0";
    char *tptr;

    if  (argc > 1) {
        tptr = argv[1];
        if (tptr[0] == '0' && tptr[1] == 'x')
            tptr = &tptr[2];
        strcpy( gdata, tptr );
    }
    else {
        tptr = (char*)"Usage: target value as hex string  \n returns difficulty\0";
        std::cout << tptr << "\n";
    }
    //strncpy( gdata, tptr, strlen( (const char*)tptr)  );

    return strlen(tptr);
}

uint64_t do_t2( uint32_t aIN , uint32_t bIN  )
{
    uint64_t innerConst = { 0x00550000C0000040ULL };
    uint32_t kAr[5] = { 0x11112222,0x66665555,0x99997777,0xAAAA8888,0xBBBB9999, };

    if (bIN != 0 )
        innerConst |= bIN;
    else
        innerConst = kAr[0] | (uint64_t)kAr[1] << 32;

    auto bb = aIN | innerConst << 32;
    return (uint64_t)bb;
}

//-------------------------------
int main(int argc, char** argv)
{
    bool do_union;
    int res_p, testcount, starcount, combcount;
    double factor1, factor2;

    res_p = parse(argc, argv);
    //  a return strlen by convention
    if (res_p > 10 ) {
        exit(1);
    }

    init_constants();
    //-----------------------
    uint64_t bRes;
    uint32_t aIN, bIN, abRes;
    aIN = 0x02000200;
    bIN = 0x0;
    //bIN = 0x82000020;
    //bIN = 0x03;
    bRes = do_t2( aIN, bIN );
    abRes = bRes;
    //----------------------------------

    arith_uint256   bd00_MAX;
    arith_uint256 v01_100000;
    unsigned long tmp_long;
    bool negB, ovrB;

    bd00_MAX = UintToArith256( uint256S( bMAX_DIFFICULTY ));
    tmp_long = std::strtoul( gdata, NULL, 16);
    // the caller must set the values of the bool (?)
    negB = 0;  ovrB = 0;
    v01_100000.SetCompact( tmp_long, &negB, &ovrB );

    arith_uint256   bd03_0 = bd00_MAX / v01_100000;
    std::cout << hex<<"0x" << v01_100000.GetCompact(false)<< "  -  "<<"0x"<<v01_100000.GetHex() << endl;



    return 0;
}


