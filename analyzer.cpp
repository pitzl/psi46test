
#include "analyzer.h"

using namespace std;

void DumpData( const vector < uint16_t > &x, unsigned int n )
{
  unsigned int i;
  printf( "\n%i samples\n", int ( x.size(  ) ) );
  for( i = 0; i < n && i < x.size(  ); ++i ) {
    if( x[i] & 0x8000 )
      printf( ">" );
    else
      printf( " " );
    printf( "%03X", ( unsigned int ) ( x[i] & 0xfff ) );
    if( i % 16 == 15 )
      printf( "\n" );
  }
  printf( "\n" );
}

//------------------------------------------------------------------------------
// decode first pixel in each event:

int DecodePixel( const vector < uint16_t > &x, int &pos,
                 PixelReadoutData & pix )
{
  PROFILING;

  pix.Clear(  );
  unsigned int raw = 0;

 // check:

  if( pos >= int ( x.size(  ) ) )
    return 1; // missing data

 // need to skip some junk at the beginning (DP 3.4.2014 at FEC):
 // scan for ROC header:
 // while( pos < int( x.size() ) && (x[pos] & 0x8ffc) != 0x87f8 ) {
 //   cout << "pos " << pos << " skip " << hex << x.at(pos) << dec << endl;
 //   ++pos;
 // }
 // scan for FPGA header:
  while( pos < int ( x.size(  ) ) && ( x[pos] & 0x8000 ) != 0x8000 ) {
    cout << "pos " << pos << " skip " << hex << x.at( pos ) << dec << endl;
    ++pos;
  }

 //cout << " pos " << pos << " size " << x.size() << " ";

  if( pos >= int ( x.size(  ) ) )
    return 2; // wrong header

  pix.hdr = x[pos++] & 0xfff; // 12 bits valid header

  if( pos >= int ( x.size(  ) ) || ( x[pos] & 0x8000 ) )
    return 0; // empty data readout

 // read first pixel:

  raw = ( x[pos++] & 0xfff ) << 12;

  if( pos >= int ( x.size(  ) ) || ( x[pos] & 0x8000 ) )
    return 3; // incomplete data

  raw += x[pos++] & 0xfff;

  pix.p = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
  raw >>= 9;

  if( tb.linearAddress ) {
    pix.y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
    pix.x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
  } // PROC600
  else {

    int c = ( raw >> 12 ) & 7;
    c = c * 6 + ( ( raw >> 9 ) & 7 );

    int r2 = ( raw >> 6 ) & 7;
    int r1 = ( raw >> 3 ) & 7;
    int r0 = ( raw >> 0 ) & 7;

    //if( tb.GetPixelAddressInverted() ) {
    if( tb.invertAddress ) {
      r2 ^= 0x7;
      r1 ^= 0x7;
      r0 ^= 0x7;
    }

    int r = r2 * 36 + r1 * 6 + r0;

    pix.y = 80 - r / 2;
    pix.x = 2 * c + ( r & 1 );

  } // dig

  ++pix.n;

 // read additional noisy pixel:

  int cnt = 0;
  while( !( pos >= int ( x.size(  ) ) || ( x[pos] & 0x8000 ) ) ) {
    cout << "noisy pixel at pos " << pos << endl;
    ++pos;
    ++cnt;
  }

  pix.n += cnt / 2;

  return 0;
}

//------------------------------------------------------------------------------
// DP return all pixels from one ROC in one event

vector < PixelReadoutData > GetEvent( const vector < uint16_t > &x, int &pos,
                                      int &hdr )
{
  PROFILING;

  vector < PixelReadoutData > vpix;

  bool ldb = 0;

  if( ldb )
    cout << "GetEvent at pos " << pos << " in data size " << x.size(  )
      << flush << endl;

 // check:

  if( pos >= int ( x.size(  ) ) ) {
    cout << "analyzer GetEvent error at pos " << pos
	 << " >= x.size " << x.size() << endl;
    throw int ( 1 );            // missing data
  }

 // need to skip some junk at the beginning (DP 3.4.2014 at FEC):

  while( pos < int ( x.size(  ) ) &&
	 ( x[pos] & 0x8ffc ) != 0x87f8 ) { // scan for header
    cout << "analyzer at pos " << pos
	 << ": " << hex << x[pos] << dec << " is not a valid ROC header"
	 << endl;
    ++pos;
  }

  if( ldb ) 
    cout << " start at pos " << pos << " size " << x.size() << " ";

  if( pos >= int ( x.size(  ) ) )
    throw int ( 2 );            // wrong header

  if( ldb )
    cout << "  header at pos " << pos
      << " hex " << hex << x[pos] << dec << flush << endl;

  hdr = x[pos] & 0xfff; // 12 bits valid header

  ++pos;

  if( pos >= int ( x.size(  ) ) )
    return vpix; // empty data readout

  if( ( x[pos] & 0x8000 ) )
    return vpix; // empty event, new event

  PixelReadoutData pix;

  pix.hdr = hdr;
  int cnt = 0;

 //while( pos + 1 < int( x.size() ) && !( x[pos] & 0x8000 ) ) {

  do {
    unsigned int raw = ( x[pos] & 0xfff ) << 12; // 12 ROC bits

    if( pos + 1 >= int ( x.size(  ) ) || ( x[pos] & 0x4000 ) )
      throw int ( 3 );          // incomplete data
    ++pos;
    raw += x[pos] & 0xfff; // 12 more ROC bits

    pix.p = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
    raw >>= 9;

    if( tb.linearAddress ) {
      pix.y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
      pix.x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
    } // PROC600

    else {

      int c = ( raw >> 12 ) & 7;
      c = c * 6 + ( ( raw >> 9 ) & 7 );

      int r2 = ( raw >> 6 ) & 7;
      int r1 = ( raw >> 3 ) & 7;
      int r0 = ( raw >> 0 ) & 7;

      //if( tb.GetPixelAddressInverted() ) {
      if( tb.invertAddress ) {
	r2 ^= 0x7;
	r1 ^= 0x7;
	r0 ^= 0x7;
      }

      int r = r2 * 36 + r1 * 6 + r0;

      pix.y = 80 - r / 2;
      pix.x = 2 * c + ( r & 1 );

    } // dig
    pix.n = cnt;
    vpix.push_back( pix );
    ++cnt;
  }
  while( !( x[pos++] & 0x4000 ) ); // FPGA end marker (token out)

  if( ldb )
    cout << "  pixels returned " << vpix.size(  )
      << flush << endl;

  return vpix;
}

/*
  #define ERROR_RO_EMPTY      0
  #define ERROR_RO_MISSING    1
  #define ERROR_RO_ALIGNMENT  2
  #define ERROR_RO_HEADER     3
  #define ERROR_RO_DATASIZE   4
*/

// --- event lister ---------------------------------------------------------

CRocEvent *CReadback::Read(  )
{
  return Get(  ); // datapipe.h
}


CRocEvent *CPulseHeight::Read(  )
{
  return Get(  ); // datapipe.h
}


void Analyzer( CTestboard & tb )
{
  CDtbSource src( tb, false );
  CDataRecordScanner rec;
  CRocDecoder dec;
  CSink < CRocEvent * >pump; // datapipe.h

  src >> rec >> dec >> pump;

  pump.GetAll(  );
}
