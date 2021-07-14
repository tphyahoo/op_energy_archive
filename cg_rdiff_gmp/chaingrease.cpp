
#include <cstdint>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <gmpxx.h>

//  chaingrease   -    with GNU Multiprecision arithmetic library
//
//   dec2020
//
//  Parse and compute values from a supplied csv file,  format
//     ind  |   factor   |  accum   |  diff
//
//  Prove that diff in each line is determined by supplied formulas, or emit ERR_strings
//
//  # -- generate data file (rough) --
//  copy (select height_str::integer, bits_hex, chainwork_hex FROM data_chain
//   order by height_str::integer)
//    to '/tmp/tmp_chaingrease.csv' with CSV header delimiter E'\|';
//  ##-  add the diff column using whatever tooling   (see python Notebooks)
//

using namespace std;

const mpz_class cwTARG0(0xFFFF*pow(2,208) );  // ( 16**52) == ( 2**208 ) ## * 0xFFFF
const mpz_class  cwMAX0(pow( 2, 256 ));  // ( 2**256 )
const mpz_class cwUNIT0( cwMAX0 / cwTARG0 );
const mpq_class cwrUNIT( cwMAX0 / cwTARG0 );

const mpq_class floatTolerance("1/100000000000000");
const bool is_verbose = false;

//-***********************************************************************
struct ChainStruct
{
  uint32_t   ind={0}, cBITS={0};
  double     dfactor, dcomp;
  mpz_class  ifactor, icomp;
  mpz_class  accum, rdiff;
};

//-***********************************************************************
//  utils

//--------------------------------
void outFloat(mpq_class x);
mpz_class doubleToBigint(double x);
//--------------------------------
mpz_class uncompress(uint32_t n)
{
  int exp  = n >> 24;
  int mant = n & 0x7FFFFF;
  return  mpz_class(mant) << (8*exp-24);
}
//--------------------------------
/*  Parse a ref input like this:
 *
 *  ind  |   factor   |  accum   |  diff
 * ------+------------+----------+-------------
 * 32768 | 0x1d00b332 | 0xdecade | 0x16db84926
 *
 *   Or like this:
 *  ind  |  factor  |  accum   |  diff
 * ------+----------+----------+-------------
 * 32768 | 1.428571 | 0xdecade | 0x16db84926
 *
 */
ChainStruct do_parse(string in_line)
{
  ChainStruct   chain1;
  string        indStr,factorStr,accumStr,diffStr;
  size_t        pos1,pos2,pos3;

  // hard-code the '|' separator value and order for convenience
  pos1= in_line.find('|');
  pos2= in_line.find('|',pos1+1);
  pos3= in_line.find('|',pos2+1);
  indStr    = in_line.substr(0,pos1);
  factorStr = in_line.substr(pos1+1, pos2-pos1-1);
  accumStr  = in_line.substr(pos2+1, pos3-pos2-1);
  diffStr   = in_line.substr(pos3+1);

  // discard a trailing 'L'  in accumStr and diffStr
  pos1 = accumStr.find('L');
  if ( pos1 < accumStr.length())
      accumStr.erase(pos1);
  pos1=diffStr.find('L');
  if (pos1<diffStr.length())
      diffStr.erase(pos1);

  // https://en.cppreference.com/w/cpp/string/basic_string/stol
  try {
        chain1.ind = stoi(indStr, nullptr, 0);
    } catch(...) {
      cerr<<"except "<<in_line<<endl;
      exit(-1);
  }

  /* SHOW that the factor times a constant equals the diff value.
   *   When the factor is a hex uint32, uncompress using a provided formula
   *     then multiply by a provided ratio  (cBITS compression)
   */

  double tmp_dF, tmp_dMAX0;
  //    detect cBITS data format in the factor column
  if ( factorStr.find_first_of("xX") < factorStr.length() )
  {
    try {
        chain1.cBITS = stoi(factorStr, nullptr, 0);
    } catch (...) {
        cerr<<" stoi EXCEPT "<<factorStr<<endl;
        exit(-1);
    }
    chain1.ifactor = uncompress( chain1.cBITS);
    tmp_dF = chain1.ifactor.get_d();
    chain1.dfactor = tmp_dF;
    tmp_dMAX0 = cwMAX0.get_d();
    chain1.dcomp = tmp_dMAX0 / tmp_dF;
  }
  else  // or, read a float format factor column
  {
    chain1.cBITS=0;
    try {
        tmp_dF = stod( factorStr,nullptr);
    } catch (...) {
        cerr<<" stod EXCEPT "<<factorStr<<endl;
        exit(-1);
    }
    chain1.dfactor = tmp_dF;
    chain1.ifactor= doubleToBigint(tmp_dF);
    chain1.dcomp = cwUNIT0.get_d() * chain1.dfactor;
  }

  chain1.icomp = cwMAX0 / chain1.ifactor;   // rounded down

  // https://gmplib.org/manual/C_002b_002b-Interface-Integers
  chain1.accum = mpz_class( accumStr);
  chain1.rdiff = mpz_class( diffStr);

    if (is_verbose) {
        cout << dec << "-- chain1: " << endl;
        cout << "    cBITS  0x" << hex << chain1.cBITS << endl;
        cout << "    rdiff 0x" << hex << chain1.rdiff << endl;
        cout << "  ifactor 0x" << hex << chain1.ifactor << endl;
        cout << "    icomp 0x" << hex << chain1.icomp << endl;
        cout << "    dcomp 0x" << hex << chain1.dcomp << endl;
        cout << "    dfactor " << std::fixed << std::setw(11) << chain1.dfactor << endl;
    }
  return chain1;
}

//=======================================================================

int main(int argc,char **argv)
{
  int i,m;
  int cnt_grps = 0;
  ChainStruct chgrp[5];

  ifstream file;
  string p_line;

  if (argc==1)
    cerr<<"Usage: chaingrease file\n";
  else
    file.open(argv[1]);

  while (file.good())
  {
    for (i=m=0;i<5;i++)
    {
      getline(file,p_line);
      if (p_line.length())
      {
          m++;
          chgrp[i] = do_parse(p_line);
      }
    }
    if (m==0)
      break;

    for (i=0;i<5;i++)
    {
      std::string tStr;
      if ( chgrp[i].icomp == chgrp[i].rdiff )  tStr = "MATCH";  else  tStr = "FAIL";

      if ( is_verbose || tStr.compare( "FAIL") == 0  ) {
          cout << "--" << endl;
          cout << dec << " index " <<chgrp[i].ind<< " diff " <<tStr<< " factor " <<chgrp[i].dfactor<< endl;
          cout << hex << "      dcomp 0x" << chgrp[i].dcomp << endl;
          cout << hex << "      icomp 0x" << chgrp[i].icomp << endl;
          cout << hex << "      rdiff 0x" << chgrp[i].rdiff << endl;
      }
    }

    for (i=1;i<5;i++) {
        if (chgrp[i].accum != chgrp[i - 1].accum + chgrp[i].rdiff)
            cout << "index " << chgrp[i].ind << " accumulators differ by wrong number\n";
    }
    cnt_grps++;
  }
  cout<<cnt_grps<<" groups parsed\n";

  return 0;
}

//-------------------------------------------------------------
void outFloat(mpq_class x)
/* Outputs a number in the form 1.nnnnnnnnnnnnnnnn*2^ee in hex.
 * Double-precision uses 13 nybbles after the hex point.
 */
{
    int i,n;
    int e=0;
    mpz_class nmp;

    while (x>0.5)
    {
        x/=2;
        e++;
    }
    while (x<1)
    {
        x*=2;
        e--;
    }
    cout.flags(ios::hex);
    for (i=0;i<=16;i++)
    {
        nmp = x.get_num() / x.get_den();
        n = nmp.get_si();
        cout<<n;
        if (i==0)
            cout<<'.';
        x = (x-nmp)*16;
    }
    cout<<" * 2^"<<e;
    cout.flags(ios::dec);
}

mpz_class doubleToBigint(double x)
/* The double number, which is written with not quite enough significant digits,
 * is cwTARG0 divided by an integer which can be expressed as cBITS exactly,
 * i.e. it has less than 24 significant bits. Return the integer.
 */
{
  double tmp_TARG0 = cwTARG0.get_d();
  double y = tmp_TARG0/ x;
  int exponent;
  double mantissa = frexp( y, &exponent);
  mantissa = nearbyint(mantissa*0x1000000)/0x1000000;
  y = ldexp( mantissa, exponent);
  return y;
}

// EOF
