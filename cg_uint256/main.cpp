#include <cstdint>

#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "crypto/common.h"
#include "prevector.h"

#include "hash.h"
#include "uint256.h"
#include "arith_uint256.h"


/*
 *    cg_uint256
 *
 *    05jan21  -dbb
 *    04feb21  -dbb      var names rewrite
 *
 *
 *    A C++ command line tool that imports  bitcoind  math directly
 *
 *    Prove that chainwork in each input line is determined by supplied formulas,
 *     else emit ERR_strings
 *
 *
 * ************************************************

  Discussion of Series Chainwork:

  the chainwork series is computed as:

    new_chainwork( prev_chainwork, target )
     = prev_chainwork + ( 2**256 / (target + 1 ) )

    Haskell notation:
               new_chainwork ::
                Integer ->
                Integer ->
               Integer

   target >= 1 and target <= (0xffff * 2**208)

  --
  each mining iteration defines a "current target" aka Difficulty Epoch   locally (REGIMEN)
  Chainwork changes with new TARGET values per Epoch
  The chainwork series accumulates a running total by block height

 */

using namespace std;

arith_uint256  cwMAXTARG;          // (0xFFFF*pow(2,208) )
arith_uint256  cwMAX256;           //  2**256 -1
arith_uint256  cwPOW208;           //  pow( 2, 208 );  (2**208)


// MAX_TARGET
//  0xFFFF *  0x10000000000000000000000000000000000000000000000000000L
//
//  [chainwork value] = [previous chainwork value] + [difficulty] * [0x100010001]
//
//     chainwork =  prev_chainwork +  D * (2**48 / 0xffff)
//             D =  MAX_TARGET / current_target  (nBITS)

arith_uint256  cwChainworkAmount;  // (2**48 / 0xffff) ;  0x100010001 + 1/0xFFFF


const bool is_verbose = true;

//-***********************************************************************
//  ChainStruct      C-style container for intermediate and final calcs
//
//   ind        index or blockHeight of the current row
//   cBITS      compactBITS target difficulty (input)
//   cwTargetValue    current_target  (decoded cBITS)
//   cwComputedValue  computed value of chainwork for current row
//   cwRefCW          chainwork of this row  (input)
//   cwDiff           chainwork delta from previous row to current row

struct ChainStruct  {
    uint32_t        ind={0}, cwBITS={0};
    arith_uint256   cwTargetValue, cwComputedValue;
    arith_uint256   cwRefCW,   cwDiff;
};

/*  Parse a reference CSV input like this:
 *
 *    ind  |   nBITS    |  chainwork   |  chainwork diff
 *  -------+------------+--------------+-------------
 *   32768 | 0x1d00b332 |  0xdecade    |  0x16db84926
 *
 * --
 *  from  /sw_op_energy/src/branch/master/chainwork_dec2020.md
 *
    (    2, '0x1d00ffff',    '0x300030003', '0x100010001')
    (    3, '0x1d00ffff',    '0x400040004', '0x100010001')
    (    4, '0x1d00ffff',    '0x500050005', '0x100010001')
    (    5, '0x1d00ffff',    '0x600060006', '0x100010001')
    (    6, '0x1d00ffff',    '0x700070007', '0x100010001')
    (32256, '0x1d00d86a', '0x7e01acd42dd2', '0x12ed3afd2')
    (32257, '0x1d00d86a', '0x7e02dba7dda4', '0x12ed3afd2')
    (32258, '0x1d00d86a', '0x7e040a7b8d76', '0x12ed3afd2')
    (32259, '0x1d00d86a', '0x7e05394f3d48', '0x12ed3afd2')
    (32260, '0x1d00d86a', '0x7e066822ed1a', '0x12ed3afd2')
    (34272, '0x1d00c428', '0x87528f22ef20', '0x14e19db60')
    (34273, '0x1d00c428', '0x8753dd3cca80', '0x14e19db60')
    (34274, '0x1d00c428', '0x87552b56a5e0', '0x14e19db60')
    (34275, '0x1d00c428', '0x875679708140', '0x14e19db60')
    (34276, '0x1d00c428', '0x8757c78a5ca0', '0x14e19db60')

 */

//---------------------------------------------------------------------------
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

    res_u256 = (b_res256 / (bnTarg+1)) + 1;
    //res_u256 = (b_res256 / (cwMAXTARG+1)) + 1;

    if ( is_verbose)
        cout<< "        res_u256 0x"<<res_u256.ToString()<<endl;

    return res_u256;
}

//--------------------------------------------------------------------------
ChainStruct do_parse(string in_line) {
    ChainStruct chain1;
    string indStr, factorStr, accumStr, diffStr;
    size_t pos1, pos2, pos3;

    // hard-code the '|' separator value and order for convenience
    pos1 = in_line.find('|');
    pos2 = in_line.find('|', pos1 + 1);
    pos3 = in_line.find('|', pos2 + 1);
    indStr = in_line.substr(0, pos1);
    factorStr = in_line.substr(pos1 + 1, pos2 - pos1 - 1);
    accumStr = in_line.substr(pos2 + 1, pos3 - pos2 - 1);
    diffStr = in_line.substr(pos3 + 1);

    // discard a trailing 'L'  in accumStr and diffStr
    pos1 = accumStr.find('L');
    if (pos1 < accumStr.length())
        accumStr.erase(pos1);
    pos1 = diffStr.find('L');
    if (pos1 < diffStr.length())
        diffStr.erase(pos1);

    // https://en.cppreference.com/w/cpp/string/basic_string/stol
    try {
        chain1.ind = stoi(indStr, nullptr, 0);
    } catch (...) {
        cerr << "except " << in_line << endl;
        exit(-1);
    }

    try {
        chain1.cwBITS = stoi(factorStr, nullptr, 0);
        bool fNegative, fBoolean;
        chain1.cwTargetValue.SetCompact(chain1.cwBITS, &fNegative, &fBoolean );
    } catch (...) {
        cerr<<" stoi EXCEPT "<<factorStr<<endl;
        exit(-1);
    }

    try {
        chain1.cwRefCW = arith_uint256( accumStr);
        chain1.cwDiff  = arith_uint256( diffStr);
    } catch (...) {
        cerr << "except " << in_line << endl;
        exit(-1);
    }

    if (is_verbose) {
        cout << dec << "-- chain "<<chain1.ind<<" : " << endl;
        cout << "    cBITS  0x" << hex << chain1.cwBITS << endl;
        cout << "    cwDiff 0x" << hex << chain1.cwDiff.ToString() << endl;
        cout << "   cwTargetValue 0x" << hex << chain1.cwTargetValue.ToString() << endl<< endl;
        //cout << " cwComputedValue 0x" << hex << chain1.cwComputedValue.ToString() << endl;
        //cout << "      dcomp " << hex << chain1.dcomp << endl;
        //cout << "    dfactor " << std::fixed << std::setw(11) << chain1.dfactor << endl;
        cout.flush();
    }

    return chain1;
}

//-------------------------------------------------------------------------------------------
void init_constants() {
    cwMAXTARG.SetHex(        "0xffff0000000000000000000000000000000000000000000000000000" );

    cwPOW208.SetHex(            "0x10000000000000000000000000000000000000000000000000000" );
    //                          (0xFFFF*pow(2,208) );  // ( 16**52) == ( 2**208 )

    cwMAX256.SetHex( "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff" );
    //                          (pow( 2, 256 ));  // ( 2**256 )  //  (1 << 256)
    //cwMAX256.SetHex( "0x1000000000000000000000000000000000000000000000000000000000000000" );

    cwChainworkAmount.SetHex( "0x100010001");
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


//============================================================================
int main(int argc,char **argv)
{
    int ind_loop, m;
    int fail_cnt = 0;
    int cnt_grps = 0;
    ChainStruct chgrp[5];

    ifstream file;
    string p_line;

    if (argc==1)
        cerr<<"Usage: chaingrease file\n";
    else
        file.open(argv[1]);

    init_constants();

    while (file.good())
    {   // parse five lines into chain struct
        for (ind_loop=m=0;ind_loop<5;ind_loop++)
        {
            getline(file,p_line);
            if (p_line.length())
            {
                m++;
                chgrp[ind_loop] = do_parse(p_line);
            }
        }
        if (m==0)
            break;

        for (ind_loop=0;ind_loop<5;ind_loop++)
        {
            std::string tStr;
            arith_uint256    chainworkDelta_u256;

            chainworkDelta_u256 = calc_chainwork_delta( chgrp[ind_loop].cwBITS );
            chgrp[ind_loop].cwComputedValue = chainworkDelta_u256;

            if (chgrp[ind_loop].cwComputedValue == chgrp[ind_loop].cwDiff ) tStr = "MATCH";
             else { tStr = "FAIL"; fail_cnt++; }

            if (ind_loop > 1) continue;  // skip some diagnostics
            if (  is_verbose || tStr.compare( "FAIL") == 0  ) {
                cout << "--" << endl;
                cout << dec <<tStr<< " regimens parsed: " << cnt_grps << " index " <<chgrp[ind_loop].ind<< " diff " <<tStr<< endl;
                cout << hex << "      0x" << chgrp[ind_loop].cwDiff.ToString() << " # from input data"<<endl;
                if (tStr.compare( "FAIL") == 0 ) {
                    cout << hex << "      0x" << chgrp[ind_loop].cwComputedValue.ToString() << " # computed value" << endl;
                    cout << hex << "      0x" << (chgrp[ind_loop].cwDiff - chgrp[ind_loop].cwComputedValue).ToString() << " # ... missed by " << endl;
                    cout << hex << "      0x" << chgrp[ind_loop].cwBITS  << " # cBITS " << endl;
                } else {
                    cout << hex << "      0x" << chgrp[ind_loop].cwComputedValue.ToString() << " # computed value" << endl;
                }
                cout << "" << endl;
            }
        }

        for (ind_loop=1;ind_loop<5;ind_loop++) {
            if (chgrp[ind_loop].cwRefCW != chgrp[ind_loop - 1].cwRefCW + chgrp[ind_loop].cwDiff)
                cerr << "index " << chgrp[ind_loop].ind << " accumulators differ by wrong number\n";
        }
        cnt_grps++;
        cout.flush();
        cerr.flush();
    }
    cout<<endl;
    cout<<dec<<cnt_grps<<" regimen groups parsed; "<<fail_cnt<<" FAIL results\n\n";

    return 0;
}


//  EOF

