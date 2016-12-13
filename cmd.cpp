
// test board, ROC, and Module commands

// Beat Meier, PSI, 31.8.2007 for Chip/Wafer tester
// Daniel Pitzl, DESY, 18.3.2014
// Claudia Seitz, DESY, 2014
// Daniel Pitzl, DESY, 5.7.2016
// Daniel Pitzl, DESY, 26.10.2016 for L1

#include <cstdlib> // abs
#include <math.h>
#include <time.h> // clock
#include <sys/time.h> // gettimeofday, timeval
#include <fstream> // gainfile, dacPar
#include <iostream> // cout
#include <iomanip> // setw
#include <sstream> // stringstream
#include <utility>
#include <set>

#include "TCanvas.h"
#include <TStyle.h>
#include <TF1.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TProfile2D.h>

#include "psi46test.h" // includes pixel_dtb.h
#include "analyzer.h" // includes datastream.h

#include "command.h"
#include "rpc.h"

#include "iseg.h" // HV

using namespace std;

#ifndef _WIN32
#define _int64 long long
#endif

#define DO_FLUSH  if( par.isInteractive() ) tb.Flush();

//#define DAQOPENCLOSE

using namespace std;

// globals:

//const uint32_t Blocksize = 8192; // Wolfram, does not help effmap 111
//const uint32_t Blocksize = 32700; // max 32767 in FW 2.0
//const uint32_t Blocksize = 124800; // FW 2.11, 4160*3*10
//const uint32_t Blocksize = 1048576; // 2^20
const uint32_t Blocksize = 4 * 1024 * 1024;
//const uint32_t Blocksize = 16*1024*1024; // ERROR
//const uint32_t Blocksize = 64*1024*1024; // ERROR

int dacval[16][256];
string dacName[256];

int Chip = 599;
int Module = 4001;
int nChannels = 2; // DAQ channels for L3/4 modules

bool L1 = 0;
bool L2 = 0;
bool L4 = 0;
//              ROC 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
int roc2ch[16] = {  6, 6, 7, 7, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 }; // ROC to DAQ channel for L1 modules
//             ch 0  1  2   3   4   5  6  7
int ch2roc[8] = { 4, 6, 8, 10, 12, 14, 0, 2 };

bool HV = 0;

int ierror = 0;

int roclist[16] = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

int modthr[16][52][80];
int modtrm[16][52][80];
int modcnt[16][52][80]; // responses
int modamp[16][52][80]; // amplitudes
bool hot[16][52][80] = { {{0}} }; // hot trim bits

// gain:

bool haveGain = 0;

double p0[16][52][80];
double p1[16][52][80];
double p2[16][52][80];
double p3[16][52][80];
double p4[16][52][80];
double p5[16][52][80];

TCanvas *c1 = NULL;

TH1D *h10 = NULL;
TH1D *h11 = NULL;
TH1D *h12 = NULL;
TH1D *h13 = NULL;
TH1D *h14 = NULL;
TH1D *h15 = NULL;
TH1D *h16 = NULL;
TH1D *h17 = NULL;

TH2D *h20 = NULL;
TH2D *h21 = NULL;
TH2D *h22 = NULL;
TH2D *h23 = NULL;
TH2D *h24 = NULL;
TH2D *h25 = NULL;

Iseg iseg;

//------------------------------------------------------------------------------
class TBstate
{
  bool daqOpen;
  uint32_t daqSize;
  int clockPhase;
  int deserPhase;

public:
  TBstate():daqOpen( 0 ), daqSize( 0 ), clockPhase( 4 ), deserPhase( 4 )
  {
  }
  ~TBstate()
  {
  }

  void SetDaqOpen( const bool open )
  {
    daqOpen = open;
  }
  bool GetDaqOpen()
  {
    return daqOpen;
  }

  void SetDaqSize( const uint32_t size )
  {
    daqSize = size;
  }
  uint32_t GetDaqSize()
  {
    return daqSize;
  }

  void SetClockPhase( const uint32_t phase )
  {
    clockPhase = phase;
  }
  uint32_t GetClockPhase()
  {
    return clockPhase;
  }

  void SetDeserPhase( const uint32_t phase160 )
  {
    deserPhase = phase160;
  }
  uint32_t GetDeserPhase()
  {
    return deserPhase;
  }
};

TBstate tbState;

//------------------------------------------------------------------------------
CMD_PROC( showtb ) // test board state
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;

  Log.section( "TBSTATE", true );

  cout << " 40 MHz clock phase " << tbState.GetClockPhase() << " ns" << endl;
  cout << "160 MHz deser phase " << tbState.GetDeserPhase() << " ns" << endl;
  if( tbState.GetDaqOpen() )
    cout << "DAQ memeory size " << tbState.GetDaqSize() << " words" << endl;
  else
    cout << "no DAQ open" << endl;

  uint8_t status = tb.GetStatus();
  printf( "SD card detect: %c\n", ( status & 8 ) ? '1' : '0' );
  printf( "CRC error:      %c\n", ( status & 4 ) ? '1' : '0' );
  printf( "Clock good:     %c\n", ( status & 2 ) ? '1' : '0' );
  printf( "CLock present:  %c\n", ( status & 1 ) ? '1' : '0' );

  Log.printf( "Clock phase %i\n", tbState.GetClockPhase() );
  Log.printf( "Deser phase %i\n", tbState.GetDeserPhase() );
  if( tbState.GetDaqOpen() )
    Log.printf( "DAQ memory size %i\n", tbState.GetDaqSize() );
  else
    Log.printf( "DAQ not open\n" );

  cout << "PixelAddressInverted " << tb.invertAddress << endl;

  return true;

} // showtb

//------------------------------------------------------------------------------
CMD_PROC( showhv ) // iseg HV status
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  iseg.status();
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( scan ) // scan for DTB on USB
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  CTestboard *tb = new CTestboard;
  string name;
  vector < string > devList;
  unsigned int nDev, nr;

  try {
    if( !tb->EnumFirst( nDev ) )
      throw int ( 1 );
    for( nr = 0; nr < nDev; ++nr ) {
      if( !tb->EnumNext( name ) )
        throw int ( 2 );
      if( name.size() < 4 )
        continue;
      if( name.compare( 0, 4, "DTB_" ) == 0 )
        devList.push_back( name );
    }
  }
  catch( int e ) {
    switch ( e ) {
    case 1:
      printf( "Cannot access the USB driver\n" );
      break;
    case 2:
      printf( "Cannot read name of connected device\n" );
      break;
    }
    delete tb;
    return true;
  }

  if( devList.size() == 0 ) {
    printf( "no DTB connected\n" );
    return true;
  }

  for( nr = 0; nr < devList.size(); ++nr )
    try {
      printf( "%10s: ", devList[nr].c_str() );
      if( !tb->Open( devList[nr], false ) ) {
	printf( "DTB in use\n" );
	continue;
      }
      unsigned int bid = tb->GetBoardId();
      printf( "DTB Id %u\n", bid );
      tb->Close();
    }
    catch( ... ) {
      printf( "DTB not identifiable\n" );
      tb->Close();
    }

  delete tb; // not permanentely opened

  return true;

} // scan

//------------------------------------------------------------------------------
CMD_PROC( open )
{
  if( tb.IsConnected() ) {
    printf( "Already connected to DTB.\n" );
    return true;
  }

  string usbId;
  char name[80];
  if( PAR_IS_STRING( name, 79 ) )
    usbId = name;
  else if( !tb.FindDTB( usbId ) )
    return true;

  bool status = tb.Open( usbId, false );

  if( !status ) {
    printf( "USB error: %s\nCould not connect to DTB %s\n",
            tb.ConnectionError(), usbId.c_str() );
    return true;
  }

  printf( "DTB %s opened\n", usbId.c_str() );

  string info;
  tb.GetInfo( info );
  printf( "--- DTB info-------------------------------------\n"
          "%s"
          "-------------------------------------------------\n",
          info.c_str() );

  return true;

} // open

//------------------------------------------------------------------------------
CMD_PROC( close )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Close();
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( welcome )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Welcome();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( setled )
{
  int val;
  PAR_INT( val, 0, 0x3f );
  tb.SetLed( val );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( log ) // put comment into log from script or command line
{
  char s[256];
  PAR_STRINGEOL( s, 255 );
  Log.printf( "%s\n", s );
  return true;
}

//------------------------------------------------------------------------------
bool UpdateDTB( const char *filename )
{
  fstream src;

  if( tb.UpgradeGetVersion() == 0x0100 ) {
    // open file
    src.open( filename );
    if( !src.is_open() ) {
      printf( "ERROR UPGRADE: Could not open \"%s\"!\n", filename );
      return false;
    }

    // check if upgrade is possible
    printf( "Start upgrading DTB.\n" );
    if( tb.UpgradeStart( 0x0100 ) != 0 ) {
      string msg;
      tb.UpgradeErrorMsg( msg );
      printf( "ERROR UPGRADE: %s!\n", msg.data() );
      return false;
    }

    // download data
    printf( "Download running ...\n" );
    string rec;
    uint16_t recordCount = 0;
    while( true ) {
      getline( src, rec );
      if( src.good() ) {
        if( rec.size() == 0 )
          continue;
        ++recordCount;
        if( tb.UpgradeData( rec ) != 0 ) {
          string msg;
          tb.UpgradeErrorMsg( msg );
          printf( "ERROR UPGRADE: %s!\n", msg.data() );
          return false;
        }
      }
      else if( src.eof() )
        break;
      else {
        printf( "ERROR UPGRADE: Error reading \"%s\"!\n", filename );
        return false;
      }
    }

    if( tb.UpgradeError() != 0 ) {
      string msg;
      tb.UpgradeErrorMsg( msg );
      printf( "ERROR UPGRADE: %s!\n", msg.data() );
      return false;
    }

    // write EPCS FLASH
    printf( "DTB download complete.\n" );
    tb.mDelay( 200 );
    printf( "FLASH write start (LED 1..4 on)\n"
            "DO NOT INTERUPT DTB POWER !\n"
            "Wait till LEDs goes off\n"
            "  exit from psi46test\n" "  power cycle the DTB\n" );
    tb.UpgradeExec( recordCount );
    tb.Flush();
    return true;
  }

  printf( "ERROR UPGRADE: Could not upgrade this DTB version!\n" );
  return false;

} // UpdateDTB

//------------------------------------------------------------------------------
CMD_PROC( upgrade )
{
  char filename[256];
  PAR_STRING( filename, 255 );
  UpdateDTB( filename );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( rpcinfo )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  string name, call, ts;

  tb.GetRpcTimestamp( ts );
  int version = tb.GetRpcVersion();
  int n = tb.GetRpcCallCount();

  printf( "--- DTB RPC info ----------------------------------------\n" );
  printf( "RPC version:     %i.%i\n", version / 256, version & 0xff );
  printf( "RPC timestamp:   %s\n", ts.c_str() );
  printf( "Number of calls: %i\n", n );
  printf( "Function calls:\n" );
  for( int i = 0; i < n; ++i ) {
    tb.GetRpcCallName( i, name );
    rpc_TranslateCallName( name, call );
    printf( "%5i: %s\n", i, call.c_str() );
  }
  return true;

} // rpcinfo

//------------------------------------------------------------------------------
CMD_PROC( info )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  string s;
  tb.GetInfo( s );
  printf( "--- DTB info ------------------------------------\n%s"
          "-------------------------------------------------\n",
          s.c_str() );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( version )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  string hw;
  tb.GetHWVersion( hw );
  int fw = tb.GetFWVersion();
  int sw = tb.GetSWVersion();
  printf( "%s: FW=%i.%02i SW=%i.%02i\n", hw.c_str(), fw / 256, fw % 256,
          sw / 256, sw % 256 );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( boardid )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  int id = tb.GetBoardId();
  printf( "Board Id = %i\n", id );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( init )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Init(); // done at power up?
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( flush )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Flush(); // send buffer of USB commands to DTB
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clear )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Clear(); // rpc_io->Clear()
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( udelay )
{
  int del;
  PAR_INT( del, 0, 65500 );
  if( del )
    tb.uDelay( del );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( mdelay )
{
  int ms;
  PAR_INT( ms, 1, 10000 );
  tb.mDelay( ms );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( d1 )
{
  int sig;
  PAR_INT( sig, 0, 131 );
  if( sig < 99 )
    tb.SignalProbeD1( sig );
  else
    tb.SignalProbeDeserD1( 0, sig ); // 4.06
    
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( d2 )
{
  int sig;
  PAR_INT( sig, 0, 131 );
  if( sig < 99 )
    tb.SignalProbeD2( sig );
  else
    tb.SignalProbeDeserD2( 0, sig );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( a1 )
{
  int sig;
  PAR_INT( sig, 0, 7 );
  tb.SignalProbeA1( sig );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( a2 )
{
  int sig;
  PAR_INT( sig, 0, 7 );
  tb.SignalProbeA2( sig );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( probeadc )
{
  int sig;
  PAR_INT( sig, 0, 7 );
  tb.SignalProbeADC( sig );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clksrc )
{
  int source;
  PAR_INT( source, 0, 1 ); // 1 = ext
  tb.SetClockSource( source );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clkok )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  if( tb.IsClockPresent() )
    cout << "clock OK" << endl;
  else
    cout << "clock missing" << endl;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( fsel ) // clock frequency selector, 0 = 40 MHz
{
  int div;
  PAR_INT( div, 0, 5 );
  tb.SetClock( div );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
// clock stretch
//pixel_dtb.h: #define STRETCH_AFTER_TRG  0
//pixel_dtb.h: #define STRETCH_AFTER_CAL  1
//pixel_dtb.h: #define STRETCH_AFTER_RES  2
//pixel_dtb.h: #define STRETCH_AFTER_SYNC 3
// width = 0 disable stretch

CMD_PROC( stretch ) // src=1 (after cal)  delay=8  width=999
{
  int src, delay, width;
  PAR_INT( src, 0, 3 );
  PAR_INT( delay, 0, 1023 );
  PAR_INT( width, 0, 0xffff );
  tb.SetClockStretch( src, delay, width );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clk ) // clock phase delay (relative to what?)
{
  int ns, duty;
  PAR_INT( ns, 0, 400 );
  if( !PAR_IS_INT( duty, -8, 8 ) )
    duty = 0;
  tb.Sig_SetDelay( SIG_CLK, ns, duty );
  tb.Sig_SetDelay( SIG_CTR, ns, duty );
  tb.Sig_SetDelay( SIG_SDA, ns + 15, duty );
  tb.Sig_SetDelay( SIG_TIN, ns +  5, duty );
  tbState.SetClockPhase( ns );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( sda ) // I2C data delay
{
  int ns, duty;
  PAR_INT( ns, 0, 400 );
  if( !PAR_IS_INT( duty, -8, 8 ) )
    duty = 0;
  tb.Sig_SetDelay( SIG_SDA, ns, duty );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( rda )
{
  int ns;
  PAR_INT( ns, 0, 32 );
  tb.Sig_SetRdaToutDelay( ns ); // 3.5
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( ctr ) // cal-trig-reset delay
{
  int ns, duty;
  PAR_INT( ns, 0, 400 );
  if( !PAR_IS_INT( duty, -8, 8 ) )
    duty = 0;
  tb.Sig_SetDelay( SIG_CTR, ns, duty );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tin ) // token in delay
{
  int ns, duty;
  PAR_INT( ns, 0, 400 );
  if( !PAR_IS_INT( duty, -8, 8 ) )
    duty = 0;
  tb.Sig_SetDelay( SIG_TIN, ns, duty );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clklvl )
{
  int lvl;
  PAR_INT( lvl, 0, 15 );
  tb.Sig_SetLevel( SIG_CLK, lvl );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( sdalvl )
{
  int lvl;
  PAR_INT( lvl, 0, 15 );
  tb.Sig_SetLevel( SIG_SDA, lvl );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( ctrlvl )
{
  int lvl;
  PAR_INT( lvl, 0, 15 );
  tb.Sig_SetLevel( SIG_CTR, lvl );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tinlvl )
{
  int lvl;
  PAR_INT( lvl, 0, 15 );
  tb.Sig_SetLevel( SIG_TIN, lvl );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( clkmode )
{
  int mode;
  PAR_INT( mode, 0, 3 );
  if( mode == 3 ) {
    int speed;
    PAR_INT( speed, 0, 31 );
    tb.Sig_SetPRBS( SIG_CLK, speed );
  }
  else
    tb.Sig_SetMode( SIG_CLK, mode );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( sdamode )
{
  int mode;
  PAR_INT( mode, 0, 3 );
  if( mode == 3 ) {
    int speed;
    PAR_INT( speed, 0, 31 );
    tb.Sig_SetPRBS( SIG_SDA, speed );
  }
  else
    tb.Sig_SetMode( SIG_SDA, mode );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( ctrmode )
{
  int mode;
  PAR_INT( mode, 0, 3 );
  if( mode == 3 ) {
    int speed;
    PAR_INT( speed, 0, 31 );
    tb.Sig_SetPRBS( SIG_CTR, speed );
  }
  else
    tb.Sig_SetMode( SIG_CTR, mode );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tinmode )
{
  int mode;
  PAR_INT( mode, 0, 3 );
  if( mode == 3 ) {
    int speed;
    PAR_INT( speed, 0, 31 );
    tb.Sig_SetPRBS( SIG_TIN, speed );
  }
  else
    tb.Sig_SetMode( SIG_TIN, mode );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( sigoffset )
{
  int offset;
  PAR_INT( offset, 0, 15 );
  tb.Sig_SetOffset( offset );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( lvds )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Sig_SetLVDS();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( lcds )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Sig_SetLCDS();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
/*
  CMD_PROC(tout)
  {
  int ns;
  PAR_INT(ns,0,450);
  tb.SetDelay(SIGNAL_TOUT, ns);
  DO_FLUSH
  return true;
  }

  CMD_PROC(trigout)
  {
  int ns;
  PAR_INT(ns,0,450);
  tb.SetDelay(SIGNAL_TRGOUT, ns);
  DO_FLUSH
  return true;
  }
*/

//------------------------------------------------------------------------------
CMD_PROC( pon )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Pon();
  Log.printf( "[PON]\n" );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( poff )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Poff();
  Log.printf( "[POFF]\n" );
  cout << "low voltage power off" << endl;
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( va )
{
  int val;
  PAR_INT( val, 0, 3000 );
  tb._SetVA( val );
  dacval[0][VAx] = val;
  Log.printf( "[SETVA] %i mV\n", val );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vd )
{
  int val;
  PAR_INT( val, 0, 3300 );
  tb._SetVD( val );
  dacval[0][VDx] = val;
  Log.printf( "[SETVD] %i mV\n", val );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( ia ) // analog current limit [mA]
{
  int val;
  PAR_INT( val, 0, 1200 );
  tb._SetIA( val * 10 ); // internal unit is 0.1 mA
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( id ) // digital current limit [mA]
{
  int val;
  PAR_INT( val, 0, 1800 );
  tb._SetID( val * 10 ); // internal unit is 0.1 mA
  ierror = 0; // clear error flag
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getva ) // measure analog supply voltage
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double v = tb.GetVA();
  printf( "VA %1.3f V\n", v );
  Log.printf( "VA %1.3f V\n", v );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getvd ) // measure digital supply voltage
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double v = tb.GetVD();
  printf( "VD %1.3f V\n", v );
  Log.printf( "VD %1.3f V\n", v );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getia ) // measure analog supply current
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double ia = tb.GetIA() * 1E3; // [mA]
  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;
  printf( "IA %4.1f mA for %i ROCs = %4.1f mA per ROC\n",
          ia, nrocs, ia / nrocs );
  Log.printf( "IA %4.1f mA for %i ROCs = %4.1f mA per ROC\n",
              ia, nrocs, ia / nrocs );
  return true;

} // getia

//------------------------------------------------------------------------------
CMD_PROC( getid ) // measure digital supply current
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double id = tb.GetID() * 1E3; // [mA]
  printf( "ID %1.1f mA\n", id );
  Log.printf( "ID %1.1f mA\n", id );

  ierror = 0;
  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;

  if( nrocs == 1 && id > 600 ) { // current limit [mA]
    ierror = 1;
    cout << "Error: digital current too high" << endl;
    Log.printf( "ERROR\n" );
  }
  return true;

} // getid

//------------------------------------------------------------------------------
CMD_PROC( hvon )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.HVon(); // close HV relais on DTB
  DO_FLUSH;
  Log.printf( "[HVON]\n" );
  HV = 1;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vb ) // set ISEG bias voltage
{
  int val;
  PAR_INT( val, 0, 2000 ); // software limit [V]
  iseg.setVoltage( val );
  Log.printf( "[SETVB] -%i\n", val );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getvb ) // measure back bias voltage
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double vb = iseg.getVoltage();
  cout << "bias voltage " << vb << " V" << endl;
  Log.printf( "VB %f V\n", vb );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( getib ) // get bias current
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  double ib = iseg.getCurrent();
  cout << "bias current " << ib*1E6 << " uA" << endl;
  Log.printf( "IB %f uA\n", ib*1E6 );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( scanvb ) // bias voltage scan
{
  int vmax;
  if( !PAR_IS_INT( vmax, 1, 400 ) )
    vmax = 150;
  int vstp;
  if( !PAR_IS_INT( vstp, 1, 100 ) )
    vstp = 5;

  int vmin = vstp;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int nstp = (vmax-vmin)/vstp + 1;

  Log.section( "SCANBIAS", true );

  if( h11 )
    delete h11;
  h11 = new
    TProfile( "bias_scan",
	      "bias scan;bias voltage [V];sensor bias current [uA]",
	      nstp, 0 - 0.5*vstp, vmax + 0.5*vstp );
  h11->SetStats( 10 );

  double wait = 2; // [s]

  for( int vb = vmin; vb <= vmax; vb += vstp ) {

    iseg.setVoltage( vb );

    double diff = vstp;
    int niter = 0;
    while( fabs( diff ) > 0.1 && niter < 11 ) {
      double vm = iseg.getVoltage(); // measure Vbias, negative!
      diff = fabs(vm) - vb;
      ++niter;
    }

    cout << "vbias " << setw(3) << vb << ": ";

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    double duration = 0;
    double uA = 0;

    while( duration < wait ) {

      uA = iseg.getCurrent() * 1E6; // [uA]

      gettimeofday( &tv, NULL );
      long s2 = tv.tv_sec; // seconds since 1.1.1970
      long u2 = tv.tv_usec; // microseconds
      duration = s2 - s1 + ( u2 - u1 ) * 1e-6;

      cout << " " << uA;

    }

    cout << endl;
    Log.printf( "%i %f\n", vb, uA );
    h11->Fill( vb, uA );

    h11->Draw( "hist" );
    c1->Update();

  } // bias scan

  iseg.setVoltage( vmin ); // for safety

  h11->Write();
  cout << "  histos 11" << endl;

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // scanvb

//------------------------------------------------------------------------------
CMD_PROC( ibvst ) // bias current vs time
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  Log.section( "IBVSTIME", true );

  if( h11 )
    delete h11;
  h11 = new
    TProfile( "bias_time",
	      "bias vs time;time [s];sensor bias current [uA]",
	      3600, 0, 3600, 0, 2000 );

  double wait = 1; // [s]

  double vb = iseg.getVoltage(); // measure Vbias, negative!

  cout << "vbias " << setw(3) << vb << endl;

  double runtime = 0;

  while( !keypressed() ) {

    double duration = 0;
    double uA = 0;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    while( duration < wait ) {

      uA = iseg.getCurrent() * 1E6; // [uA]

      gettimeofday( &tv, NULL );
      long s2 = tv.tv_sec; // seconds since 1.1.1970
      long u2 = tv.tv_usec; // microseconds
      duration = s2 - s1 + ( u2 - u1 ) * 1e-6;
      runtime = s2 - s0 + ( u2 - u0 ) * 1e-6;
      cout << " " << uA;

    }

    cout << endl;
    Log.printf( "%i %f\n", vb, uA );
    h11->Fill( runtime, uA );
    h11->Draw( "hist" );
    c1->Update();

  } // time

  h11->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // ibvst

//------------------------------------------------------------------------------
CMD_PROC( hvoff )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  iseg.setVoltage( 0 );
  tb.HVoff(); // open HV relais on DTB
  DO_FLUSH;
  Log.printf( "[HVOFF]\n" );
  cout << "HV off" << endl;
  HV = 0;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( reson )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.ResetOn();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( resoff )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.ResetOff();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( select ) // define active ROCs
{
  int rocmin, rocmax;
  PAR_RANGE( rocmin, rocmax, 0, 15 );

  int i;
  for( i = 0; i < rocmin; ++i )
    roclist[i] = 0;
  for( ; i <= rocmax; ++i )
    roclist[i] = 1;
  for( ; i < 16; ++i )
    roclist[i] = 0;

  //tb.Daq_Deser400_OldFormat( true ); // PSI D0003 has TBM08
  tb.Daq_Deser400_OldFormat( false ); // DESY has TBM08b/a

  vector < uint8_t > trimvalues( 4160 ); // uint8 in 2.15
  for( int i = 0; i < 4160; ++i )
    trimvalues[i] = 15; // 15 = no trim

  // set up list of ROCs in FPGA:

  vector < uint8_t > roci2cs;
  roci2cs.reserve( 16 );
  for( uint8_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] )
      roci2cs.push_back( roc );

  tb.SetI2CAddresses( roci2cs );

  cout << "setTrimValues for ROC";
  for( uint8_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      cout << " " << ( int ) roc << flush;
      tb.roc_I2cAddr( roc );
      tb.SetTrimValues( roc, trimvalues );
    }
  cout << endl;
  tb.roc_I2cAddr( rocmin );
  DO_FLUSH;

  return true;

} // select

//------------------------------------------------------------------------------
CMD_PROC( seltbm ) // select next TBM (FW 4.6 with L1 flag)
{
  int tbm;
  PAR_INT( tbm, 0, 1 );
  if( tbm )
    tb.roc_I2cAddr( 0 ); // TBM 1 talks to ROCs 0-3 and 12-15
  else
    tb.roc_I2cAddr( 4 ); // TBM 0 talks to ROCs 4-7 and 8-11
  DO_FLUSH;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( rocaddr )
{
  int addr;
  PAR_INT( addr, 0, 15 );
  tb.SetRocAddress( addr );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( rowinvert )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  //tb.SetPixelAddressInverted( 1 );
  tb.invertAddress = 1;
  cout << "SetPixelAddressInverted" << endl;
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( chip )
{
  int chip;
  PAR_INT( chip, 0, 9999 );

  Chip = chip;

  cout << "Chip " << Chip << endl;

  if( Chip >= 200 && Chip < 299 ) {
    //tb.SetPixelAddressInverted( 1 );
    tb.invertAddress = 1;
    cout << "SetPixelAddressInverted" << endl;
  }
  else { // just to be sure
    //tb.SetPixelAddressInverted( 0 );
    tb.invertAddress = 0;
    cout << "SetPixelAddressNotInverted" << endl;
  }
  if( Chip < 600 )
    tb.linearAddress = 0;
  else {
    tb.linearAddress = 1;
    cout << "PROC600 with linear address encoding" << endl;
  }

  dacName[1] = "Vdig";
  dacName[2] = "Vana";
  if( Chip < 400 )
    dacName[3] = "Vsf";
  else
    dacName[3] = "Vsh"; // digV2.1
  dacName[4] = "Vcomp";

  dacName[5] = "Vleak_comp"; // only analog
  dacName[6] = "VrgPr"; // removed on dig
  dacName[7] = "VwllPr";
  dacName[8] = "VrgSh"; // removed on dig
  dacName[9] = "VwllSh";

  dacName[10] = "VhldDel";
  dacName[11] = "Vtrim";
  dacName[12] = "VthrComp";

  dacName[13] = "VIBias_Bus";
  dacName[14] = "Vbias_sf";

  dacName[15] = "VoffsetOp";
  dacName[16] = "VIbiasOp"; // analog
  if( Chip < 400 )
    dacName[17] = "VoffsetRO"; //
  else
    dacName[17] = "PHOffset"; // digV2.1
  dacName[18] = "VIon";

  dacName[19] = "Vcomp_ADC"; // dig
  if( Chip < 400 )
    dacName[20] = "VIref_ADC"; // dig
  else
    dacName[20] = "PHScale"; // digV2.1
  dacName[21] = "VIbias_roc"; // analog
  dacName[22] = "VIColOr";

  dacName[25] = "VCal";
  dacName[26] = "CalDel";

  dacName[31] = "VD ";
  dacName[32] = "VA ";

  dacName[253] = "CtrlReg";
  dacName[254] = "WBC";
  dacName[255] = "RBReg";

  string gainFileName;

  if( Chip == 205 )
    gainFileName = "/home/pitzl/psi/dtb/tst305/c205-gaincal-trim36.dat"; // 6.8.2014

  if( Chip == 300 )
    gainFileName = "/home/pitzl/psi/dtb/tst221/c300-gaincal-trim36.dat"; // 25.6.2014
  //gainFileName = "/home/pitzl/psi/dtb/chip300/phcal-Ia25-trim30.dat";

  if( Chip == 309 )
    gainFileName = "/home/pitzl/psi/dtb/tst305/c309-trim36-tune-gaincal-17deg.dat"; // 4.8.2014

  if( Chip == 400 )
    gainFileName = "/home/pitzl/psi/dtb/tst221/c400-gaincal-trim24-vdig6-cool17.dat"; // 2.7.2014 HH
  //gainFileName = "/home/pitzl/psi/dtb/tst221/c400-gaincal-trim28-vdig6.dat"; // 2.7.2014 HH
  //gainFileName = "/home/pitzl/psi/dtb/tst221/c400-gaincal-trim28-cool17-vdig6.dat"; // 1.7.2014 HH
  //gainFileName = "/home/pitzl/psi/dtb/tst221/c400-gaincal-trim24-cool17.dat"; // 1.7.2014
  //gainFileName = "/home/pitzl/psi/dtb/tst221/c400-gaincal-trim21.dat"; // 23.6.2014
  //gainFileName = "/home/pitzl/psi/dtb/tst221/c400-gaincal-trim28.dat"; // 23.6.2014
  //gainFileName = "/home/pitzl/psi/dtb/tst221/c400-gaincal-trim33.dat"; // 23.6.2014
  //gainFileName = "/home/pitzl/psi/dtb/tst221/c400-phroc-Ia25-trim30.dat";

  if( Chip == 401 )
    gainFileName = "/home/pitzl/psi/dtb/tst219/phroc-c401-Ia25-trim30.dat";

  if( Chip == 405 )
    gainFileName = "/home/pitzl/psi/dtb/tst305/phroc-c405-high-gain-trim22.dat";
  //gainFileName = "/home/pitzl/psi/dtb/tst215/phroc-c405-trim30.dat";

  if( Chip == 431 )
    gainFileName = "/home/pitzl/psi/dtb/tst221/c431-phroc-Ia25-trim30.dat";

  if( Chip == 502 )
    gainFileName = "d502-phcal-trim32.dat";

  if( Chip == 503 )
    gainFileName = "d503-phroc-trim29.dat";

  if( Chip == 504 )
    gainFileName = "c504-ia24-trim27-gaincal.dat";

  if( Chip == 506 )
    gainFileName = "c506-ia24-trim28-gaincal-Sun.dat";
    //gainFileName = "c506-ia24-trim28-gaincal.dat";

  if( gainFileName.length() > 0 ) {

    ifstream gainFile( gainFileName.c_str() );

    if( gainFile ) {

      haveGain = 1;
      cout << "gainFile: " << gainFileName << endl;

      char ih[99];
      int icol;
      int irow;
      int roc = 0;

      while( gainFile >> ih ) {
	gainFile >> icol;
	gainFile >> irow;
	gainFile >> p0[roc][icol][irow];
	gainFile >> p1[roc][icol][irow];
	gainFile >> p2[roc][icol][irow];
	gainFile >> p3[roc][icol][irow];
	gainFile >> p4[roc][icol][irow];
	gainFile >> p5[roc][icol][irow];
      }

    } // gainFile open

  } // gainFileName

  return true;

} // chip

//------------------------------------------------------------------------------
CMD_PROC( module )
{
  int module;
  PAR_INT( module, 0, 9999 );

  Module = module;

  cout << "Module " << Module << endl;

  L1 = 0;
  L2 = 0;
  L4 = 0;

  if( Module <=  999 ) {
    tb.Daq_Deser400_OldFormat( true ); // PSI D0003 has TBM08
    cout << "TBM08 with old format" << endl;
  }
  else if( Module <= 1999 ) {
    tb.Daq_Deser400_OldFormat( false );
    nChannels = 8; // Layer 1
    L1 = 1;
    cout << "TBM10 with 8 DAQ channels" << endl;
  }
  else if( Module <= 2999 ) {
    tb.Daq_Deser400_OldFormat( false );
    nChannels = 4; // Layer 2
    L2 = 1;
    cout << "TBM09 with 4 DAQ channels" << endl;
  }
  else {
    tb.Daq_Deser400_OldFormat( false ); // DESY has TBM08b/a
    nChannels = 2; // Layer 3-4
    L4 = 1;
    cout << "TBM08abc with 2 channels" << endl;
  }

  string gainFileName;

  if( Module == 4016 )
    gainFileName = "D4016-trim32-chill17-gaincal.dat"; // E-lab
  if( Module == 4016 )
    gainFileName = "D4016-trim32UHH2-gaincal.dat"; // UHH X-rays

  if( Module == 4017 )
    //gainFileName = "D4017-ia24-trim32-chiller15-tuned-gaincal.dat";
    gainFileName = "D4017-ia24-trim32-chiller15-Vcomp_ADC1-gaincal.dat";

  if( Module == 4020 )
    gainFileName = "D4020-trim36-tune-gaincal.dat"; // chiller 15

  if( Module == 4022 )
    //gainFileName = "D4022-trim36-gaincal.dat"; // chiller 15
    //gainFileName = "D4022-trim32-gaincal.dat"; // chiller 15
    gainFileName = "D4022-ia24-trim16-gaincal.dat"; // at Uni X-ray, 15 deg

  if( Module == 4023 )
    gainFileName = "D4023-trim32-gaincal.dat";

  if( Module == 4509 )
    gainFileName = "K4509_trim32_chiller15_gaincalib.dat"; // KIT

  if( Module == 1405 )
    gainFileName = "P1405-trim32-chiller15-gaincal.dat"; // PSI

  if( Module == 1020 )
    gainFileName = "D1020-gaincal.dat"; // L1 at PSI

  if( Module == 1049 )
    gainFileName = "D1049-gaincal.dat"; // L1 at PSI, brass cooling block

  if( gainFileName.length() > 0 ) {

    ifstream gainFile( gainFileName.c_str() );

    if( gainFile ) {

      haveGain = 1;
      cout << "gainFile: " << gainFileName << endl;

      int roc;
      int icol;
      int irow;

      while( gainFile >> roc ) {
	gainFile >> icol;
	gainFile >> irow;
	gainFile >> p0[roc][icol][irow];
	gainFile >> p1[roc][icol][irow];
	gainFile >> p2[roc][icol][irow];
	gainFile >> p3[roc][icol][irow];
	gainFile >> p4[roc][icol][irow];
	gainFile >> p5[roc][icol][irow];
      }

    } // gainFile open

  } // gainFileName

  return true;

} // module

//------------------------------------------------------------------------------
CMD_PROC( pgset )
{
  int addr, delay, bits;
  PAR_INT( addr, 0, 255 );
  PAR_INT( bits, 0, 63 );
  PAR_INT( delay, 0, 255 );
  tb.Pg_SetCmd( addr, ( bits << 8 ) + delay );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pgstop )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Pg_Stop();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pgsingle )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Pg_Single();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pgtrig )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Pg_Trigger();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pgloop )
{
  int period;
  PAR_INT( period, 0, 65535 );
  tb.Pg_Loop( period );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( trigdel )
{
  int delay;
  PAR_INT( delay, 0, 65535 );
  tb.SetLoopTriggerDelay( delay ); // [BC]
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( trgsel ) // 4.1.1, bitmask 16 = PG, 4 = PG_DIR, 256 = ASYNC, 2048=ASYNC_DIR
{
  int bitmask;
  PAR_INT( bitmask, 0, 4095 );
  tb.Trigger_Select(bitmask);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC(trgdel) // 4.0
{
  int delay;
  PAR_INT( delay, 0, 255 );
  tb.Trigger_Delay(delay);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC(trgtimeout) // 4.0
{
  int timeout;
  PAR_INT(timeout, 0, 65535);
  tb.Trigger_Timeout(timeout);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC(trgper) // 4.0
{
  int period;
  PAR_INT(period, 0, 0x7fffffff);
  tb.Trigger_SetGenPeriodic(period);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC(trgrand) // 4.0
{
  int rate;
  PAR_INT(rate, 0, 0x7fffffff);
  tb.Trigger_SetGenRandom(rate);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC(trgsend) // 4.0
{
  int bitmask;
  PAR_INT(bitmask, 0, 31);
  tb.Trigger_Send(bitmask);
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
// inverse decorrelated Weibull PH -> large Vcal DAC
double PHtoVcal( double ph, uint16_t roc, uint16_t col, uint16_t row )
{
  if( !haveGain )
    return ph;

  if( roc > 15 )
    return ph;

  if( col > 51 )
    return ph;

  if( row > 79 )
    return ph;

  // phcal2ps decorrelated: ph = p4 + p3*exp(-t^p2), t = p0 + q/p1
  // phroc2ps decorrelated: ph = p4 - p3*exp(-t^p2), t = p0 + q/p1

  double Ared = ph - p4[roc][col][row]; // p4 is asymptotic maximum

  if( Ared >= 0 ) {
    Ared = -0.1; // avoid overflow
  }

  double a3 = p3[roc][col][row]; // negative

  if( a3 > 0 )
    a3 = -a3; // sign changed

  // large Vcal = ( (-ln((A-p4)/p3))^1/p2 - p0 )*p1
  // phroc: q =  ( (-ln(-(A-p4)/p3))^1/p2 - p0 )*p1 // sign changed

  double vc = p1[roc][col][row] * ( pow( -log( Ared / a3 ), 1 / p2[roc][col][row] ) - p0[roc][col][row] ); // [large Vcal]

  if( vc > 999 )
    cout << "overflow " << vc << " at"
	 << setw( 3 ) << col
	 << setw( 3 ) << row << ", Ared " << Ared << ", a3 " << a3 << endl;

  if( dacval[roc][CtrlReg] < 4 )
    return vc * p5[roc][col][row]; // small Vcal

  return vc; // large Vcal
}

//------------------------------------------------------------------------------
CMD_PROC( upd ) // redraw ROOT canvas; only works for global histos
{
  int plot;
  if( !PAR_IS_INT( plot, 10, 29 ) ) {
    gPad->Modified();
    gPad->Update();
    //c1->Modified();
    //c1->Update();
  }
  else if( plot == 10 ) {
    gStyle->SetOptStat( 111111 );
    if( h10 != NULL )
      h10->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 11 ) {
    gStyle->SetOptStat( 111111 );
    if( h11 != NULL )
      h11->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 12 ) {
    gStyle->SetOptStat( 111111 );
    if( h12 != NULL )
      h12->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 13 ) {
    gStyle->SetOptStat( 111111 );
    if( h13 != NULL )
      h13->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 14 ) {
    gStyle->SetOptStat( 111111 );
    if( h14 != NULL )
      h14->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 15 ) {
    gStyle->SetOptStat( 111111 );
    if( h15 != NULL )
      h15->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 16 ) {
    gStyle->SetOptStat( 111111 );
    if( h16 != NULL )
      h16->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 17 ) {
    gStyle->SetOptStat( 111111 );
    if( h17 != NULL )
      h17->Draw( "hist" );
    c1->Update();
  }
  else if( plot == 20 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h20 != NULL )
      h20->Draw( "colz" );
    c1->Update();
    gStyle->SetStatY( statY );
  }
  else if( plot == 21 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h21 != NULL )
      h21->Draw( "colz" );
    c1->Update();
    gStyle->SetStatY( statY );
  }
  else if( plot == 22 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h22 != NULL )
      h22->Draw( "colz" );
    c1->Update();
    gStyle->SetStatY( statY );
  }
  else if( plot == 23 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h23 != NULL )
      h23->Draw( "colz" );
    c1->Update();
    gStyle->SetStatY( statY );
  }
  else if( plot == 24 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h24 != NULL )
      h24->Draw( "colz" );
    c1->Update();
    gStyle->SetStatY( statY );
  }
  else if( plot == 25 ) {
    double statY = gStyle->GetStatY();
    gStyle->SetStatY( 0.95 );
    gStyle->SetOptStat( 10 ); // entries
    if( h25 != NULL )
      h25->Draw( "colz" );
    c1->Update();
    gStyle->SetStatY( statY );
  }

  return true;

} // update

//------------------------------------------------------------------------------
CMD_PROC( dopen )
{
  int buffersize;
  PAR_INT( buffersize, 0, 64100200 ); // [words = 2 bytes]
  int ch;
  if( !PAR_IS_INT( ch, 0, 7 ) )
    ch = 0;

  buffersize = tb.Daq_Open( buffersize, ch );
  if( buffersize == 0 )
    cout << "Daq_Open error for channel " << ch << ", size " <<
      buffersize << endl;
  else {
    cout << buffersize << " words allocated for data buffer " << ch <<
      endl;
    tbState.SetDaqOpen( 1 );
    tbState.SetDaqSize( buffersize );
  }

  return true;

} // dopen

//------------------------------------------------------------------------------
CMD_PROC( dsel ) // dsel 160 or 400
{
  int MHz;
  PAR_INT( MHz, 160, 400 );
  if( MHz > 200 )
    tb.Daq_Select_Deser400();
  else
    tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dselmod )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Daq_Select_Deser400();
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dselroca ) // activate ADC for analog ROC ?
{
  int datasize;
  PAR_INT( datasize, 1, 2047 );
  tb.Daq_Select_ADC( datasize, 1, 4, 6 );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dselsim )
{
  int startvalue;
  if( !PAR_IS_INT( startvalue, 0, 16383 ) )
    startvalue = 0;
  tb.Daq_Select_Datagenerator( startvalue );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dseloff )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.Daq_DeselectAll();
  DO_FLUSH return true;
}

//------------------------------------------------------------------------------
CMD_PROC( deser )
{
  int shift;
  PAR_INT( shift, 0, 7 );
  tb.Daq_Select_Deser160( shift );
  tbState.SetDeserPhase( shift );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( deser160 ) // scan DESER160 and clock phase for header 7F8
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  //DP tb.Pg_SetCmd(20, PG_RESR);
  //DP tb.Pg_SetCmd(0, PG_TOK);

  vector < uint16_t > data;

  vector < std::pair < int, int > > goodvalues;

  int x, y;
  printf( "deser phs 0     1     2     3     4     5     6     7\n" );

  for( y = 0; y < 25; ++y ) { // clk

    printf( "clk %2i:", y );

    tb.Sig_SetDelay( SIG_CLK, y );
    tb.Sig_SetDelay( SIG_CTR, y );
    tb.Sig_SetDelay( SIG_SDA, y + 15 );
    tb.Sig_SetDelay( SIG_TIN, y + 5 );

    for( x = 0; x < 8; ++x ) { // deser160 phase

      tb.Daq_Select_Deser160( x );
      tb.uDelay( 10 );

      tb.Daq_Start();

      tb.uDelay( 10 );

      tb.Pg_Single();

      tb.uDelay( 10 );

      tb.Daq_Stop();

      data.resize( tb.Daq_GetSize(), 0 );

      tb.Daq_Read( data, 100 );

      if( data.size() ) {
        int h = data[0] & 0xffc;
        if( h == 0x7f8 ) {
          printf( " <%03X>", int ( data[0] & 0xffc ) );
          goodvalues.push_back( std::make_pair( y, x ) );
        }
        else
          printf( "  %03X ", int ( data[0] & 0xffc ) );
      }
      else
        printf( "  ... " );

    } // deser phase

    printf( "\n" );

  } // clk phase

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
#endif

  printf( "Old values: clk delay %i, deserPhase %i\n",
          tbState.GetClockPhase(), tbState.GetDeserPhase() );

  if( goodvalues.size() == 0 ) {

    printf
      ( "No value found where header could be read back - no adjustments made.\n" );
    tb.Daq_Select_Deser160( tbState.GetDeserPhase() ); // back to default
    y = tbState.GetClockPhase(); // back to default
    tb.Sig_SetDelay( SIG_CLK, y );
    tb.Sig_SetDelay( SIG_CTR, y );
    tb.Sig_SetDelay( SIG_SDA, y + 15 );
    tb.Sig_SetDelay( SIG_TIN, y + 5 );
    return true;
  }
  printf( "Good values are:\n" );
  for( std::vector < std::pair < int, int > >::const_iterator it =
	 goodvalues.begin(); it != goodvalues.end(); ++it ) {
    printf( "%i %i\n", it->first, it->second );
  }
  const int select = floor( 0.5 * goodvalues.size() - 0.5 );
  tbState.SetClockPhase( goodvalues[select].first );
  tbState.SetDeserPhase( goodvalues[select].second );
  printf( "New values: clock delay %i, deserPhase %i\n",
          tbState.GetClockPhase(), tbState.GetDeserPhase() );
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() ); // set new
  y = tbState.GetClockPhase();
  tb.Sig_SetDelay( SIG_CLK, y );
  tb.Sig_SetDelay( SIG_CTR, y );
  tb.Sig_SetDelay( SIG_SDA, y + 15 );
  tb.Sig_SetDelay( SIG_TIN, y + 5 );

  return true;

} // deser160

//------------------------------------------------------------------------------
CMD_PROC( deserext ) // scan DESER160 phase for header 7F8 with ext trig
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif

  vector < uint16_t > data;

  vector < std::pair < int, int > > goodvalues;

  tb.Trigger_Select(256); // ext TRG

  int x, y;
  printf( "deser phs 0     1     2     3     4     5     6     7\n" );

  for( y = 0; y < 25; ++y ) { // clk

    printf( "clk %2i:", y );

    tb.Sig_SetDelay( SIG_CLK, y );
    tb.Sig_SetDelay( SIG_CTR, y );
    tb.Sig_SetDelay( SIG_SDA, y + 15 );
    tb.Sig_SetDelay( SIG_TIN, y + 5 );

    for( x = 0; x < 8; ++x ) { // deser160 phase

      tb.Daq_Select_Deser160( x );
      tb.uDelay( 10 );

      tb.Daq_Start();

      tb.uDelay( 100 );

      tb.Daq_Stop();

      data.resize( tb.Daq_GetSize(), 0 );

      tb.Daq_Read( data, 100 );

      if( data.size() ) {
        int h = data[0] & 0xffc;
        if( h == 0x7f8 ) {
          printf( " <%03X>", int ( data[0] & 0xffc ) );
          goodvalues.push_back( std::make_pair( y, x ) );
        }
        else
          printf( "  %03X ", int ( data[0] & 0xffc ) );
      }
      else
        printf( "  ... " );

    } // deser phase

    printf( "\n" );

  } // clk phase

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
#endif

  printf( "Old values: clk delay %i, deserPhase %i\n",
          tbState.GetClockPhase(), tbState.GetDeserPhase() );

  if( goodvalues.size() == 0 ) {

    printf
      ( "No value found where header could be read back - no adjustments made.\n" );
    tb.Daq_Select_Deser160( tbState.GetDeserPhase() ); // back to default
    y = tbState.GetClockPhase(); // back to default
    tb.Sig_SetDelay( SIG_CLK, y );
    tb.Sig_SetDelay( SIG_CTR, y );
    tb.Sig_SetDelay( SIG_SDA, y + 15 );
    tb.Sig_SetDelay( SIG_TIN, y + 5 );
    return true;
  }
  printf( "Good values are:\n" );
  for( std::vector < std::pair < int, int > >::const_iterator it =
	 goodvalues.begin(); it != goodvalues.end(); ++it ) {
    printf( "%i %i\n", it->first, it->second );
  }
  const int select = floor( 0.5 * goodvalues.size() - 0.5 );
  tbState.SetClockPhase( goodvalues[select].first );
  tbState.SetDeserPhase( goodvalues[select].second );
  printf( "New values: clock delay %i, deserPhase %i\n",
          tbState.GetClockPhase(), tbState.GetDeserPhase() );
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() ); // set new
  y = tbState.GetClockPhase();
  tb.Sig_SetDelay( SIG_CLK, y );
  tb.Sig_SetDelay( SIG_CTR, y );
  tb.Sig_SetDelay( SIG_SDA, y + 15 );
  tb.Sig_SetDelay( SIG_TIN, y + 5 );

  return true;

} // deserext

//------------------------------------------------------------------------------
CMD_PROC( dreset ) // dreset 3 = Deser400 reset
{
  int reset;
  PAR_INT( reset, 0, 255 ); // bits, 3 = 11
  tb.Daq_Deser400_Reset( reset );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dmodres )
{
  int reset;
  if( !PAR_IS_INT( reset, 0, 3 ) )
    reset = 3;
  tb.Daq_Deser400_Reset( reset );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dclose )
{
  int ch;
  if( !PAR_IS_INT( ch, 0, 7 ) )
    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Close( ch );
  else
    tb.Daq_Close( ch );
  tbState.SetDaqOpen( 0 );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dstart )
{
  int ch;
  if( !PAR_IS_INT( ch, 0, 7 ) )
    ch = 0;
  tb.Daq_Start( ch );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dstop )
{
  int ch;
  if( !PAR_IS_INT( ch, 0, 7 ) )
    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Stop( ch );
  else
    tb.Daq_Stop( ch );    
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dsize )
{
  int ch;
  if( !PAR_IS_INT( ch, 0, 7 ) )
    ch = 0;
  unsigned int size = tb.Daq_GetSize( ch );
  printf( "size = %u\n", size );
  return true;
}

//------------------------------------------------------------------------------
void DecodeTbmHeader08( unsigned int raw )
{
  int evNr = raw >> 8;
  int stkCnt = raw & 6;
  printf( "  EV(%3i) STF(%c) PKR(%c) STKCNT(%2i)",
          evNr,
          ( raw & 0x0080 ) ? '1' : '0',
          ( raw & 0x0040 ) ? '1' : '0', stkCnt );
}

//------------------------------------------------------------------------------
void DecodeTbmTrailer08( unsigned int raw )
{
  int dataId = ( raw >> 6 ) & 0x3;
  int data = raw & 0x3f;
  printf
    ( "  NTP(%c) RST(%c) RSR(%c) SYE(%c) SYT(%c) CTC(%c) CAL(%c) SF(%c) D%i(%2i)",
      ( raw & 0x8000 ) ? '1' : '0', ( raw & 0x4000 ) ? '1' : '0',
      ( raw & 0x2000 ) ? '1' : '0', ( raw & 0x1000 ) ? '1' : '0',
      ( raw & 0x0800 ) ? '1' : '0', ( raw & 0x0400 ) ? '1' : '0',
      ( raw & 0x0200 ) ? '1' : '0', ( raw & 0x0100 ) ? '1' : '0', dataId,
      data );
}

//------------------------------------------------------------------------------
void DecodeTbmHeader( unsigned int raw ) // 08abc
{
  int data = raw & 0x3f;
  int dataId = ( raw >> 6 ) & 0x3;
  int evNr = raw >> 8;
  printf( "  EV(%3i) D%i(%2i)", evNr, dataId, data );
}

//------------------------------------------------------------------------------
void DecodeTbmTrailer( unsigned int raw ) // 08bc
{
  int stkCnt = raw & 0x3f; // 3f = 11'1111
  printf
    ( "  NTP(%c) RST(%c) RSR(%c) SYE(%c) SYT(%c) CTC(%c) CAL(%c) SF(%c) ARS(%c) PRS(%c) STKCNT(%2i)", // DP bug fix: ARS-PKR order
      ( raw & 0x8000 ) ? '1' : '0',
      ( raw & 0x4000 ) ? '1' : '0',
      ( raw & 0x2000 ) ? '1' : '0',
      ( raw & 0x1000 ) ? '1' : '0',
      ( raw & 0x0800 ) ? '1' : '0',
      ( raw & 0x0400 ) ? '1' : '0',
      ( raw & 0x0200 ) ? '1' : '0',
      ( raw & 0x0100 ) ? '1' : '0',
      ( raw & 0x0080 ) ? '1' : '0',
      ( raw & 0x0040 ) ? '1' : '0',
      stkCnt );
}

//------------------------------------------------------------------------------
void DecodePixel( unsigned int raw )
{
  unsigned int ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
  raw >>= 9;

  int y = 80;
  int x = 0;

  if( Chip < 600 ) { // psi46dig

    int c = ( raw >> 12 ) & 7;
    c = c * 6 + ( ( raw >> 9 ) & 7 );

    int r2 = ( raw >> 6 ) & 7;
    int r1 = ( raw >> 3 ) & 7;
    int r0 = ( raw >> 0 ) & 7;

    if( tb.invertAddress ) {
      r2 ^= 0x7;
      r1 ^= 0x7;
      r0 ^= 0x7;
    }

    int r = r2 * 36 + r1 * 6 + r0;

    y = 80 - r / 2;
    x = 2 * c + ( r & 1 );

  } // psi46dig
  else {
    y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
    x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
  } // proc600

  //printf( "   Pixel [%05o] %2i/%2i: %3u", raw, x, y, ph);
  printf( " = pix %2i.%2i ph %3u", x, y, ph );
}

//------------------------------------------------------------------------------
#define DECBUFFERSIZE 2048

class Decoder
{
  int printEvery;

  int nReadout;
  int nPixel;

  FILE *f;
  int nSamples;
  uint16_t *samples;

  int x, y, ph;
public:
  Decoder():printEvery( 0 ), nReadout( 0 ), nPixel( 0 ), f( 0 ),
	      nSamples( 0 ), samples( 0 )
  {
  }
  ~Decoder()
  {
    Close();
  }
  bool Open( const char *filename );
  void Close()
  {
    if( f )
      fclose( f );
    f = 0;
    delete[]samples;
  }
  bool Sample( uint16_t sample );
  void DumpSamples( int n );
  void Translate( unsigned long raw );
  uint16_t GetX()
  {
    return x;
  }
  uint16_t GetY()
  {
    return y;
  }
  uint16_t GetPH()
  {
    return ph;
  }
  void AnalyzeSamples();

}; // class Decoder

//------------------------------------------------------------------------------
bool Decoder::Open( const char *filename )
{
  samples = new uint16_t[DECBUFFERSIZE];
  f = fopen( filename, "wt" );
  return f != 0;
}

//------------------------------------------------------------------------------
bool Decoder::Sample( uint16_t sample )
{
  if( sample & 0x8000 ) { // start marker
    if( nReadout && printEvery >= 1000 ) {
      AnalyzeSamples();
      printEvery = 0;
    }
    else
      ++printEvery;
    ++nReadout;
    nSamples = 0;
  }
  if( nSamples < DECBUFFERSIZE ) {
    samples[nSamples++] = sample;
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------
void Decoder::DumpSamples( int n )
{
  if( nSamples < n )
    n = nSamples;
  for( int i = 0; i < n; ++i )
    fprintf( f, " %04X", ( unsigned int ) ( samples[i] ) );
  fprintf( f, " ... %04X\n", ( unsigned int ) ( samples[nSamples - 1] ) );
}

//------------------------------------------------------------------------------
void Decoder::Translate( unsigned long raw )
{
  ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
  raw >>= 9;

  y = 80;
  x = 0;

  if( Chip < 600 ) {

    int c = ( raw >> 12 ) & 7;
    c = c * 6 + ( ( raw >> 9 ) & 7 );

    int r2 = ( raw >> 6 ) & 7;
    int r1 = ( raw >> 3 ) & 7;
    int r0 = ( raw >> 0 ) & 7;

    if( tb.invertAddress ) {
      r2 ^= 0x7;
      r1 ^= 0x7;
      r0 ^= 0x7;
    }

    int r = r2 * 36 + r1 * 6 + r0;

    y = 80 - r / 2;
    x = 2 * c + ( r & 1 );

  } // psi46dig
  else {
    y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
    x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
  } // proc600

} // Translate

//------------------------------------------------------------------------------
void Decoder::AnalyzeSamples()
{
  if( nSamples < 1 ) {
    nPixel = 0;
    return;
  }
  fprintf( f, "%5i: %03X: ", nReadout,
           ( unsigned int ) ( samples[0] & 0xfff ) );
  nPixel = ( nSamples - 1 ) / 2;
  int pos = 1;
  for( int i = 0; i < nPixel; ++i ) {
    unsigned long raw = ( samples[pos++] & 0xfff ) << 12;
    raw += samples[pos++] & 0xfff;
    Translate( raw );
    fprintf( f, " %2i", x );
  }
  // for( pos = 1; pos < nSamples; pos++) fprintf(f, " %03X", int(samples[pos]));
  fprintf( f, "\n" );
}

//------------------------------------------------------------------------------
CMD_PROC( dread ) // daq read and print as ROC data
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;
  cout << nrocs << " ROCs defined" << endl;
  int tbmData = 0;
  if( nrocs > 1 )
    tbmData = 2; // header and trailer words

  uint32_t words_remaining = 0;
  vector < uint16_t > data;

  tb.Daq_Read( data, Blocksize, words_remaining );

  int size = data.size();

  cout << "words read: " << size
       << " = " << ( size - nrocs - tbmData ) / 2 << " pixels"
       << endl;
  cout << "words remaining in DTB memory: " << words_remaining << endl;

  for( int i = 0; i < size; ++i ) {
    printf( " %4X", data[i] );
    Log.printf( " %4X", data[i] );
    if( i % 20 == 19 ) {
      printf( "\n" );
      Log.printf( "\n" );
    }
  }
  printf( "\n" );
  Log.printf( "\n" );
  Log.flush();

  cout << "PixelAddressInverted " << tb.invertAddress << endl;

  //decode:

  Decoder dec;

  uint32_t npx = 0;
  uint32_t nrr = 0;
  uint32_t ymax = 0;
  uint32_t nn[52][80] = { {0}
  };

  bool ldb = 0;

  if( h21 )
    delete h21;
  h21 = new TH2D( "HitMap",
                  "Hit map;col;row;hits", 52, -0.5, 51.5, 80, -0.5, 79.5 );

  bool even = 1;

  for( unsigned int i = 0; i < data.size(); ++i ) {

    if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
      if( ldb )
        printf( "found header: %X\n", data[i] );
      even = 0;
    }
    else if( even ) { // merge 2 words into one pixel int:

      if( i < data.size() - 1 ) {

        unsigned long raw = ( data[i] & 0xfff ) << 12;
        raw += data[i + 1] & 0xfff;

        dec.Translate( raw );

        uint16_t ix = dec.GetX();
        uint16_t iy = dec.GetY();
        uint16_t ph = dec.GetPH();
        double vc = ph;
        if( ldb )
          vc = PHtoVcal( ph, 0, ix, iy );
        if( ldb )
          cout << setw( 3 ) << ix << setw( 3 ) << iy << setw( 4 ) << ph <<
            ",";
        if( ldb )
          cout << "(" << vc << ")";
        if( npx % 8 == 7 && ldb )
          cout << endl;
        if( ix < 52 && iy < 80 ) {
          ++nn[ix][iy]; // hit map
          if( iy > ymax )
            ymax = iy;
          h21->Fill( ix, iy );
        }
        else
          ++nrr; // error
        ++npx;
      }
    }
    even = 1 - even;
  } // data
  if( ldb )
    cout << endl;

  cout << npx << " pixels" << endl;
  cout << nrr << " address errors" << endl;

  h21->Write();
  gStyle->SetOptStat( 10 ); // entries
  gStyle->SetStatY( 0.95 );
  h21->GetYaxis()->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 21" << endl;

  // hit map:
  /*
    if( npx > 0 ) {
    if( ymax < 79 ) ++ymax; // print one empty
    for( int row = ymax; row >= 0; --row ) {
    cout << setw(2) << row << " ";
    for( int col = 0; col < 52; ++col )
    if( nn[col][row] )
    cout << "x";
    else
    cout << " ";
    cout << "|" << endl;
    }
    }
  */
  return true;

} // dread

//------------------------------------------------------------------------------
CMD_PROC( dreadm ) // module
{
  int ch;
  if( !PAR_IS_INT( ch, 0, 7 ) )
    ch = 0;

  uint32_t words_remaining = 0;
  vector < uint16_t > data;

  tb.Daq_Read( data, Blocksize, words_remaining, ch );

  int size = data.size();
  printf( "DAQ read %i words, remaining: %u\n", size, words_remaining );

  unsigned int hdr = 0, trl = 0;
  unsigned int raw = 0;

  Decoder dec;

  if( h21 )
    delete h21;
  h21 = new TH2D( "ModuleHitMap",
                  "Module response map;col;row;hits",
                  8 * 52, -0.5, 8 * 52 - 0.5,
                  2 * 80, -0.5, 2 * 80 - 0.5 );
  gStyle->SetOptStat( 10 ); // entries
  h21->GetYaxis()->SetTitleOffset( 1.3 );

  // int TBM_eventnr,TBM_stackinfo,ColAddr,RowAddr,PulseHeight,TBM_trailerBits,TBM_readbackData;

  int32_t kroc = (16/nChannels) * ch - 1; // will start at 0 or 8

  for( int i = 0; i < size; ++i ) {

    int d = data[i] & 0x0fff; // 12 data bits

    uint16_t v = ( data[i] >> 12 ) & 0xe; // e = 14 = 1110, flag

    switch ( v ) {

    case 10: // TBM header
      printf( "TBM H12(%03X)", d );
      hdr = d;
      break;
    case 8: // TBM header part 2
      printf( "+H34(%03X) =", d );
      hdr = ( hdr << 8 ) + d;
      DecodeTbmHeader( hdr );
      kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
      if( L1 ) kroc = ch2roc[ch] - 1;
      break;

    case 4: // ROC header
      printf( "\nROC-HEADER(%03X): ", d );
      ++kroc; // start at 0 or 8
      break;

    case 0: // pixel data
      printf( "\nR123(%03X)", d );
      raw = d;
      break;
    case 2: // pixel data part 2
      printf( "+R456(%03X)", d );
      raw = ( raw << 12 ) + d;
      DecodePixel( raw );
      dec.Translate( raw );
      {
	uint16_t ix = dec.GetX();
	uint16_t iy = dec.GetY();
	//uint16_t ph = dec.GetPH();
	int l = kroc % 8; // 0..7
	int xm = 52 * l + ix; // 0..415  rocs 0 1 2 3 4 5 6 7
	int ym = iy; // 0..79
	if( kroc > 7 ) {
	  xm = 415 - xm; // rocs 8 9 A B C D E F
	  ym = 159 - ym; // 80..159
	}
	h21->Fill( xm, ym );
      }
      break;

    case 14: // TBM trailer
      printf( "\nTBM T12(%03X)", d );
      trl = d;
      break;
    case 12: // TBM trailer part 2
      printf( "+T34(%03X) =", d );
      trl = ( trl << 8 ) + d;
      DecodeTbmTrailer( trl );
      printf( "\n" );
      break;

    default:
      printf( "\nunknown data: %X = %d", v, v );
      break;

    } // switch

  } // data block

  /*
  for( int i = 0; i < size; ++i ) {
    int x = data[i] & 0xffff;
    Log.printf( " %04X", x );
    if( i % 100 == 99 )
      Log.printf( "\n" );
  }
  Log.printf( "\n" );
  Log.flush();
  */
  h21->Write();
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 21" << endl;

  return true;

} // dreadm

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 14.12.2015, scan TBM delays, PG trigger, until first error
CMD_PROC( tbmscan )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Open( Blocksize, ch );
#endif

  tb.Pg_Stop(); // stop DTB RCTT pattern generator

  tb.Daq_Select_Deser400();
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  vector < int > errcnt(256);

  for( int itin = 0; itin < 2; ++itin )
    for( int itbm = 0; itbm < 2; ++itbm )
      for( int ipt1 = 0; ipt1 < 8; ++ipt1 )
	for( int ipt0 = 0; ipt0 < 8; ++ipt0 ) {

	  tb.Pg_Stop(); // quiet

	  tb.tbm_Set( 0xE4, 0xF4 ); // clear TBM, reset ROCs
	  tb.tbm_Set( 0xF4, 0xF4 ); // clear TBM, reset ROCs
	  // more for L1 TBM10

	  int ibyte = 128*itin + 64*itbm + 8*ipt1 + ipt0;

	  tb.tbm_Set( 0xEA, ibyte );
	  tb.tbm_Set( 0xFA, ibyte );

	  cout << setw(3) << ibyte;

	  tb.Flush();

	  vector <unsigned int> nev(nChannels); // init to zero
	  vector <unsigned int> nrr(nChannels);

	  uint32_t trl = 0; // need to remember from previous daq_read

	  bool ldb = 0;

	  bool roerr = 0;
	  int nok = 0;

	  tb.Daq_Deser400_Reset( 3 );
	  tb.uDelay( 100 );
	  for( int ch = 0; ch < nChannels; ++ch )
	    tb.Daq_Start( ch );
	  tb.uDelay( 100 );

	  tb.Pg_Single();

	  tb.uDelay( 10 );
	  for( int ch = 0; ch < nChannels; ++ch )
	    tb.Daq_Stop( ch );

	  // decode data:

	  for( int ch = 0; ch < nChannels; ++ch ) {

	    uint32_t remaining = 0;
	    vector <uint16_t > data;
	    data.reserve( Blocksize );

	    tb.Daq_Read( data, Blocksize, remaining, 0 );

	    int size = data.size();
	    uint32_t raw = 0;
	    uint32_t hdr = 0;
	    int32_t kroc = (16/nChannels) * ch - 1; // will start at 0

	    for( int ii = 0; ii < size; ++ii ) {

	      int d = 0;
	      int v = 0;
	      d = data[ii] & 0xfff; // 12 bits data
	      v = ( data[ii] >> 12 ) & 0xe; // 3 flag bits: e = 14 = 1110
	      int c = 0;
	      int r = 0;
	      int x = 0;
	      int y = 0;

	      switch ( v ) {

		// TBM header:
	      case 10:
		hdr = d;
		if( nev[ch] > 0 && trl == 0 ) {
		  cout << "TBM error: header without previous trailer in event "
		       << nev[ch]
		       << ", channel " << ch
		       << endl;
		  ++nrr[ch];
		  if( !roerr ) nok = nev[ch];
		  roerr = 1;
		}
		trl = 0;
		break;
	      case 8:
		hdr = ( hdr << 8 ) + d;
		if( ldb ) {
		  cout << "event " << nev[ch] << endl;
		  cout << "TBM header";
		  DecodeTbmHeader( hdr );
		  cout << endl;
		}
		kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
		if( L1 ) kroc = ch2roc[ch] - 1;
		break;

		// ROC header data:
	      case 4:
		++kroc; // start at 0
		if( ldb ) {
		  if( kroc > 0 )
		    cout << endl;
		  cout << "ROC " << setw( 2 ) << kroc;
		}
		if( kroc > 15 ) {
		  cout << "Error kroc " << kroc << endl;
		  kroc = 15;
		  ++nrr[ch];
		  if( !roerr ) nok = nev[ch];
		  roerr = 1;
		}
		if( kroc == 0 && hdr == 0 ) {
		  cout << "TBM error: no header in event " << nev[ch]
		       << " ch " << ch
		       << endl;
		  ++nrr[ch];
		  if( !roerr ) nok = nev[ch];
		  roerr = 1;
		}
		hdr = 0;
		break;

		// pixel data:
	      case 0:
		raw = d;
		break;
	      case 2:
		raw = ( raw << 12 ) + d;
		raw >>= 9;
		if( Chip < 600 ) {
		  c = ( raw >> 12 ) & 7;
		  c = c * 6 + ( ( raw >> 9 ) & 7 );
		  r = ( raw >> 6 ) & 7;
		  r = r * 6 + ( ( raw >> 3 ) & 7 );
		  r = r * 6 + ( raw & 7 );
		  y = 80 - r / 2;
		  x = 2 * c + ( r & 1 );
		} // psi46dig
		else {
		  y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
		  x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
		} // proc600
		if( ldb )
		  cout << " " << x << "." << y;
		if( kroc < 0 || ( L4 && ch == 1 && kroc < 8 ) || kroc > 15 ) {
		  cout << "ROC data with wrong ROC count " << kroc
		       << " in event " << nev[ch]
		       << endl;
		  ++nrr[ch];
		  if( !roerr ) nok = nev[ch];
		  roerr = 1;
		}
		else if( y > -1 && y < 80 && x > -1 && x < 52 ) {
		} // valid px
		else {
		  cout << "invalid col row " << setw(3) << x
		       << setw(4) << y
		       << " in ROC " << setw(2) << kroc
		       << endl;
		  ++nrr[ch];
		  if( !roerr ) nok = nev[ch];
		  roerr = 1;
		}
		break;

		// TBM trailer:
	      case 14:
		trl = d;
		if( L4 && ch == 0 && kroc != 7 ) {
		  cout
		    << "wrong ROC count " << kroc
		    << " in event " << nev[ch] << " ch 0" 
		    << endl;
		  ++nrr[ch];
		  if( !roerr ) nok = nev[ch];
		  roerr = 1;
		}
		else if( L4 && ch == 1 && kroc != 15 ) {
		  cout
		    << "wrong ROC count " << kroc
		    << " in event " << nev[ch] << " ch 1" 
		    << endl;
		  ++nrr[ch];
		  if( !roerr ) nok = nev[ch];
		  roerr = 1;
		}
		break;
	      case 12:
		trl = ( trl << 8 ) + d;
		if( ldb ) {
		  cout << endl;
		  cout << "TBM trailer";
		  DecodeTbmTrailer( trl );
		  cout << endl;
		}
		++nev[ch];
		trl = 1; // flag
		break;

	      default:
		printf( "\nunknown data: %X = %d", v, v );
		++nrr[ch];
		if( !roerr ) nok = nev[ch];
		roerr = 1;
		break;

	      } // switch

	    } // data

	    cout << ": ch " << ch << " err " << nrr[ch];

	  } // ch

	  cout << endl;

	  errcnt.at(ibyte) = 0;
	  for( int ch = 0; ch < nChannels; ++ch )
	    errcnt.at(ibyte) -= nrr[ch]; // negative

	} // tbmset

  // all off:

  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Stop( ch );
  tb.Flush();

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Close( ch );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  cout << endl;
  for( int itin = 0; itin < 2; ++itin )
    for( int itbm = 0; itbm < 2; ++itbm ) {
      cout << "TIN delay " << itin*6.25
	   << "ns, TBM header/trailer delay " << itbm*6.25
	   << " ns: -errcnt(A + B)" << endl;
      cout << "port:";
      for( int ipt0 = 0; ipt0 < 8; ++ipt0 )
	cout << setw(8) << ipt0;
      cout << endl;
      for( int ipt1 = 0; ipt1 < 8; ++ipt1 ) {
	cout << setw(4) << ipt1 <<":";
	for( int ipt0 = 0; ipt0 < 8; ++ipt0 ){
	  int ibyte = 128*itin + 64*itbm + 8*ipt1 + ipt0;
	  cout << setw(8) << errcnt.at(ibyte);
	}
	cout << endl;
      }
    }

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // tbmscan

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 14.12.2015, scan TBM delays, random trigger, until first error
CMD_PROC( tbmscant )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // enable all pixels:

  int masked = 0;

  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.roc_ClrCal();
      for( int col = 0; col < 52; ++col ) {
        tb.roc_Col_Enable( col, 1 );
        for( int row = 0; row < 80; ++row ) {
          int trim = modtrm[roc][col][row];
	  if( trim > 15 ) {
	    tb.roc_Pix_Mask( col, row );
	    ++masked;
	    cout << "mask roc col row "
		 << setw(2) << roc
		 << setw(3) << col
		 << setw(3) << row
		 << endl;
	  }
	  else
          tb.roc_Pix_Trim( col, row, trim );
        }
      }
    }
  tb.Flush();

  if( h21 )
    delete h21;
  h21 = new TH2D( "ModuleHitMap",
		  "Module response map;col;row;hits",
		  8 * 52, -0.5, 8 * 52 - 0.5,
		  2 * 80, -0.5, 2 * 80 - 0.5 );
  gStyle->SetOptStat( 10 ); // entries
  h21->GetYaxis()->SetTitleOffset( 1.3 );

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Open( Blocksize, ch );
#endif

  tb.Pg_Stop(); // stop DTB RCTT pattern generator

  tb.Daq_Select_Deser400();
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Start( ch );
  tb.uDelay( 1000 );

  for( int itin = 0; itin < 2; ++itin )
    for( int itbm = 0; itbm < 2; ++itbm )
      for( int ipt1 = 0; ipt1 < 8; ++ipt1 )
	for( int ipt0 = 0; ipt0 < 8; ++ipt0 ) {

	  tb.Trigger_Select(0); // quiet

	  tb.tbm_Set( 0xE4, 0xF4 ); // clear TBM, reset ROCs
	  tb.tbm_Set( 0xF4, 0xF4 ); // clear TBM, reset ROCs

	  tb.tbm_Set( 0xEA, 128*itin + 64*itbm + 8*ipt1 + ipt0 );
	  tb.tbm_Set( 0xFA, 128*itin + 64*itbm + 8*ipt1 + ipt0 );

	  tb.Trigger_SetGenRandom(10*1000*1000); // 90 kHz randoms

	  tb.Trigger_Select(512); // start GEN_DIR trigger generator
	  tb.Flush();

	  unsigned int nev[8] = { };
	  unsigned int ndq = 0;
	  unsigned int got = 0;
	  unsigned int npx = 0;
	  unsigned int nrr[8] = { };

	  uint32_t trl = 0; // need to remember from previous daq_read

	  bool ldb = 0;

	  bool roerr = 0;
	  int nok = 0;

	  while( !roerr && nev[0] < 1000*1000 ) {

	    tb.mDelay( 200 ); // [ms]

	    ++ndq;

	    gettimeofday( &tv, NULL );
	    long s2 = tv.tv_sec; // seconds since 1.1.1970
	    long u2 = tv.tv_usec; // microseconds

	    // decode data:

	    for( int ch = 0; ch < nChannels; ++ch ) {

	      uint32_t remaining = 0;
	      vector <uint16_t > data;
	      data.reserve( Blocksize );
	      tb.Daq_Read( data, Blocksize, remaining, ch );
	      got += data.size();
	      int size = data.size();
	      uint32_t raw = 0;
	      uint32_t hdr = 0;
	      int32_t kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
	      unsigned int npxev = 0;

	      for( int ii = 0; ii < size; ++ii ) {

		int d = 0;
		int v = 0;
		d = data[ii] & 0xfff; // 12 bits data
		v = ( data[ii] >> 12 ) & 0xe; // 3 flag bits: e = 14 = 1110
		int c = 0;
		int r = 0;
		int x = 0;
		int y = 0;

		switch ( v ) {

		  // TBM header:
		case 10:
		  hdr = d;
		  npxev = 0;
		  if( nev[ch] > 0 && trl == 0 ) {
		    cout << "TBM error: header without previous trailer in event "
			 << nev[ch]
			 << ", channel " << ch
			 << endl;
		    ++nrr[ch];
		    if( !roerr ) nok = nev[ch];
		    roerr = 1;
		  }
		  trl = 0;
		  break;
		case 8:
		  hdr = ( hdr << 8 ) + d;
		  if( ldb ) {
		    cout << "event " << nev[ch] << endl;
		    cout << "TBM header";
		    DecodeTbmHeader( hdr );
		    cout << endl;
		  }
		  kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
		  if( L1 ) kroc = ch2roc[ch] - 1;
		  break;

		  // ROC header data:
		case 4:
		  ++kroc; // start at 0
		  if( ldb ) {
		    if( kroc > 0 )
		      cout << endl;
		    cout << "ROC " << setw( 2 ) << kroc;
		  }
		  if( kroc > 15 ) {
		    cout << "Error kroc " << kroc << endl;
		    kroc = 15;
		    ++nrr[ch];
		    if( !roerr ) nok = nev[ch];
		    roerr = 1;
		  }
		  if( kroc == 0 && hdr == 0 ) {
		    cout << "TBM error: no header in event " << nev[ch]
			 << " ch " << ch
			 << endl;
		    ++nrr[ch];
		    if( !roerr ) nok = nev[ch];
		    roerr = 1;
		  }
		  hdr = 0;
		  break;

		  // pixel data:
		case 0:
		  raw = d;
		  break;
		case 2:
		  raw = ( raw << 12 ) + d;
		  raw >>= 9;
		  if( Chip < 600 ) {
		    c = ( raw >> 12 ) & 7;
		    c = c * 6 + ( ( raw >> 9 ) & 7 );
		    r = ( raw >> 6 ) & 7;
		    r = r * 6 + ( ( raw >> 3 ) & 7 );
		    r = r * 6 + ( raw & 7 );
		    y = 80 - r / 2;
		    x = 2 * c + ( r & 1 );
		  } // psi46dig
		  else {
		    y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
		    x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
		  } // proc600
		  if( ldb )
		    cout << " " << x << "." << y;
		  if( kroc < 0 || ( L4 && ch == 1 && kroc < 8 ) || kroc > 15 ) {
		    cout << "ROC data with wrong ROC count " << kroc
			 << " in event " << nev[ch]
			 << endl;
		    ++nrr[ch];
		    if( !roerr ) nok = nev[ch];
		    roerr = 1;
		  }
		  else if( y > -1 && y < 80 && x > -1 && x < 52 ) {
		    ++npx;
		    ++npxev;
		    int l = kroc % 8; // 0..7
		    int xm = 52 * l + x; // 0..415  rocs 0 1 2 3 4 5 6 7
		    int ym = y; // 0..79
		    if( kroc > 7 ) {
		      xm = 415 - xm; // rocs 8 9 A B C D E F
		      ym = 159 - y; // 80..159
		    }
		    h21->Fill( xm, ym );
		  } // valid px
		  else {
		    cout << "invalid col row " << setw(3) << x
			 << setw(4) << y
			 << " in ROC " << setw(2) << kroc
			 << endl;
		    ++nrr[ch];
		    if( !roerr ) nok = nev[ch];
		    roerr = 1;
		  }
		  break;

		  // TBM trailer:
		case 14:
		  trl = d;
		  if( L4 && ch == 0 && kroc != 7 ) {
		    cout
		      << "wrong ROC count " << kroc
		      << " in event " << nev[ch] << " ch 0" 
		      << endl;
		    ++nrr[ch];
		    if( !roerr ) nok = nev[ch];
		    roerr = 1;
		  }
		  else if( L4 && ch == 1 && kroc != 15 ) {
		    cout
		      << "wrong ROC count " << kroc
		      << " in event " << nev[ch] << " ch 1" 
		      << endl;
		    ++nrr[ch];
		    if( !roerr ) nok = nev[ch];
		    roerr = 1;
		  }
		  break;
		case 12:
		  trl = ( trl << 8 ) + d;
		  if( ldb ) {
		    cout << endl;
		    cout << "TBM trailer";
		    DecodeTbmTrailer( trl );
		    cout << endl;
		  }
		  ++nev[ch];
		  trl = 1; // flag
		  break;

		default:
		  printf( "\nunknown data: %X = %d", v, v );
		  ++nrr[ch];
		  if( !roerr ) nok = nev[ch];
		  roerr = 1;
		  break;

		} // switch

	      } // data

	      if( ldb )
		cout << endl;

	      cout << s2 - s0 + ( u2 - u0 ) * 1e-6 << " s"
		   << ",  calls " << ndq
		   << ",  last " << data.size()
		   << ",  rest " << remaining
		   << ",  trig " << nev[ch]
		   << ",  byte " << got
		   << ",  pix " << npx
		   << ",  err " << nrr[ch]
		   << endl;

	    } // ch

	    h21->Draw( "colz" );
	    c1->Update();

	  } // while

	  // daq_read for remaining ?

	} // tbmset

  // all off:

  tb.Trigger_Select(4); // back to default PG trigger

  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Stop( ch );
  tb.Flush();

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Close( ch );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // tbmscant

//------------------------------------------------------------------------------
CMD_PROC( takedata ) // takedata period (ROC, trigger f = 40 MHz / period)
// period = 1 = flag for external trigger

// ext trig: event rate = 0.5 trigger rate ? 15.5.2015 TB 22

// source:
// allon
// stretch 1 8 999
// takedata 10000
//
// noise: dac 12 122; allon; takedata 10000
//
// pulses: arm 11 0:33; takedata 999 (few err, rate not uniform)
// arm 0:51 11 = 52 pix = 312 BC = 7.8 us
// readout not synchronized with trigger?
{
  int period;
  PAR_INT( period, 1, 65500 );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // run number from file:

  unsigned int run = 1;
  ifstream irunFile( "runNumber.dat" );
  irunFile >> run;
  irunFile.close();

  ++run;

  ofstream orunFile( "runNumber.dat" );
  orunFile << run;
  orunFile.close();

  // output file:

  ostringstream fname; // output string stream

  fname << "run" << run << ".out";

  ofstream outFile( fname.str().c_str() );

  if( h10 )
    delete h10;
  h10 = new
    TH1D( "pixels",
          "pixels per trigger;multiplicity [pixel];triggers",
          101, -0.5, 100.5 );
  h10->Sumw2();

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "pixelPH", "pixel PH;pixel PH [ADC];pixels",
	  255, -0.5, 254.5 ); // 255 is overflow
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "pixel_charge",
          dacval[0][CtrlReg] == 0 ?
          "pixel charge;pixel charge [small Vcal DAC];pixels" :
          "pixel charge;pixel charge [large Vcal DAC];pixels",
          256, -0.5, 255.5 );
  h12->Sumw2();

  if( h21 )
    delete h21;
  h21 = new TH2D( "HitMap",
                  "Hit map;col;row;hits", 52, -0.5, 51.5, 80, -0.5, 79.5 );

  if( h22 )
    delete h22;
  h22 = new TProfile2D( "PHMap",
                        dacval[0][CtrlReg] == 0 ?
                        "PH map;col;row;<PH> [small Vcal DAC]" :
                        "PH map;col;row;<PH> [large Vcal DAC]",
                        52, -0.5, 51.5, 80, -0.5, 79.5, 0, 500 );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );

  if( period == 1 ) // flag for ext trig
    tb.Pg_Stop(); // stop triggers, necessary for clean data
  else
    tb.Pg_Loop( period ); // not with ext trig

  tb.SetTimeout( 2000 ); // [ms] USB
  tb.Flush();

  uint32_t remaining;
  vector < uint16_t > data;

  unsigned int nev = 0;
  //unsigned int pev = 0; // previous nev
  unsigned int ndq = 0;
  unsigned int got = 0;
  unsigned int rst = 0;
  unsigned int npx = 0;
  unsigned int nrr = 0;

  bool ldb = 0;

  Decoder dec;

  uint32_t NN[52][80] = { {0} };
  //uint32_t PN[52][80] = {{0}}; // previous NN
  uint32_t PH[256] = { 0 };

  double duration = 0;
  double tprev = 0;
  double dttrig = 0;
  double dtread = 0;

  while( !keypressed() ) {

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    tb.mDelay( 500 ); // once per second

    if( period > 1 ) // flag for ext trig
      tb.Pg_Stop(); // stop triggers, necessary for clean data

    tb.uDelay( 100 );

    if( period > 1 ) // flag for ext trig
      tb.Daq_Stop();
    tb.uDelay( 100 );
    //tb.uDelay(4000); // better
    //tb.uDelay(9000); // some overflow events (rest)

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
    dttrig += s2 - s1 + ( u2 - u1 ) * 1e-6;

    tb.Daq_Read( data, Blocksize, remaining );

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

    ++ndq;

    //tb.uDelay(1000);
    if( period > 1 ) // flag for ext trig
      tb.Daq_Start();
    tb.uDelay( 100 );
    if( period > 1 ) // flag for ext trig
      tb.Pg_Loop( period ); // start triggers
    tb.Flush();

    got += data.size();
    rst += remaining;

    gettimeofday( &tv, NULL );
    long s9 = tv.tv_sec; // seconds since 1.1.1970
    long u9 = tv.tv_usec; // microseconds
    duration = s9 - s0 + ( u9 - u0 ) * 1e-6;

    if( duration - tprev > 0.999 ) {

      cout << duration << " s"
	   << ", last " << data.size()
	   << ",  rest " << remaining
	   << ",  daq " << ndq
	   << ",  trg " << nev
	   << ",  sum " << got
	   << ",  rest " << rst
	   << ",  pix " << npx
	   << ",  err " << nrr
	   << ", file " << outFile.tellp() // put pointer [bytes]
	   << endl;

      h21->Draw( "colz" );
      c1->Update();

      tprev = duration;

    }

    // decode data:

    int npxev = 0;
    bool even = 0;
    bool tbm = 0;
    int phase = 0;

    for( unsigned int i = 0; i < data.size(); ++i ) {

      // Simon mail 26.1.2015
      // ext trigger: soft TBM header and trailer

      bool tbmend = 0;

      if( (      data[i] & 0xff00 ) == 0xA000 ) { // TBM header
        even = 0;
	tbm = 1;
      }
      else if( ( data[i] & 0xff00 ) == 0x8000 ) { // TBM header
        even = 0;
	tbm = 1;
	phase = ( data[i] & 0xf ); // phase trig vs clk 0..9
      }
      else if( ( data[i] & 0xff00 ) == 0xE000 ) { // TBM trailer
        even = 0;
      }
      else if( ( data[i] & 0xff00 ) == 0xC000 ) { // TBM trailer
        even = 0;
	tbmend = 1;
      }
      else if( ( data[i] & 0xffc ) == 0x7f8 ) { // ROC header
        if( ldb && i > 0 )
          cout << endl;
        if( ldb )
          printf( "%X", data[i] );
        even = 0;
        npxev = 0;
        ++nev;
      }
      else if( even ) { // 24 data bits come in 2 words

        if( i < data.size() - 1 ) {

          unsigned long raw = ( data[i] & 0xfff ) << 12;
          raw += data[i + 1] & 0xfff; // even + odd

	  ++npx;
          ++npxev;

          dec.Translate( raw );

          uint16_t ix = dec.GetX();
          uint16_t iy = dec.GetY();
          uint16_t ph = dec.GetPH();
	  if( npxev == 1 ) { // non-empty event
	    outFile << nev;
	    if( tbm )
	      outFile << " " << phase;
	  }
	  outFile << " " << ix << " " << iy << " " << ph;

          if( ix < 52 && iy < 80 ) {
	    double vc = PHtoVcal( ph, 0, ix, iy );
	    if( ldb )
	      cout << " " << ix << "." << iy
		   << ":" << ph << "(" << ( int ) vc << ")";
	    h11->Fill( ph );
	    h12->Fill( vc );
	    h21->Fill( ix, iy );
	    h22->Fill( ix, iy, vc );

            ++NN[ix][iy]; // hit map
            if( ph > 0 && ph < 256 )
              ++PH[ph];
          }
          else {
            ++nrr;
	    cout << "err col " << ix << ", row " << iy << endl;
	  }
	}
        else { // truncated pixel
          ++nrr;
	  cout << " err at " << i << " in " << data.size() << endl;
        }

      } // even

      even = 1 - even;

      // $4000 = 0100'0000'0000'0000  FPGA end marker
      // $8000 = 1000'0000'0000'0000  soft TBM header
      // $A000 = 1010'0000'0000'0000  soft TBM header
      // $C000 = 1100'0000'0000'0000  soft TBM trailer
      // $E000 = 1110'0000'0000'0000  soft TBM trailer

      bool lend = 0;
      if( tbm ) {
	if( tbmend )
	  lend = 1;
      }
      else if( ( data[i] & 0x4000 ) == 0x4000 ) // FPGA end marker
	lend = 1;

      if( lend ) {
        h10->Fill( npxev );
	if( npxev > 0 ) outFile << endl;
      }

    } // data

    if( ldb )
      cout << endl;

    // check for ineff in armed pixels:
    /*
      int dev = nev - pev;
      for( int row = 79; row >= 0; --row )
      for( int col = 0; col < 52; ++col ) {
      if( NN[col][row] < nev/2 ) continue; // not armed or noise
      int dn = NN[col][row] - PN[col][row];
      if( dn != dev )
      cout << dev << " events, pix " << setw(2) << col << setw(3) << row
      << " got " << dn << " hits" << endl;
      PN[col][row] = NN[col][row];
      }
      pev = nev;
    */

  } // while takedata

  tb.Daq_Stop();
  if( period > 1 ) // flag for ext trig
    tb.Pg_Stop(); // stop triggers, necessary for clean re-start
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
#endif
  tb.SetTimeout( 200000 ); // restore [ms] USB
  tb.Flush();

  outFile.close(); // redundant

  uint32_t nmx = 0;
  if( npx > 0 ) {
    for( int row = 79; row >= 0; --row ) {
      cout << setw( 2 ) << row << ": ";
      for( int col = 0; col < 52; ++col ) {
	cout << " " << NN[col][row];
        if( NN[col][row] > nmx )
          nmx = NN[col][row];
      }
      cout << endl;
    }

    // increase trim bits:

    cout << endl;
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        if( NN[col][row] > nev / 100 )
          cout << "pixt 0"
	       << setw(3) << col
	       << setw(3) << row
	       << setw(3) << modtrm[0][col][row] + 1
	       << setw(7) << NN[col][row]
	       << endl;
    cout << endl;
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        if( NN[col][row] > nev / 1000 )
          cout << "pixt 0"
	       << setw(3) << col
	       << setw(3) << row
	       << setw(3) << modtrm[0][col][row] + 1
	       << setw(7) << NN[col][row]
	       << endl;
    cout << endl;
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
        if( NN[col][row] > nev / 10000 )
          cout << "pixt 0"
	       << setw(3) << col
	       << setw(3) << row
	       << setw(3) << modtrm[0][col][row] + 1
	       << setw(7) << NN[col][row]
	       << endl;
  }
  cout << "sum " << npx << ", max " << nmx << endl;

  h10->Write();
  h11->Write();
  h12->Write();
  h21->Write();
  h22->Write();
  gStyle->SetOptStat( 10 ); // entries
  gStyle->SetStatY( 0.95 );
  h21->GetYaxis()->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 10, 11, 12, 21, 22" << endl;

  cout << endl;
  cout << fname.str() << endl;
  cout << "duration    " << duration << endl;
  cout << "taking data " << dttrig << endl;
  cout << "readin data " << dtread << endl;
  cout << "triggers    " << nev << " = " << nev / duration << " Hz" << endl;
  cout << "DAQ calls   " << ndq << endl;
  cout << "words read  " << got << ", remaining " << rst << endl;
  cout << "data rate   " << 2 * got / duration << " bytes/s" << endl;
  cout << "pixels      " << npx << " = " << npx /
    ( double ) nev << "/ev " << endl;
  cout << "data errors " << nrr << endl;

  return true;

} // takedata

//------------------------------------------------------------------------------
CMD_PROC( tdscan ) // takedata vs VthrComp: X-ray threshold method

// allon
// stretch 1 8 999
// tdscan 50 150
{
  int vmin;
  PAR_INT( vmin, 0, 255 );
  int vmax;
  PAR_INT( vmax, vmin, 255 );
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 10000;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  if( h10 )
    delete h10;
  h10 = new
    TH1D( "pixels",
          "pixels per trigger;multiplicity [pixel];triggers",
          101, -0.5, 100.5 );
  h10->Sumw2();

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "pixelPH", "pixel PH;pixel PH [ADC];pixels",
	  255, -0.5, 254.5 ); // 255 is overflow
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "pixel_charge",
          dacval[0][CtrlReg] == 0 ?
          "pixel charge;pixel charge [small Vcal DAC];pixels" :
          "pixel charge;pixel charge [large Vcal DAC];pixels",
          256, -0.5, 255.5 );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new
    TH1D( "flux",
	  "flux above threshold;VthrComp [DAC];flux above threshold",
	  vmax-vmin+1, vmin-0.5, vmax+0.5 );
  h13->Sumw2();

  if( h21 )
    delete h21;
  h21 = new TH2D( "HitMap",
                  "Hit map;col;row;hits", 52, -0.5, 51.5, 80, -0.5, 79.5 );

  if( h22 )
    delete h22;
  h22 = new TProfile2D( "PHMap",
                        dacval[0][CtrlReg] == 0 ?
                        "PH map;col;row;<PH> [small Vcal DAC]" :
                        "PH map;col;row;<PH> [large Vcal DAC]",
                        52, -0.5, 51.5, 80, -0.5, 79.5, 0, 500 );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.Pg_Stop(); // stop triggers (who knows what was going on before?)
  tb.SetTimeout( 2000 ); // [ms] USB
  tb.Flush();

  double dttrig = 0;
  double dtread = 0;

  vector < uint16_t > data;
  uint32_t remaining;

  Decoder dec;

  // threshold scan:

  for( int vthr = vmin; vthr <= vmax; ++vthr ) {

    tb.SetDAC( VthrComp, vthr );

    // set CalDel? not needed for random hits with clock stretch

    tb.uDelay( 10000 );
    tb.Flush();

    // take data:

    tb.Daq_Start();
    tb.uDelay( 1000 );

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    for( int k = 0; k < nTrig; ++k ) {
      tb.Pg_Single();
      tb.uDelay( 20 );
    }
    tb.Flush();

    tb.Daq_Stop();
    tb.uDelay( 100 );
    tb.Daq_GetSize(); // read statement ends trig block for timer

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
    double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;
    dttrig += s2 - s1 + ( u2 - u1 ) * 1e-6;

    tb.Daq_Read( data, Blocksize, remaining );

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

    // decode data:

    unsigned int nev = 0;
    unsigned int npx = 0;
    unsigned int nrr = 0;
    unsigned int npxev = 0;
    bool even = 1;

    for( unsigned int i = 0; i < data.size(); ++i ) {

      if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
        even = 0;
        npxev = 0;
        ++nev;
      }
      else if( even ) { // 24 data bits come in 2 words

        if( i < data.size() - 1 ) {

          ++npx;
          ++npxev;

          unsigned long raw = ( data[i] & 0xfff ) << 12;
          raw += data[i + 1] & 0xfff; // even + odd

          dec.Translate( raw );

          uint16_t ix = dec.GetX();
          uint16_t iy = dec.GetY();
          uint16_t ph = dec.GetPH();
          double vc = PHtoVcal( ph, 0, ix, iy );
          h11->Fill( ph );
          h12->Fill( vc );
          h13->Fill( vthr );
          h21->Fill( ix, iy );
          h22->Fill( ix, iy, vc );
          if( ix > 51 || iy > 79 || ph > 255 )
            ++nrr;
        }
        else {
          ++nrr;
        }

      } // even

      even = 1 - even;

      if( ( data[i] & 0x4000 ) == 0x4000 ) { // FPGA end marker
        h10->Fill( npxev );
      }

    } // data

    cout
      << "Vthr " << setw(3) << vthr
      << ", dur " << dt+dtr
      << ", size " << data.size()
      << ",  rest " << remaining
      << ",  events " << nev
      << ",  pix " << npx
      << ",  err " << nrr
      << endl;

    h21->Draw( "colz" );
    c1->Update();

  } // Vthr loop

  tb.Daq_Stop();
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
#endif
  tb.SetDAC( VthrComp, dacval[0][VthrComp] ); // restore
  tb.SetTimeout( 200000 ); // restore [ms] USB
  tb.Flush();

  h10->Write();
  h11->Write();
  h12->Write();
  h13->Write();
  h21->Write();
  h22->Write();
  cout << "  histos 10, 11, 12, 13, 21, 22" << endl;

  h13->Draw( "hist" );
  c1->Update();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // tdscan

//------------------------------------------------------------------------------
CMD_PROC( onevst ) // pulse one pixel vs time
{
  int32_t col;
  int32_t row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  int period;
  PAR_INT( period, 1, 65500 );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  uint32_t remaining;
  vector < uint16_t > data;

  unsigned int nev = 0;
  //unsigned int pev = 0; // previous nev
  unsigned int ndq = 0;
  unsigned int got = 0;
  unsigned int rst = 0;
  unsigned int npx = 0;
  unsigned int nrr = 0;

  bool ldb = 0;

  Decoder dec;

  double duration = 0;
  double tprev = 0;

  if( h11 )
    delete h11;
  h11 = new
    TProfile( "PH_vs_time",
              "PH vs time;time [s];<PH> [ADC]", 300, 0, 300, 0, 255 );
  h11->Sumw2();

  tb.roc_Col_Enable( col, true );
  int trim = modtrm[0][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.roc_Pix_Cal( col, row, false );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );
  tb.Pg_Loop( period );

  tb.SetTimeout( 2000 ); // [ms] USB
  tb.Flush();

  int nsum = 0;
  double phsum = 0;

  while( !keypressed() ) {

    tb.uDelay( 1000 ); // limit daq rate

    tb.Pg_Stop(); // stop triggers, necessary for clean data

    tb.uDelay( 100 );

    tb.Daq_Stop();
    tb.uDelay( 100 );

    tb.Daq_Read( data, Blocksize, remaining );

    ++ndq;
    got += data.size();
    rst += remaining;

    //tb.uDelay(1000);
    tb.Daq_Start();
    tb.uDelay( 100 );
    tb.Pg_Loop( period ); // start triggers
    tb.Flush();

    gettimeofday( &tv, NULL );
    long s9 = tv.tv_sec; // seconds since 1.1.1970
    long u9 = tv.tv_usec; // microseconds
    duration = s9 - s0 + ( u9 - u0 ) * 1e-6;

    if( duration - tprev > 0.999 ) {

      double phavg = 0;
      if( nsum > 0 )
        phavg = phsum / nsum;

      cout << duration << " s"
	   << ", last " << data.size()
	   << ",  rest " << remaining
	   << ",  calls " << ndq
	   << ",  events " << nev
	   << ",  got " << got
	   << ",  rest " << rst << ",  pix " << npx << ",  ph " << phavg << endl;

      h11->Draw( "hist" );
      c1->Update();

      nsum = 0;
      phsum = 0;

      tprev = duration;

    }

    // decode data:

    bool even = 1;

    for( unsigned int i = 0; i < data.size(); ++i ) {

      if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
        if( ldb && i > 0 )
          cout << endl;
        if( ldb )
          printf( "%X", data[i] );
        even = 0;
        ++nev;
      }
      else if( even ) { // 24 data bits come in 2 words

        if( i < data.size() - 1 ) {

          unsigned long raw = ( data[i] & 0xfff ) << 12;
          raw += data[i + 1] & 0xfff; // even + odd

          dec.Translate( raw );

          uint16_t ix = dec.GetX();
          uint16_t iy = dec.GetY();
          uint16_t ph = dec.GetPH();
          double vc = PHtoVcal( ph, 0, ix, iy );
          if( ldb )
            cout << " " << ix << "." << iy
		 << ":" << ph << "(" << ( int ) vc << ")";
          if( ix == col && iy == row ) {
            h11->Fill( duration, ph );
            ++nsum;
            phsum += ph;
          }
          else
            ++nrr;
          ++npx;
        }
        else {
          ++nrr;
        }

      } // even

      even = 1 - even;

    } // data

    if( ldb )
      cout << endl;

  } // while takedata

  tb.Daq_Stop();
  tb.Pg_Stop(); // stop triggers, necessary for clean re-start
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
#endif
  tb.SetTimeout( 200000 ); // restore [ms] USB

  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  h11->Write();
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  cout << endl;
  cout << "duration    " << duration << endl;
  cout << "events      " << nev << " = " << nev / duration << " Hz" << endl;
  cout << "DAQ calls   " << ndq << endl;
  cout << "words read  " << got << ", remaining " << rst << endl;
  cout << "data rate   " << 2 * got / duration << " bytes/s" << endl;
  cout << "pixels      " << npx << " = " << npx /
    ( double ) nev << "/ev " << endl;
  cout << "data errors " << nrr << endl;

  return true;

} // onevst

//------------------------------------------------------------------------------
CMD_PROC( modtd ) // module take data (trigger f = 40 MHz / period)
// period = 1 = external trigger = two streams
// pulses:
//   modcaldel 44 66
//   arm 44 66
//   modtd 4000
// source or X-rays:
//   stretch does not work with modules
//   smr (caldel not needed for random triggers)
//   tdx (contains allon, modtd 4000)
{
  int period;
  PAR_INT( period, 1, 65500 ); // f = 40 MHz / period

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // run number from file:

  unsigned int run = 1;
  ifstream irunFile( "runNumber.dat" );
  irunFile >> run;
  irunFile.close();

  ++run;

  ofstream orunFile( "runNumber.dat" );
  orunFile << run;
  orunFile.close();

  // output file:

  ostringstream fname; // output string stream

  if( period == 1 )
    fname << "run" << run << "A.out";
  else
    fname << "run" << run << ".out";
  cout <<  "run" << run << ".out" << endl;

  ofstream outFileA( fname.str().c_str() );

  fname.str("");
  fname << "run" << run << "B.out";
  ofstream outFileB( fname.str().c_str() );

  unsigned int nev[8] = {  };
  unsigned int ndq = 0;
  unsigned int got = 0;
  unsigned int rst = 0;
  unsigned int npx = 0;
  unsigned int nrr[8] = {  };
  unsigned int nth[8] = {  }; // TBM headers
  unsigned int nrh[8] = {  }; // ROC headers
  unsigned int ntt[8] = {  }; // TBM trailers
  unsigned int nfl[8] = {  }; // filled events

  int PX[16] = {  };
  int ER[16] = {  };
  uint32_t NN[16][52][80] = { {{0}} };
  uint32_t PH[16][52][80] = { {{0}} };

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "pixels",
          "pixels per trigger;pixel hits;triggers", 65, -0.5, 64.5 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "ADC",
          "PH ADC spectrum;PH [ADC];hits", 255, -0.5, 255.5 );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new
    TH1D( "Vcal",
          "PH Vcal spectrum;PH [Vcal DAC];hits", 255, -0.5, 255.5 );
  h13->Sumw2();

  if( h21 )
    delete h21;
  //int n = 4; // fat pixels
  //int n = 2; // big pixels
  int n = 1; // pixels
  h21 = new TH2D( "ModuleHitMap",
                  "Module hit map;col;row;hits",
                  8 * 52 / n, -0.5 * n, 8 * 52 - 0.5 * n,
                  2 * 80 / n, -0.5 * n, 2 * 80 - 0.5 * n );
  gStyle->SetOptStat( 10 ); // entries
  h21->GetYaxis()->SetTitleOffset( 1.3 );

  if( h22 )
    delete h22;
  h22 = new TH2D( "ModulePHmap",
                  "Module PH map;col;row;sum PH [ADC]",
                  8 * 52 / n, -0.5 * n, 8 * 52 - 0.5 * n,
                  2 * 80 / n, -0.5 * n, 2 * 80 - 0.5 * n );

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Open( Blocksize, ch );
#endif

  tb.Daq_Select_Deser400();
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Start( ch );
  tb.uDelay( 1000 );

  if( period > 1 ) // random trigger
    tb.Pg_Loop( period ); // start triggers

  tb.Flush();

  uint32_t trl = 0; // need to remember from previous daq_read

  while( !keypressed() ) {

    //tb.uDelay( 50000 ); // [us] limit daq rate, more efficient
    //tb.uDelay(  5000 ); // 2 kpx/s Sr 90 no Makrolon
    //tb.uDelay( 10000 ); // 4 kpx/s Sr 90 no Makrolon
    //tb.uDelay( 20000 ); // 5.1 kpx/s Sr 90 no Makrolon
    //tb.uDelay( 40000 ); // 5.6 kpx/s Sr 90 no Makrolon
    tb.mDelay( 500 ); // once per second
    //tb.mDelay( 200 ); // 11.12.2015 readout error chasing

    if( period > 1 ) // random trigger
      tb.Pg_Stop(); // stop triggers, necessary for clean data

    if( period > 1 ) { // random trigger
      for( int ch = 0; ch < nChannels; ++ch )
	tb.Daq_Stop( ch );
    }

    ++ndq;

    vector < uint16_t > data[8];
    uint32_t remaining = 0;

    for( int ch = 0; ch < nChannels; ++ch ) {
      data[ch].reserve( Blocksize );
      tb.Daq_Read( data[ch], Blocksize, remaining, ch );
      got += data[ch].size();
      rst += remaining;
    }

    if( period > 1 ) { // random trigger
      tb.Daq_Deser400_Reset( 3 ); // more stable ?
      for( int ch = 0; ch < nChannels; ++ch )
	tb.Daq_Start( ch );

      tb.Pg_Loop( period ); // start triggers
      tb.Flush();
    }

    bool ldb = 0;
    //if( ndq == 1 ) ldb = 1;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds

    h21->Draw( "colz" );
    c1->Update();

    cout << s2 - s0 + ( u2 - u0 ) * 1e-6 << " s"
	 << ",  calls " << ndq
	 << ",  last " << data[0].size() + data[1].size()
	 << ",  rest " << remaining
	 << ",  trig " << nev[0]
	 << ",  byte " << got
	 << ",  rest " << rst
	 << ",  pix " << npx
	 << ",  err " << nrr[0] + nrr[1]
	 << endl;

    // decode data:

    for( int ch = 0; ch < nChannels; ++ch ) {

      int size = data[ch].size();
      uint32_t raw = 0;
      uint32_t hdr = 0;
      int32_t kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
      unsigned int npxev = 0;

      int iiprev = 0;

      for( int ii = 0; ii < size; ++ii ) {

	int d = 0;
	int v = 0;
	int xflg = 0;
	d = data[ch][ii] & 0xfff; // 12 bits data
	v = ( data[ch][ii] >> 12 ) & 0xe; // 3 flag bits: e = 14 = 1110
	xflg = ( data[ch][ii] >> 12 ) & 0x1; // DESER400 error flag
	if( xflg && Chip < 700 ) {
	  cout << "DESER400 error at case " << v
	       << " in event " << nev[ch]
	       << " ch " << ch
	       << endl;
	    ++nrr[ch];
	}
        uint32_t ph = 0;
        int c = 0;
        int r = 0;
        int x = 0;
        int y = 0;

        switch ( v ) {

	  // TBM header:
        case 10:
          hdr = d;
          ++nth[ch];
          npxev = 0;
	  if( period == 1 ) { // ext trig
	    if( ch == 0 )
	      outFileA << nev[ch]; // need every event
	    else
	      outFileB << nev[ch]; //  for multi-module beam test
	  }
	  if( nev[ch] > 0 && trl == 0 ) {
	    cout << "TBM error: header without previous trailer in event "
		 << nev[ch]
		 << " ch " << ch
		 << " pos " << ii
		 << endl;
	    ++nrr[ch];
	  }
          trl = 0;
	  iiprev = ii;
          break;
        case 8:
          hdr = ( hdr << 8 ) + d;
          if( ldb ) {
            cout << "event " << nev[ch] << endl;
            cout << "TBM header";
            DecodeTbmHeader( hdr );
            cout << endl;
          }
          kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
	  if( L1 ) kroc = ch2roc[ch] - 1;
          break;

	  // ROC header data:
        case 4:
          ++kroc; // start at 0 or 8
          ++nrh[ch];
          if( ldb ) {
            if( kroc > 0 )
              cout << endl;
            cout << "ROC " << setw( 2 ) << kroc;
          }
          if( kroc > 15 || ( ch == 0 && kroc > 7 ) ) {
            cout << "Error kroc " << kroc << " in event " << nev[ch]
		 << " ch " << ch
		 << " pos " << ii
		 << endl;
	    if( ch )
	      kroc = 15;
	    else
	      kroc = 7;
	    ++nrr[ch];
	    cout << "    this event:";
	    for( int jj = iiprev; jj <= ii; ++jj )
	      cout << " " << hex << data[ch][jj];
	    cout << dec << endl;
          }
          if( kroc == 0 && hdr == 0 ) {
            cout << "TBM error: no header in event " << nev[ch]
		 << " ch " << ch
		 << " pos " << ii
		 << endl;
	    ++nrr[ch];
	  }
          hdr = 0;
          break;

	  // pixel data:
        case 0:
          raw = d;
          break;
        case 2:
          raw = ( raw << 12 ) + d;
	  //DecodePixel(raw);
          ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
	  h12->Fill( ph );
          raw >>= 9;
	  if( Chip < 600 ) {
	    c = ( raw >> 12 ) & 7;
	    c = c * 6 + ( ( raw >> 9 ) & 7 );
	    r = ( raw >> 6 ) & 7;
	    r = r * 6 + ( ( raw >> 3 ) & 7 );
	    r = r * 6 + ( raw & 7 );
	    y = 80 - r / 2;
	    x = 2 * c + ( r & 1 );
	  } // psi46dig
	  else {
	    y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
	    x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
	  } // proc600
          if( ldb )
            cout << " " << x << "." << y << ":" << ph;
	  if( kroc < 0 || ( L4 && ch == 1 && kroc < 8 ) || kroc > 15 ) {
	    cout << "ROC data with wrong ROC count " << kroc
		 << " in event " << nev[ch]
		 << endl;
	    ++nrr[ch];
	  }
	  else if( y > -1 && y < 80 && x > -1 && x < 52 ) {
	    ++npx;
	    ++npxev;
	    ++PX[kroc];
            NN[kroc][x][y] += 1;
            PH[kroc][x][y] += ph;
	    int l = kroc % 8; // 0..7
	    int xm = 52 * l + x; // 0..415  rocs 0 1 2 3 4 5 6 7
	    int ym = y; // 0..79
	    if( kroc > 7 ) {
	      xm = 415 - xm; // rocs 8 9 A B C D E F
	      ym = 159 - y; // 80..159
	    }
	    double vc = PHtoVcal( ph, kroc, x, y );
	    h13->Fill( vc );
	    h21->Fill( xm, ym );
	    h22->Fill( xm, ym, vc );
	    if( period > 1 && npxev == 1 ) outFileA << nev[ch]; // non-empty event
	    if( ch == 0 || period > 1 )
	      outFileA << " " << xm << " " << ym << " " << ph;
	    else
	      outFileB << " " << xm << " " << ym << " " << ph;
	  } // valid px
	  else {
	    ++nrr[ch];
	    ++ER[kroc];
	    if( ldb )
	      cout << "invalid col row " << setw(3) << x
		   << setw(4) << y
		   << " in ROC " << setw(2) << kroc
		   << endl;
	  }
          break;

	  // TBM trailer:
        case 14:
          trl = d;
	  if( L4 && ch == 0 && kroc != 7 ) {
	    cout
	      << "wrong ROC count " << kroc
	      << " in event " << nev[ch] << " ch 0" 
	      << " pos " << ii
	      << endl;
	    ++nrr[ch];
	  }
	  else if( L4 && ch == 1 && kroc != 15 ) {
	    cout
	      << "wrong ROC count " << kroc
	      << " in event " << nev[ch] << " ch 1" 
	      << " pos " << ii
	      << endl;
	    ++nrr[ch];
	  }
          break;

        case 12:
          trl = ( trl << 8 ) + d;
          if( ldb ) {
            cout << endl;
            cout << "TBM trailer";
            DecodeTbmTrailer( trl );
            cout << endl;
          }
          ++nev[ch];
          ++ntt[ch];
	  if( npxev > 0 ) ++nfl[ch];
          h11->Fill( npxev );
	  if( period > 1 && npxev > 0 ) outFileA << endl;
	  if( period == 1 ) {
	    if( ch == 0 )
	      outFileA << endl;
	    else
	      outFileB << endl;
	  }
	  trl = 1; // flag
          break;

        default:
          printf( "\nunknown data: %X = %d", v, v );
	  ++nrr[ch];
          break;

        } // switch

      } // data

    } // ch

    if( ldb )
      cout << endl;

  } // while

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  double dt = s9 - s0 + ( u9 - u0 ) * 1e-6;

  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Stop( ch );
  tb.Pg_Stop(); // stop triggers, necessary for clean re-start
  //tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();

  outFileA.close(); // redundant
  if( period == 1 )
    outFileB.close(); // redundant
  h11->Write();
  h12->Write();
  h13->Write();
  h21->Write();
  h22->Write();
  h21->Draw( "colz" );
  c1->Update();

  cout << endl;
  cout << "run       " << run << endl;
  for( int roc = 0; roc < 16; ++roc )
    cout << "ROC " << setw( 2 ) << roc
	 << ", hits " << PX[roc]
	 << ", err " << ER[roc]
	 << endl;

  cout << "DAQ calls " << ndq << endl;
  cout << "written to " << fname.str() << endl;
  cout << "duration  " << dt << " s" << endl;
  cout << "triggers  " << nev[0] << " = " << nev[0]/dt << " Hz" << endl;

  cout << "Ch 0: TBM headers " << nth[0]
       << ", trailers " << ntt[0]
       << ", ROCs " << nrh[0]
       << ", errors " << nrr[0]
       << endl;
  
  cout << "Ch 1: TBM headers " << nth[1]
       << ", trailers " << ntt[1]
       << ", ROCs " << nrh[1]
       << ", errors " << nrr[1]
       << endl;

  cout << endl;
  int allact = 0;
  for( int roc = 0; roc < 16; ++roc ) {
    int act = 0;
    int hotn = 0;
    int hotx = 0;
    int hoty = 0;
    for( int x = 0; x < 52; ++x )
      for( int y = 0; y < 80; ++y ) {
	int n = NN[roc][x][y];
	if( n ) ++act;
	if( n > hotn ) {
	  hotn = n;
	  hotx = x;
	  hoty = y;
	}
      }
    cout << "ROC " << setw( 2 ) << roc
	 << ": err " << setw(4) << ER[roc]
	 << ", hit " << setw(6 ) << PX[roc]
	 << ", pix " << setw(4) << act
	 << ", hot " << setw(2) << hotx << setw(3) << hoty
	 << " " << hotn
	 << endl;
    allact += act;
  } // ROCs

  cout << "hits " << npx
       << " = " << ( double ) npx / dt << "/s"
       << " = " << ( double ) npx / max(1,int(nev[0])) << "/event"
       << endl;

  cout << "active pixels " << allact << endl;

  cout << "histos 11, 12, 13, 21, 22" << endl;

  return true;

} // modtd

//------------------------------------------------------------------------------
CMD_PROC( showclk )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  const unsigned int nSamples = 20;
  const int gain = 1;
  // PAR_INT( gain, 1, 4 );

  unsigned int i, k;
  vector < uint16_t > data[20];

  tb.Pg_Stop();
  /* DP
  tb.Pg_SetCmd( 0, PG_SYNC + 5 );
  tb.Pg_SetCmd( 1, PG_CAL + 6 );
  tb.Pg_SetCmd( 2, PG_TRG + 6 );
  tb.Pg_SetCmd( 3, PG_RESR + 6 );
  tb.Pg_SetCmd( 4, PG_REST + 6 );
  tb.Pg_SetCmd( 5, PG_TOK );
  */
  tb.SignalProbeD1( 9 );
  tb.SignalProbeD2( 17 );
  tb.SignalProbeA2( PROBEA_CLK );
  tb.uDelay( 10 );
  tb.SignalProbeADC( PROBEA_CLK, gain - 1 );
  tb.uDelay( 10 );

  tb.Daq_Select_ADC( nSamples, 1, 1 );
  tb.uDelay( 1000 );
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  for( i = 0; i < 20; ++i ) {
    tb.Sig_SetDelay( SIG_CLK, 26 - i );
    tb.uDelay( 10 );
    tb.Daq_Start();
    tb.Pg_Single();
    tb.uDelay( 1000 );
    tb.Daq_Stop();
    tb.Daq_Read( data[i], 1024 );
    if( data[i].size() != nSamples ) {
      printf( "Data size %i: %i\n", i, int ( data[i].size() ) );
      return true;
    }
    //cout << 26-i << "  " << data[i].size() << endl;
  }

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
#endif
  tb.Flush();

  int n = 20 * nSamples;
  vector < double > values( n );
  int x = 0;
  for( k = 0; k < nSamples; ++k ) {
    for( i = 0; i < 20; ++i ) {
      int y = ( data[i] )[k] & 0x0fff;
      if( y & 0x0800 )
        y |= 0xfffff000;
      values[x++] = y;
      cout << setw(5) << y;
    }
    cout << endl;
  }

  //Scope( "CLK", values);

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( showctr )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  const unsigned int nSamples = 60;
  const int gain = 1;
  // PAR_INT( gain, 1, 4 );

  unsigned int i, k;
  vector < uint16_t > data[20];

  tb.Pg_Stop();
  /* DP
  tb.Pg_SetCmd( 0, PG_SYNC + 5 );
  tb.Pg_SetCmd( 1, PG_CAL + 6 );
  tb.Pg_SetCmd( 2, PG_TRG + 6 );
  tb.Pg_SetCmd( 3, PG_RESR + 6 );
  tb.Pg_SetCmd( 4, PG_REST + 6 );
  tb.Pg_SetCmd( 5, PG_TOK );
  */
  tb.SignalProbeD1( 9 );
  tb.SignalProbeD2( 17 );
  tb.SignalProbeA2( PROBEA_CTR );
  tb.uDelay( 10 );
  tb.SignalProbeADC( PROBEA_CTR, gain - 1 );
  tb.uDelay( 10 );

  tb.Daq_Select_ADC( nSamples, 1, 1 );
  tb.uDelay( 1000 );
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  for( i = 0; i < 20; ++i ) {
    tb.Sig_SetDelay( SIG_CTR, 26 - i );
    tb.uDelay( 10 );
    tb.Daq_Start();
    tb.Pg_Single();
    tb.uDelay( 1000 );
    tb.Daq_Stop();
    tb.Daq_Read( data[i], 1024 );
    if( data[i].size() != nSamples ) {
      printf( "Data size %i: %i\n", i, int ( data[i].size() ) );
      return true;
    }
  }
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
#endif
  tb.Flush();

  int n = 20 * nSamples;
  vector < double > values( n );
  int x = 0;
  for( k = 0; k < nSamples; ++k ) {
    for( i = 0; i < 20; ++i ) {
      int y = ( data[i] )[k] & 0x0fff;
      if( y & 0x0800 )
        y |= 0xfffff000;
      values[x++] = y;
      cout << setw(5) << y;
    }
    cout << endl;
  }

  //Scope( "CTR", values);

  /*
    FILE *f = fopen( "X:\\developments\\adc\\data\\adc.txt", "wt" );
    if( !f) { printf( "Could not open File!\n" ); return true; }
    double t = 0.0;
    for( k=0; k<100; ++k) for( i=0; i<20; ++i)
    {
    int x = (data[i])[k] & 0x0fff;
    if( x & 0x0800) x |= 0xfffff000;
    fprintf(f, "%7.2f %6i\n", t, x);
    t += 1.25;
    }
    fclose(f);
  */
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( showsda )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  const unsigned int nSamples = 52;
  unsigned int i, k;
  vector < uint16_t > data[20];

  tb.SignalProbeD1( 9 );
  tb.SignalProbeD2( 17 );
  tb.SignalProbeA2( PROBEA_SDA );
  tb.uDelay( 10 );
  tb.SignalProbeADC( PROBEA_SDA, 0 );
  tb.uDelay( 10 );

  tb.Daq_Select_ADC( nSamples, 2, 7 );
  tb.uDelay( 1000 );
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  for( i = 0; i < 20; ++i ) {
    tb.Sig_SetDelay( SIG_SDA, 26 - i );
    tb.uDelay( 10 );
    tb.Daq_Start();
    tb.roc_Pix_Trim( 12, 34, 5 );
    tb.uDelay( 1000 );
    tb.Daq_Stop();
    tb.Daq_Read( data[i], 1024 );
    if( data[i].size() != nSamples ) {
      printf( "Data size %i: %i\n", i, int ( data[i].size() ) );
      return true;
    }
  }
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
#endif
  tb.Flush();

  int n = 20 * nSamples;
  vector < double > values( n );
  int x = 0;
  for( k = 0; k < nSamples; ++k ) {
    for( i = 0; i < 20; ++i ) {
      int y = ( data[i] )[k] & 0x0fff;
      if( y & 0x0800 )
        y |= 0xfffff000;
      values[x++] = y;
      cout << setw(5) << y;
    }
    cout << endl;
  }

  //Scope( "SDA", values);

  return true;
}

//------------------------------------------------------------------------------
// utility function
void decoding_show2( vector < uint16_t > &data ) // bit-wise print
{
  int size = data.size();
  if( size > 6 )
    size = 6;
  for( int i = 0; i < size; ++i ) {
    uint16_t x = data[i];
    for( int k = 0; k < 12; ++k ) { // 12 valid bits per word
      Log.puts( ( x & 0x0800 ) ? "1" : "0" );
      x <<= 1;
      if( k == 3 || k == 7 || k == 11 )
        Log.puts( " " );
    }
  }
}

//------------------------------------------------------------------------------
// utility function
void decoding_show( vector < uint16_t > *data )
{
  int i;
  for( i = 1; i < 16; ++i )
    if( data[i] != data[0] )
      break;
  decoding_show2( data[0] );
  Log.puts( "  " );
  if( i < 16 )
    decoding_show2( data[i] );
  else
    Log.puts( " no diff" );
  Log.puts( "\n" );
}

//------------------------------------------------------------------------------
CMD_PROC( decoding )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  unsigned short t;
  vector < uint16_t > data[16];
  tb.Pg_SetCmd( 0, PG_TOK + 0 );
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 10 );
  Log.section( "decoding" );
#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  for( t = 0; t < 44; ++t ) {
    tb.Sig_SetDelay( SIG_CLK, t );
    tb.Sig_SetDelay( SIG_TIN, t + 5 );
    tb.uDelay( 10 );
    for( int i = 0; i < 16; ++i ) {
      tb.Daq_Start();
      tb.Pg_Single();
      tb.uDelay( 200 );
      tb.Daq_Stop();
      tb.Daq_Read( data[i], 200 );
    }
    Log.printf( "%3i ", int ( t ) );
    decoding_show( data );
  }
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
#endif
  Log.flush();

  return true;
}

// -- Wafer Test Adapter commands ----------------------------------------
/*
  CMD_PROC(vdreg)    // regulated VD
  {
  double v = tb.GetVD_Reg();
  printf( "\n VD_reg = %1.3fV\n", v);
  return true;
  }

  CMD_PROC(vdcap)    // unregulated VD for contact test
  {
  double v = tb.GetVD_CAP();
  printf( "\n VD_cap = %1.3fV\n", v);
  return true;
  }

  CMD_PROC(vdac)     // regulated VDAC
  {
  double v = tb.GetVDAC_CAP();
  printf( "\n V_dac = %1.3fV\n", v);
  return true;
  }
*/

//------------------------------------------------------------------------------
CMD_PROC( tbmdis )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  tb.tbm_Enable( false );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tbmsel ) // normal: tbmsel 31 6
{
  int hub, port;
  PAR_INT( hub, 0, 31 );
  PAR_INT( port, 0, 6 ); // port 0 = ROC 0-3, 1 = 4-7, 2 = 8-11, 3 = 12-15, 6 = 0-15
  tb.tbm_Enable( true );
  tb.tbm_Addr( hub, port );
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( modsel )
{
  int hub;
  PAR_INT( hub, 0, 31 );
  tb.tbm_Enable( true );
  tb.mod_Addr( hub );
  cout << "modsel hub ID " << hub << endl;
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( modsel2 ) // select L1 module
{
  int hub;
  PAR_INT( hub, 1, 31 );
  tb.tbm_Enable( true );
  tb.mod_Addr( hub, hub-1 ); // for L1 modules
  DO_FLUSH;
  L1 = 1; // for L1 modules
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tbmset )
{
  int reg, value;
  PAR_INT( reg, 0, 255 );
  PAR_INT( value, 0, 255 );
  tb.tbm_Set( reg, value );
  DO_FLUSH;
  //cout << "tbm_Set reg " << reg << " (" << int(reg) << ") with " << value << endl;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tbmget )
{
  int reg;
  PAR_INT( reg, 0, 255 );

  cout << "tbm_Get reg " << reg << " (" << int(reg) << ")" << endl;
  unsigned char value;
  if( tb.tbm_Get( reg, value ) ) {
    printf( " reg 0x%02X = %3i (0x%02X)\n", reg, (int)value, (int)value);
  }
  else
    puts( " error\n" );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( tbmgetraw ) // 
{
  int reg;
  PAR_INT( reg, 0, 255 );

  uint32_t value;

  tb.Sig_SetLCDS(); // TBM sends RDA as LCDS, ROC sends token as LVDS

  for( int idel = 0; idel < 19; ++idel ) {
    tb.Sig_SetRdaToutDelay( idel ); // 0..19
    if( tb.tbm_GetRaw( reg, value ) ) {
      printf( "value = 0x%02X (Hub=%2i; Port=%i; Reg=0x%02X; inv=0x%X; stop=%c)\n",
	      value & 0xff, (value>>19) & 0x1f, (value>>16) & 0x07,
	      ( value>>8) & 0xff, (value>>25) & 0x1f, (value & 0x1000) ? '1' : '0' );
    }
    else
      puts( "error\n" );
  }
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( dac ) // e.g. dac 25 222 [8]
{
  int addr, value;
  PAR_INT( addr, 1, 255 );
  PAR_INT( value, 0, 255 );

  int rocmin, rocmax;
  if( !PAR_IS_INT( rocmin, 0, 15 ) )  { // no user input
    rocmin = 0;
    rocmax = 15;
  }
  else
    rocmax = rocmin; // one ROC

  for( int roc = rocmin; roc <= rocmax; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.SetDAC( addr, value ); // set corrected
      dacval[roc][addr] = value;
      Log.printf( "[SETDAC] %i  %i\n", addr, value );
    }

  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
void SetDACallROCs( int dac, int val )
{
  if( dac < 1 ) return;
  if( dac > 255 ) return;
  if( val < 0 ) return;
  if( val > 255 ) return;

  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.SetDAC( dac, val ); // set corrected
      dacval[roc][dac] = val;
    }
  Log.printf( "[SETDAC] %i  %i\n", dac, val );
  cout << setw(3) << dac << " " << dacName[dac] << "\t" << val << endl;
}

//------------------------------------------------------------------------------
CMD_PROC( vdig )
{
  int value;
  PAR_INT( value, 0, 15 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.SetDAC( Vdig, value );
      dacval[roc][Vdig] = value;
      Log.printf( "[SETDAC] %i  %i\n", Vdig, value );
    }

  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vana )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.SetDAC( Vana, value );
      dacval[roc][Vana] = value;
      Log.printf( "[SETDAC] %i  %i\n", Vana, value );
    }

  DO_FLUSH;
  tb.mDelay( 500 );
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vtrim )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.SetDAC( Vtrim, value );
      dacval[roc][Vtrim] = value;
      Log.printf( "[SETDAC] %i  %i\n", Vtrim, value );
    }

  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vthr )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.SetDAC( VthrComp, value );
      dacval[roc][VthrComp] = value;
      Log.printf( "[SETDAC] %i  %i\n", VthrComp, value );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( subvthr ) // subtract from VthrComp (make it harder)(or softer)
{
  int sub;
  PAR_INT( sub, -255, 255 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      int vthr = dacval[roc][VthrComp];
      cout << "ROC " << setw(2) << roc << " VthrComp from " << setw(3) << vthr;
      vthr -= sub;
      if( vthr < 0 )
	vthr = 0;
      else if( vthr > 255 )
	vthr = 255;
      tb.SetDAC( VthrComp, vthr );
      dacval[roc][VthrComp] = vthr;
      Log.printf( "[SETDAC] %i  %i\n", VthrComp, vthr );
      cout << " to " << setw(3) << vthr << endl;
    } // roc
  DO_FLUSH;
  return true;

} // subvthr

//------------------------------------------------------------------------------
CMD_PROC( addvtrim ) // add to Vtrim
{
  int add;
  PAR_INT( add, -255, 255 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      int vtrim = dacval[roc][Vtrim];
      cout << "ROC " << setw(2) << roc << " Vtrim from " << setw(3) << vtrim;
      vtrim += add;
      if( vtrim > 255 )
	vtrim = 255;
      else if( vtrim < 0 )
	vtrim = 0;
      tb.SetDAC( Vtrim, vtrim );
      dacval[roc][Vtrim] = vtrim;
      Log.printf( "[SETDAC] %i  %i\n", Vtrim, vtrim );
      cout << " to " << setw(3) << vtrim << endl;
    } // roc
  DO_FLUSH;
  return true;

} // addvtrim

//------------------------------------------------------------------------------
CMD_PROC( vcal )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.SetDAC( Vcal, value );
      dacval[roc][Vcal] = value;
      Log.printf( "[SETDAC] %i  %i\n", Vcal, value );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( ctl )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.SetDAC( CtrlReg, value );
      dacval[roc][CtrlReg] = value;
      Log.printf( "[SETDAC] %i  %i\n", CtrlReg, value );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( wbc )
{
  int value;
  PAR_INT( value, 0, 255 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.SetDAC( WBC, value );
      dacval[roc][WBC] = value;
      Log.printf( "[SETDAC] %i  %i\n", WBC, value );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
bool setia( int target ) // single ROC: set analog current
{
  Log.section( "OPTIA", false );
  Log.printf( "Ia %i mA\n", target );
  cout << endl;
  cout << "  [OPTIA] target " << target << " mA" << endl;

  int val = dacval[0][Vana];

  if( val < 1 || val > 254 ) {
    val = 111;
    tb.SetDAC( Vana, val );
    dacval[0][Vana] = val;
    tb.mDelay( 200 );
  }

  double ia = tb.GetIA() * 1E3; // [mA]

  const double slope = 6; // 255 DACs / 40 mA
  const double eps = 0.5; // convergence, for DTB [mA]

  double tia = target + 0.5*eps; // besser zuviel als zuwenig

  double diff = ia - tia;

  int iter = 0;
  cout << "  " << iter << ". " << val << "  " << ia << "  " << diff << endl;

  while( fabs( diff ) > eps && iter < 11 && val > 0 && val < 255 ) {

    int stp = int ( fabs( slope * diff ) );
    if( stp == 0 )
      stp = 1;
    if( diff > 0 )
      stp = -stp;

    val += stp;
    if( val < 0 )
      val = 0;
    else if( val > 255 )
      val = 255;

    tb.SetDAC( Vana, val );
    dacval[0][Vana] = val;
    Log.printf( "[SETDAC] %i  %i\n", Vana, val );
    tb.mDelay( 200 );
    ia = tb.GetIA() * 1E3; // contains flush
    diff = ia - tia;
    ++iter;
    cout << "  " << iter << ". " << val << "  " << ia << "  " << diff << endl;
  }
  Log.flush();
  cout << "  set Vana to " << val << endl;

  if( fabs( diff ) < eps && iter < 11 && val > 0 && val < 255 )
    return true;
  else
    return false;
}

//------------------------------------------------------------------------------
CMD_PROC( optia ) // DP  optia ia [mA] for one ROC
{
  if( ierror ) return false;

  int target;
  PAR_INT( target, 5, 50 );

  return setia( target );

} // optia

//------------------------------------------------------------------------------
bool setiamod( int target )
{
  Log.section( "OPTIAMOD", false );
  Log.printf( "Ia %i mA\n", target );
  cout << "adjust Vana to get IA " << target << " mA for each ROC:" << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // set all VA to zero:

  int val[16];

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc ) {
    if( roclist[iroc] )
      ++nrocs;
    val[iroc] = dacval[iroc][Vana];
    tb.roc_I2cAddr(iroc);
    tb.SetDAC( Vana, 0 );
  }
  tb.mDelay(300);

  // one ROC optimized each time, reduce offset current by one

  double corr =  ( double( nrocs ) - 1.0) / double( nrocs );

  double IAoff = tb.GetIA() * 1E3 * corr; // [mA]

  const double slope = 6; // 255 DACs / 40 mA
  const double eps = 0.5; // DTB [mA]

  double iaroc[16];

  bool ok = 1;

  for( int iroc = 0; iroc < 16; ++iroc ) {

    if( !roclist[iroc] ) continue;

    tb.roc_I2cAddr( iroc );
    tb.SetDAC( Vana, val[iroc] );
    tb.mDelay( 300 );
    double ia = tb.GetIA() * 1E3 - IAoff; // [mA]
    double diff = target + 0.5*eps - ia; // besser zuviel als zuwenig

    int iter = 0;
    //initial values
    cout << "ROC "<< iroc << ":" << endl;
    cout << "  " << iter << ". " << val[iroc] << "  " << ia << "  " << diff << endl;
    //save initial settings in case values are alrady good
    dacval[iroc][Vana] = val[iroc];
    iaroc[iroc] = ia;

    while( fabs( diff ) > eps && iter < 10 &&
	   val[iroc] > 0 && val[iroc] < 255 ) {

      int stp = int ( fabs( slope * diff ) );
      if( stp == 0 )
	stp = 1;
      if( diff < 0 )
	stp = -stp;
      val[iroc] += stp;
      if( val[iroc] < 0 )
	val[iroc] = 0;
      else if( val[iroc] > 255 )
	val[iroc] = 255;
      tb.SetDAC( Vana, val[iroc] );
      tb.mDelay( 200 );
      ia = tb.GetIA() * 1E3 -  IAoff; // contains flush
      dacval[iroc][Vana] = val[iroc];
      Log.printf( "[SETDAC] %i  %i\n", Vana, val[iroc] );
      diff = target + 0.1 - ia;
      iaroc[iroc] = ia;
      ++iter;
      cout << "  " << iter << ". " << val[iroc] << "  " << ia << "  " << diff << endl;

    } // while

    ok = fabs( diff ) < 5; // [mA]

    Log.flush();
    tb.SetDAC( Vana, 0 ); // set Vana back to 0 for next ROC
    tb.mDelay( 200 ); // wait for IA to settle

  } // rocs

  double sumia = 0;
  for( int iroc = 0; iroc < 16; ++iroc ) {
    if( !roclist[iroc] ) continue;
    tb.roc_I2cAddr(iroc);
    tb.SetDAC( Vana, dacval[iroc][Vana] );
    tb.mDelay( 200 );
    cout << "ROC " << iroc << " set Vana to " << val[iroc]
	 << " ia " << iaroc[iroc]  << endl;
    sumia = sumia + iaroc[iroc];
  }
  cout<<"sum of all rocs " << sumia
      << " with average " << sumia / nrocs << " per roc"
      << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return ok;

} // setiamod

//------------------------------------------------------------------------------
CMD_PROC( optiamod ) // CS  optiamod ia [mA] for a module
{
  if( ierror ) return false;

  int target;
  PAR_INT( target, 10, 50 );

  return setiamod( target );

} // optiamod

//------------------------------------------------------------------------------
CMD_PROC( show )
{
  int rocmin, rocmax;
  if( !PAR_IS_INT( rocmin, 0, 15 ) ) { // no user input
    rocmin = 0;
    rocmax = 15;
  }
  else
    rocmax = rocmin; // one ROC

  Log.section( "DACs", true );
  for( int32_t roc = rocmin; roc <= rocmax; ++roc )
    if( roclist[roc] ) {
      cout << "ROC " << setw( 2 ) << roc << " Chip " << Chip << endl;
      Log.printf( "ROC %2i Chip %i\n", roc, Chip );

      for( int j = 1; j < 256; ++j ) {
        if( dacval[roc][j] < 0 ) continue; // DAC not active
	cout << setw( 3 ) << j
	     << "  " << dacName[j] << "\t"
	     << setw( 5 ) << dacval[roc][j] << endl;
	Log.printf( "%3i  %s %4i\n", j, dacName[j].c_str(),
		    dacval[roc][j] );
      } // dac
    } // roc
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( show1 ) // print one DAC for all ROCs
{
  int dac;
  PAR_INT( dac, 1, 255 ) ;

  for( int32_t roc = 0; roc <= 15; ++roc )
    if( roclist[roc] ) {
      cout << "ROC " << setw( 2 ) << roc;
      for( int j = 1; j < 256; ++j ) {
        if( dacval[roc][j] < 0 ) continue; // DAC not active
	if( j == dac )
	  cout << "  " << dacName[j]
	       << setw( 5 ) << dacval[roc][j] << endl;
      } // dac
    } //roc
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( wdac ) // write DACs to file
{
  char cdesc[80];
  PAR_STRING( cdesc, 80 );
  string desc = cdesc;

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;

  ostringstream fname; // output string stream

  if( nrocs == 1 )
    fname << "dacParameters_c" << Chip << "_" << desc.c_str() << ".dat";
  else
    fname << "dacParameters_D" << Module << "_" << desc.c_str() << ".dat";

  ofstream dacFile( fname.str().c_str() ); // love it!

  for( size_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      for( int idac = 1; idac < 256; ++idac )
        if( dacval[roc][idac] > -1 ) {
          dacFile << setw( 3 ) << idac
		  << "  " << dacName[idac] << "\t"
		  << setw( 5 ) << dacval[roc][idac] << endl;
        } // dac
    } // ROC
  cout << "DAC values written to " << fname.str() << endl;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( rddac ) // read DACs from file
{
  char cdesc[80];
  PAR_STRING( cdesc, 80 );
  string desc = cdesc;

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;

  ostringstream fname; // output string stream

  if( nrocs == 1 )
    fname << "dacParameters_c" << Chip << "_" << desc.c_str() << ".dat";
  else
    fname << "dacParameters_D" << Module << "_" << desc.c_str() << ".dat";

  ifstream dacFile( fname.str().c_str() );

  if( !dacFile ) {
    cout << "dac file " << fname.str() << " does not exist" << endl;
    return 0;
  }
  cout << "read dac values from " << fname.str() << endl;
  Log.printf( "[RDDAC] %s\n", fname.str().c_str() );

  int idac;
  string dacname;
  int vdac;
  int roc = -1;
  while( !dacFile.eof() ) {
    dacFile >> idac >> dacname >> vdac;
    if( idac == 1 ) {
      ++roc;
      cout << "ROC " << roc << endl;
      tb.roc_I2cAddr( roc );
    }
    if( idac < 0 )
      cout << "illegal dac number " << idac;
    else if( idac > 255 )
      cout << "illegal dac number " << idac;
    else {
      tb.SetDAC( idac, vdac );
      dacval[roc][idac] = vdac;
      cout << setw( 3 ) << idac
	   << "  " << dacName[idac] << "\t" << setw( 5 ) << vdac << endl;
      Log.printf( "[SETDAC] %i  %i\n", idac, vdac );
    }
  }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( wtrim ) // write trim bits to file
{
  char cdesc[80];
  PAR_STRING( cdesc, 80 );
  string desc = cdesc;

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;

  ostringstream fname; // output string stream

  if( nrocs == 1 )
    fname << "trimParameters_c" << Chip << "_" << desc.c_str() << ".dat";
  else
    fname << "trimParameters_D" << Module << "_" << desc.c_str() << ".dat";

  ofstream trimFile( fname.str().c_str() );

  for( size_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      for( int col = 0; col < 52; ++col )
        for( int row = 0; row < 80; ++row ) {
          trimFile << setw( 2 ) << modtrm[roc][col][row]
		   << "  Pix" << setw( 3 ) << col << setw( 3 ) << row << endl;
        }
    }
  cout << "trim values written to " << fname.str() << endl;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( rdtrim ) // read trim bits from file
{
  char cdesc[80];
  PAR_STRING( cdesc, 80 );
  string desc = cdesc;

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;

  ostringstream fname; // output string stream

  if( nrocs == 1 )
    fname << "trimParameters_c" << Chip << "_" << desc.c_str() << ".dat";
  else
    fname << "trimParameters_D" << Module << "_" << desc.c_str() << ".dat";

  ifstream trimFile( fname.str().c_str() );

  if( !trimFile ) {
    cout << "trim file " << fname.str() << " does not exist" << endl;
    return 0;
  }
  cout << "read trim values from " << fname.str() << endl;
  Log.printf( "[RDTRIM] %s\n", fname.str().c_str() );

  string Pix; // dummy
  int icol;
  int irow;
  int itrm;
  vector < uint8_t > trimvalues( 4160 ); // 0..15
  for( size_t roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      for( int col = 0; col < 52; ++col )
        for( int row = 0; row < 80; ++row ) {
          trimFile >> itrm >> Pix >> icol >> irow;
          if( icol != col )
            cout << "pixel number out of order at "
		 << col << " " << row << endl;
          else if( irow != row )
            cout << "pixel number out of order at "
		 << col << " " << row << endl;
          else {
            modtrm[roc][col][row] = itrm;
            int i = 80 * col + row;
            trimvalues[i] = itrm;
          }
        } // pix
      tb.roc_I2cAddr( roc );
      tb.SetTrimValues( roc, trimvalues ); // load into FPGA
      DO_FLUSH;
    } // roc

  return true;

} // rdtrim

//------------------------------------------------------------------------------
CMD_PROC( setbits ) // set trim bits, for modules
{
  int bit;
  PAR_INT( bit, 0, 15 );

  // active ROCs:

  vector < uint8_t > rocAddress;
  rocAddress.reserve( 16 );

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    rocAddress.push_back( roc );
    tb.roc_I2cAddr( roc );

    vector < uint8_t > trimvalues( 4160 );

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        modtrm[roc][col][row] = bit;
        int i = 80 * col + row;
        trimvalues[i] = bit;
      } // row

    tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  } // rocs

  tb.uDelay( 1000 );
  tb.Flush();

  cout << " = " << rocAddress.size() << " ROCs" << endl;

  return true;

} // setbits

//------------------------------------------------------------------------------
int32_t dacStrt( int32_t num )  // min DAC range
{
  if(      num == VDx )
    return 2100; // for 311i
    //return 1800; // ROC 300 looses settings at lower voltage
  else if( num == VAx )
    return  800; // [mV]
  else
    return 0;
}

//------------------------------------------------------------------------------
int32_t dacStop( int32_t num )  // max DAC value
{
  if(      num < 1 )
    return 0;
  else if( num > 255 )
    return 0;
  else if( num == Vdig || num == Vcomp || num == Vbias_sf )
    return 15; // 4-bit
  else if( num == CtrlReg )
    return 4;
  else if( num == VDx )
    return 3000;
  else if( num == VAx )
    return 2000;
  else
    return 255; // 8-bit
}

//------------------------------------------------------------------------------
int32_t dacStep( int32_t num )
{
  if(      num == VDx )
    return 10;
  else if( num == VAx )
    return 10;
  else
    return 1; // 8-bit
}

//------------------------------------------------------------------------------
CMD_PROC( cole )
{
  int colmin, colmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      for( int col = colmin; col <= colmax; col += 2 ) // DC is enough
        tb.roc_Col_Enable( col, 1 );
    }
  DO_FLUSH;
  return true;
}
/* PSI
//------------------------------------------------------------------------------
CMD_PROC( cold )
{
  int col, colmin, colmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      for( col = colmin; col <= colmax; col += 2 )
        tb.roc_Col_Enable( col, 0 );
    }
  DO_FLUSH;
  return true;
}
*/
//------------------------------------------------------------------------------
CMD_PROC( cold ) // DP 21.5.2015
{
  int32_t roc = 0;
  int32_t col;
  PAR_INT( roc, 0, 15 );
  PAR_INT( col, 0, 51 );

  if( roclist[roc] ) {
    tb.roc_I2cAddr( roc );
    tb.roc_Col_Enable( col, 0 );
  }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pixe )
{
  int32_t col, colmin, colmax;
  int32_t row, rowmin, rowmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  PAR_RANGE( rowmin, rowmax, 0, ROC_NUMROWS - 1 );

  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      for( col = colmin; col <= colmax; ++col )
        for( row = rowmin; row <= rowmax; ++row ) {
	  int trim = modtrm[roc][col][row];
	  if( trim > 15 ) {
	    tb.roc_Pix_Mask( col, row );
	    cout << "mask roc col row "
		 << setw(2) << roc
		 << setw(3) << col
		 << setw(3) << row
		 << endl;
	  }
	  else
	    tb.roc_Pix_Trim( col, row, trim );
        } // row
    } //roc
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pixi ) // info
{
  int32_t roc = 0;
  int32_t col;
  int32_t row;
  PAR_INT( roc, 0, 15 );
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  if( roclist[roc] ) {
    cout << "trim bits " << setw( 2 ) << modtrm[roc][col][row]
	 << ", thr " << modthr[roc][col][row] << endl;
  }
  else
    cout << "ROC not active" << endl;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( pixt ) // set trim bits
{
  int32_t roc;
  int32_t col;
  int32_t row;
  int32_t trm;
  PAR_INT( roc, 0, 15 ); 
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( trm, 0, 16 ); // 0..15 valid, 16 = mask

  if( roclist[roc] ) {

    modtrm[roc][col][row] = trm;

    tb.roc_I2cAddr( roc );
    vector < uint8_t > trimvalues( 4160 );

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        int i = 80 * col + row;
        trimvalues[i] = modtrm[roc][col][row];
      } // row
    tb.SetTrimValues( roc, trimvalues ); // load into FPGA
    DO_FLUSH;
  }
  else
    cout << "ROC not active" << endl;

  return true;

} // pixt

//------------------------------------------------------------------------------
CMD_PROC( arm ) // DP arm 2 2 or arm 0:51 2  activate pixel for cal
{
  int32_t colmin, colmax;
  int32_t rowmin, rowmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  PAR_RANGE( rowmin, rowmax, 0, ROC_NUMROWS - 1 );

  int roc;
  if( !PAR_IS_INT( roc, 0, 15 ) )
    roc = 16;
  int rocmin =  0;
  int rocmax = 15;
  if( roc < 16 ) { // only one ROC
    rocmin = roc;
    rocmax = roc;
  }
  for( int32_t roc = rocmin; roc <= rocmax; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      for( int col = colmin; col <= colmax; ++col ) {
        tb.roc_Col_Enable( col, 1 );
        for( int row = rowmin; row <= rowmax; ++row ) {
          int trim = modtrm[roc][col][row];
          tb.roc_Pix_Trim( col, row, trim );
          tb.roc_Pix_Cal( col, row, false );
        }
      }
    }
  DO_FLUSH;
  return true;

} // arm

//------------------------------------------------------------------------------
CMD_PROC( pixd )
{
  int32_t roc = 0;
  int32_t colmin, colmax;
  int32_t rowmin, rowmax;
  PAR_INT( roc, 0, 15 );
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  PAR_RANGE( rowmin, rowmax, 0, ROC_NUMROWS - 1 );
  if( roclist[roc] ) {
    tb.roc_I2cAddr( roc );
    for( int col = colmin; col <= colmax; ++col )
      for( int row = rowmin; row <= rowmax; ++row ) {
	tb.roc_Pix_Mask( col, row );
	modtrm[roc][col][row] = 16; // 16 = mask flag
	cout << "mask roc col row " << roc << " " << col << " " << row << endl;
      }
  }
  DO_FLUSH;
  return true;

} // pixd

//------------------------------------------------------------------------------
CMD_PROC( pixm ) // mask one pix
{
  int32_t roc = 0;
  int32_t col;
  int32_t row;
  PAR_INT( roc, 0, 15 );
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  if( roclist[roc] ) {
    tb.roc_I2cAddr( roc );
    tb.roc_Pix_Mask( col, row );
    modtrm[roc][col][row] = 16; // 16 = mask flag
    cout << "mask roc col row " << roc << " " << col << " " << row << endl;
  }
  DO_FLUSH;
  return true;

}

//------------------------------------------------------------------------------
CMD_PROC( cal )
{
  int32_t col, colmin, colmax;
  int32_t row, rowmin, rowmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  PAR_RANGE( rowmin, rowmax, 0, ROC_NUMROWS - 1 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      for( col = colmin; col <= colmax; ++col )
        for( row = rowmin; row <= rowmax; ++row )
          tb.roc_Pix_Cal( col, row, false );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( cals )
{
  int32_t col, colmin, colmax;
  int32_t row, rowmin, rowmax;
  PAR_RANGE( colmin, colmax, 0, ROC_NUMCOLS - 1 );
  PAR_RANGE( rowmin, rowmax, 0, ROC_NUMROWS - 1 );
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      for( col = colmin; col <= colmax; ++col )
        for( row = rowmin; row <= rowmax; ++row )
          tb.roc_Pix_Cal( col, row, true );
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( cald )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.roc_ClrCal();
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( mask )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.roc_Chip_Mask();
    }
  DO_FLUSH;
  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( daci ) // currents vs dac
// noise activity peak: VthrComp
// cole 0:51
// pixe 0:51 0:79
// daci 12
{
  int32_t roc = 0;
  //PAR_INT( roc, 0, 15 );

  int32_t dac;
  PAR_INT( dac, 0, 32 );

  if( dacval[roc][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  tb.roc_I2cAddr( roc );

  int val = dacval[roc][dac];

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int32_t dacstep = dacStep( dac );
  int32_t nstp = ( dacstop - dacstrt ) / dacstep + 1;

  Log.section( "DACCURRENT", false );
  Log.printf( "DAC %i  %i:%i\n", dac, 0, dacstop );

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "ID_DAC%02i", dac ),
          Form( "Digital current vs %s;%s [DAC];ID [mA]",
                dacName[dac].c_str(), dacName[dac].c_str() ),
	  nstp, dacstrt - 0.5, dacstop + 0.5 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "IA_DAC%02i", dac ),
          Form( "Analog current vs %s;%s [DAC];IA [mA]",
                dacName[dac].c_str(), dacName[dac].c_str() ),
	  nstp, dacstrt - 0.5, dacstop + 0.5 );
  h12->Sumw2();

  for( int32_t i = dacstrt; i <= dacstop; i += dacstep ) {

    tb.SetDAC( dac, i );
    tb.mDelay( 50 );

    double id = tb.GetID() * 1E3; // [mA]
    double ia = tb.GetIA() * 1E3;

    Log.printf( "%i %5.1f %5.1f\n", i, id, ia );
    printf( "%3i %5.1f %5.1f\n", i, id, ia );

    h11->Fill( i, id );
    h12->Fill( i, ia );
  }
  Log.flush();

  tb.SetDAC( dac, val ); // restore
  tb.Flush();

  h11->Write();
  h12->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 12" << endl;

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( vthrcompi ) // Id vs VthrComp: noise peak (amplitude depends on Temp?)
// peak position depends on Vana
{
  if( ierror ) return false;

  Log.section( "Id-vs-VthrComp", false );

  int roc = 0;
  PAR_INT( roc, 0, 15 );

  int32_t dacstop = dacStop( VthrComp );
  int32_t nstp = dacstop + 1;
  vector < double > yvec( nstp, 0.0 );
  double ymax = 0;
  int32_t imax = 0;

  Log.printf( "DAC %i: %i:%i on ROC %i\n", VthrComp, 0, dacstop, roc );

  //select the ROC:
  tb.roc_I2cAddr( roc );

  // all pix on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, true );
    for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[roc][col][row];
      tb.roc_Pix_Trim( col, row, trim );
    }
  }
  tb.Flush();

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "ID_VthrComp",
          "Digital current vs comparator threshold;VthrComp [DAC];ID [mA]",
          256, -0.5, 255.5 );
  h11->Sumw2();

  for( int32_t i = 0; i <= dacstop; ++i ) {

    tb.SetDAC( VthrComp, i );
    tb.mDelay( 20 );

    double id = tb.GetID() * 1000.0; // [mA]

    h11->Fill( i, id );

    Log.printf( "%3i %5.1f\n", i, id );
    if( roc == 0 ) printf( "%i %5.1f mA\n", i, id );

    yvec[i] = id;
    if( id > ymax ) {
      ymax = id;
      imax = i;
    }
  }

  // all off:

  tb.roc_Chip_Mask();
  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 0 );
    //for( int row = 0; row < 80; ++row ) tb.roc_Pix_Mask( col, row );
  }
  tb.roc_ClrCal();
  tb.Flush();

  h11->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  cout << "peak " << ymax << " mA" << " at " << imax << endl;

  double maxd1 = 0;
  double maxd2 = 0;
  double maxd4 = 0;
  double maxd7 = 0;
  double maxd11 = 0;
  double maxd16 = 0;
  double maxd22 = 0;
  double maxd29 = 0;
  int maxi1 = 0;
  int maxi2 = 0;
  int maxi4 = 0;
  int maxi7 = 0;
  int maxi11 = 0;
  int maxi16 = 0;
  int maxi22 = 0;
  int maxi29 = 0;
  for( int i = 0; i <= dacstop - 29; ++i ) {
    double ni = yvec[i];
    double d1 = yvec[i + 1] - ni;
    double d2 = yvec[i + 2] - ni;
    double d4 = yvec[i + 4] - ni;
    double d7 = yvec[i + 7] - ni;
    double d11 = yvec[i + 11] - ni;
    double d16 = yvec[i + 16] - ni;
    double d22 = yvec[i + 22] - ni;
    double d29 = yvec[i + 29] - ni;
    if( d1 > maxd1 ) {
      maxd1 = d1;
      maxi1 = i;
    }
    if( d2 > maxd2 ) {
      maxd2 = d2;
      maxi2 = i;
    }
    if( d4 > maxd4 ) {
      maxd4 = d4;
      maxi4 = i;
    }
    if( d7 > maxd7 ) {
      maxd7 = d7;
      maxi7 = i;
    }
    if( d11 > maxd11 ) {
      maxd11 = d11;
      maxi11 = i;
    }
    if( d16 > maxd16 ) {
      maxd16 = d16;
      maxi16 = i;
    }
    if( d22 > maxd22 ) {
      maxd22 = d22;
      maxi22 = i;
    }
    if( d29 > maxd29 ) {
      maxd29 = d29;
      maxi29 = i;
    }
  }
  cout << "max d1  " << maxd1 << " at " << maxi1 << endl;
  cout << "max d2  " << maxd2 << " at " << maxi2 << endl;
  cout << "max d4  " << maxd4 << " at " << maxi4 << endl;
  cout << "max d7  " << maxd7 << " at " << maxi7 << endl;
  cout << "max d11 " << maxd11 << " at " << maxi11 << endl;
  cout << "max d16 " << maxd16 << " at " << maxi16 << endl;
  cout << "max d22 " << maxd22 << " at " << maxi22 << endl;
  cout << "max d29 " << maxd29 << " at " << maxi29 << endl;

  int32_t val = maxi22 - 8; // safety for digV2.1
  val = maxi29 - 30; // safety for digV2
  if( val < 0 )
    val = 0;
  tb.SetDAC( VthrComp, val );
  tb.Flush();
  dacval[roc][VthrComp] = val;
  Log.printf( "[SETDAC] %i  %i\n", VthrComp, val );
  Log.flush();

  cout << "set VthrComp to " << val << endl;

  return true;

} // vthrcompi

//------------------------------------------------------------------------------
// utility function for Pixel PH and cnt, ROC or mod
bool GetPixData( int roc, int col, int row, int nTrig,
                 int &nReadouts, double &PHavg, double &PHrms )
{
  bool ldb = 0;

  nReadouts = 0;
  PHavg = -1;
  PHrms = -1;

  tb.roc_I2cAddr( roc );
  tb.roc_Col_Enable( col, true );
  int trim = modtrm[roc][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.roc_Pix_Cal( col, row, false );

  vector < uint8_t > trimvalues( 4160 );
  for( int x = 0; x < 52; ++x )
    for( int y = 0; y < 80; ++y ) {
      int trim = modtrm[roc][x][y];
      int i = 80 * x + y;
      trimvalues[i] = trim;
    } // row y

  tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  int ch = roc / (16/nChannels); // 0, 1, 2, 3
  if( L1 )
    ch = roc2ch[roc];

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  if( nrocs == 1 )
    tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  else {
    tb.Daq_Select_Deser400();
    tb.Daq_Deser400_Reset( 3 );
  }
  tb.uDelay( 100 );
  tb.Daq_Start( ch );
  tb.uDelay( 100 );

  uint16_t flags = 0;
  if( nTrig < 0 )
    flags = 0x0002; // FLAG_USE_CALS

  flags |= 0x0010; // FLAG_FORCE_MASK else noisy, is FPGA default

  uint16_t mTrig = abs( nTrig );

  tb.LoopSingleRocOnePixelCalibrate( roc, col, row, mTrig, flags );

  tb.Daq_Stop( ch );

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize( ch ) );
  uint32_t rest;
  tb.Daq_Read( data, Blocksize, rest, ch );

#ifdef DAQOPENCLOSE
  tb.Daq_Close( ch );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  // unpack data:

  int cnt = 0;
  int phsum = 0;
  int phsu2 = 0;

  if( nrocs == 1 ) {

    Decoder dec;
    bool even = 1;

    bool extra = 0;

    for( unsigned int i = 0; i < data.size(); ++i ) {

      if( ( data[i] & 0xffc ) == 0x7f8 ) { // header
	even = 0;
      }
      else if( even ) { // merge 2 data words into one int:

	if( i < data.size() - 1 ) {

	  unsigned long raw = ( data[i] & 0xfff ) << 12;
	  raw += data[i + 1] & 0xfff;

	  dec.Translate( raw );

	  uint16_t ix = dec.GetX();
	  uint16_t iy = dec.GetY();
	  uint16_t ph = dec.GetPH();

	  if( ix == col && iy == row ) {
	    ++cnt;
	    phsum += ph;
	    phsu2 += ph * ph;
	  }
	  else {
	    cout << " + " << ix << " " << iy << " " << ph;
	    extra = 1;
	  }
	}
      }

      even = 1 - even;

    } // data loop

    if( extra )
      cout << endl;

  } // ROC

  else { // mod

    bool ldbm = 0;

    int event = 0;

    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t kroc = (16/nChannels) * ch - 1; // will start at 0 or 8

    // nDAC * nTrig * (TBM header, some ROC headers, one pixel, more ROC headers, TBM trailer)

    for( size_t i = 0; i < data.size(); ++i ) {

      int d = data[i] & 0xfff; // 12 bits data
      int v = ( data[i] >> 12 ) & 0xe; // 3 flag bits: e = 14 = 1110

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;

      switch ( v ) {

	// TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
	//DecodeTbmHeader(hdr);
        if( ldbm )
          cout << "event " << setw( 6 ) << event;
        kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
	if( L1 ) kroc = ch2roc[ch] - 1;
        break;

	// ROC header data:
      case 4:
        ++kroc;
        break;

	// pixel data:
      case 0:
        raw = d;
        break;
      case 2:
        raw = ( raw << 12 ) + d;
	//DecodePixel(raw);
        ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
        raw >>= 9;
	if( Chip < 600 ) {
	  c = ( raw >> 12 ) & 7;
	  c = c * 6 + ( ( raw >> 9 ) & 7 );
	  r = ( raw >> 6 ) & 7;
	  r = r * 6 + ( ( raw >> 3 ) & 7 );
	  r = r * 6 + ( raw & 7 );
	  y = 80 - r / 2;
	  x = 2 * c + ( r & 1 );
	} // psi46dig
	else {
	  y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
	  x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
	} // proc600
        if( ldbm )
          cout << setw( 3 ) << kroc << setw( 3 ) << x << setw( 3 ) << y <<
            setw( 4 ) << ph;
	
        if( kroc == roc && x == col && y == row ) {
          ++cnt;
          phsum += ph;
	  phsu2 += ph * ph;
        }
        break;

	// TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
	//DecodeTbmTrailer(trl);
        if( ldbm )
          cout << endl;
        ++event;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    if( ldb )
      cout << "  events " << event
	   << " = " << event / mTrig << " dac values" << endl;

  } // mod

  nReadouts = cnt;
  if( cnt > 0 ) {
    PHavg = ( double ) phsum / cnt;
    PHrms = sqrt( ( double ) phsu2 / cnt - PHavg * PHavg );
  }

  return 1;

} // GetPixData

//------------------------------------------------------------------------------
CMD_PROC( fire ) // fire col row (nTrig) [send cal and read ROC]
{
  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  int nTrig;
  if( !PAR_IS_INT( nTrig, -65500, 65500 ) )
    nTrig = 1;

  tb.roc_Col_Enable( col, true );
  int trim = modtrm[0][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  bool cals = nTrig < 0;
  tb.roc_Pix_Cal( col, row, cals );

  cout << "fire"
       << " col " << col
       << " row " << row << " trim " << trim << " nTrig " << nTrig << endl;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );
  tb.Flush();

  for( int k = 0; k < abs( nTrig ); ++k ) {
    tb.Pg_Single();
    tb.uDelay( 20 );
  }
  tb.Flush();

  tb.Daq_Stop();
  //cout << "  DAQ size " << tb.Daq_GetSize() << endl;

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize() );

  tb.Daq_Read( data, Blocksize );

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  // pixel off:

  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  cout << "  data size " << data.size();
  for( size_t i = 0; i < data.size(); ++i ) {
    if( ( data[i] & 0xffc ) == 0x7f8 )
      cout << ":";
    printf( " %4X", data[i] ); // full word with FPGA markers
    //printf( " %3X", data[i] & 0xfff ); // 12 bits per word
  }
  cout << endl;

  Decoder dec;
  bool even = 1;

  int evt = 0;
  int cnt = 0;
  int phsum = 0;
  int phsu2 = 0;
  double vcsum = 0;

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "ph_distribution",
          Form( "ph col %i row %i;PH [ADC];triggers", col, row ),
          256, -0.5, 255.5 );
  h11->Sumw2();

  for( unsigned int i = 0; i < data.size(); ++i ) {

    if( ( data[i] & 0xffc ) == 0x7f8 ) { // ROC header
      even = 0;
      if( evt > 0 ) cout << endl;
      ++evt;
      cout << "  evt " << setw( 2 ) << evt;
    }
    else if( even ) { // merge 2 words into one pixel int:

      if( i < data.size() - 1 ) {

        unsigned long raw = ( data[i] & 0xfff ) << 12;
        raw += data[i + 1] & 0xfff;

        dec.Translate( raw );

        uint16_t ix = dec.GetX();
        uint16_t iy = dec.GetY();
        uint16_t ph = dec.GetPH();
        double vc = PHtoVcal( ph, 0, ix, iy );

        cout << " pix " << setw( 2 ) << ix << setw( 3 ) << iy << setw( 4 ) << ph;

        if( ix == col && iy == row ) {
          ++cnt;
          phsum += ph;
          phsu2 += ph * ph;
          vcsum += vc;
	  h11->Fill( ph );
        }
        else {
          cout << " + " << ix << " " << iy << " " << ph;
        }
      }
    }

    even = 1 - even;

  } // data

  cout << endl;

  double ph = -1;
  double rms = -1;
  double vc = -1;
  if( cnt > 0 ) {
    ph = ( double ) phsum / cnt;
    rms = sqrt( ( double ) phsu2 / cnt - ph * ph );
    vc = vcsum / cnt;
  }
  cout << "  pixel " << setw( 2 ) << col << setw( 3 ) << row
       << " responses " << cnt
       << ", PH " << ph << ", RMS " << rms << ", Vcal " << vc << endl;

  h11->Write();
  h11->SetStats( 111111 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  return true;

} // fire

//------------------------------------------------------------------------------
CMD_PROC( fire2 ) // fire2 col row (nTrig) [2-row correlation]
// fluctuates wildly
{
  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 1;

  tb.roc_Col_Enable( col, true );
  int trim = modtrm[0][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.roc_Pix_Cal( col, row, false );

  int row2 = row + 1;
  if( row2 == 80 )
    row2 = row - 1;
  trim = modtrm[0][col][row2];
  tb.roc_Pix_Trim( col, row2, trim );
  tb.roc_Pix_Cal( col, row2, false );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );

  for( int k = 0; k < nTrig; ++k ) {
    tb.Pg_Single();
    tb.uDelay( 200 );
  }

  tb.Daq_Stop();
  cout << "DAQ size " << tb.Daq_GetSize() << endl;

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize() );

  tb.Daq_Read( data, Blocksize );

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  cout << "data size " << data.size();
  for( size_t i = 0; i < data.size(); ++i ) {
    if( ( data[i] & 0xffc ) == 0x7f8 )
      cout << " :";
    printf( " %4X", data[i] ); // full word with FPGA markers
    //printf( " %3X", data[i] & 0xfff ); // 12 bits per word
  }
  cout << endl;

  vector <CRocPixel> pixels; // datastream.h
  pixels.reserve( 2 );

  Decoder dec;
  bool even = 1;

  int evt = 0;
  int cnt[8] = { 0 };
  int phsum[8] = { 0 };
  int phsu2[8] = { 0 };
  double vcsum[8] = { 0 };
  int php[8] = { 0 };
  int n2 = 0;
  int ppsum = 0;

  for( unsigned int i = 0; i < data.size(); ++i ) {

    if( ( data[i] & 0xffc ) == 0x7f8 ) { // ROC header
      even = 0;
      ++evt;
      pixels.clear();
      php[0] = -1;
      php[1] = -1;
    }
    else if( even ) { // merge 2 words into one pixel int:

      if( i < data.size() - 1 ) {

        unsigned long raw = ( data[i] & 0xfff ) << 12;
        raw += data[i + 1] & 0xfff;

        CRocPixel pix;
        pix.raw = data[i] << 12;
        pix.raw += data[i + 1] & 0xfff;
        pix.DecodeRaw();
        pixels.push_back( pix );

        dec.Translate( raw );

        uint16_t ix = dec.GetX();
        uint16_t iy = dec.GetY();
        uint16_t ph = dec.GetPH();
        double vc = PHtoVcal( ph, 0, ix, iy );

        if( ix == col && iy == row ) {
          ++cnt[0];
          phsum[0] += ph;
          phsu2[0] += ph * ph;
          vcsum[0] += vc;
          php[0] = ph;
        }
        else if( ix == col && iy == row2 ) {
          ++cnt[1];
          phsum[1] += ph;
          phsu2[1] += ph * ph;
          vcsum[1] += vc;
          php[1] = ph;
        }
        else {
          cout << " + " << ix << " " << iy << " " << ph;
        }

	// check for end of event:

        if( data[i + 1] & 0x4000 && php[0] > 0 && php[1] > 0 ) {
          ppsum += php[0] * php[1];
          ++n2;
        } // event end

      } // data size

    } // even

    even = 1 - even;

  } // data
  cout << endl;

  if( n2 > 0 && cnt[0] > 1 && cnt[1] > 1 ) {
    double ph0 = ( double ) phsum[0] / cnt[0];
    double rms0 = sqrt( ( double ) phsu2[0] / cnt[0] - ph0 * ph0 );
    double ph1 = ( double ) phsum[1] / cnt[1];
    double rms1 = sqrt( ( double ) phsu2[1] / cnt[1] - ph1 * ph1 );
    double cov = ( double ) ppsum / n2 - ph0 * ph1;
    double cor = 0;
    if( rms0 > 1e-9 && rms1 > 1e-9 )
      cor = cov / rms0 / rms1;

    cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row
	 << " responses " << cnt[0]
	 << ", <PH> " << ph0 << ", RMS " << rms0 << endl;
    cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row2
	 << " responses " << cnt[1]
	 << ", <PH> " << ph1 << ", RMS " << rms1 << endl;
    cout << "cov " << cov << ", corr " << cor << endl;
  }

  tb.roc_Pix_Mask( col, row );
  tb.roc_Pix_Mask( col, row2 );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  return true;

} // fire2

//------------------------------------------------------------------------------
// utility function: single pixel dac scan, ROC or mod
bool DacScanPix( const uint8_t roc, const uint8_t col, const uint8_t row,
                 const uint8_t dac, const uint8_t stp, const int16_t nTrig,
                 vector < int16_t > &nReadouts,
                 vector < double > &PHavg, vector < double > &PHrms )
{
  bool ldb = 0;

  tb.roc_I2cAddr( roc );
  tb.roc_Col_Enable( col, true );
  int trim = modtrm[roc][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.roc_Pix_Cal( col, row, false );

  vector < uint8_t > trimvalues( 4160 );
  for( int x = 0; x < 52; ++x )
    for( int y = 0; y < 80; ++y ) {
      int trim = modtrm[roc][x][y];
      int i = 80 * x + y;
      trimvalues[i] = trim;
    } // row y

  tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  int ch = roc / (16/nChannels); // 0, 1, 2, 3
  if( L1 )
    ch = roc2ch[roc];

  int nrocs = 0;
  for( int iroc = 0; iroc < 16; ++iroc )
    if( roclist[iroc] )
      ++nrocs;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize, ch );
#endif
  if( nrocs == 1 )
    tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  else {
    tb.Daq_Select_Deser400();
    tb.Daq_Deser400_Reset( 3 );
  }
  tb.uDelay( 100 );
  tb.Daq_Start( ch );
  tb.uDelay( 100 );
  tb.Flush();

  uint16_t flags = 0;
  if( nTrig < 0 )
    flags = 0x0002; // FLAG_USE_CALS
  //flags |= 0x0020; // don't use DAC correction table
  //flags |= 0x0010; // FLAG_FORCE_MASK (needs trims)

  // scan dac:

  int vdac = dacval[roc][dac]; // remember

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );

  uint16_t mTrig = abs( nTrig );

  if( ldb )
    cout << "  scan Dac " << ( int ) dac
	 << " for Pix " << ( int ) roc << " " << ( int ) col << " " << ( int ) row
	 << " mTrig " << mTrig << ", flags hex " << hex << flags << dec << endl;

  bool done = 0;
  try {
    if( dac < 31 ) 
      done =
	tb.LoopSingleRocOnePixelDacScan( roc, col, row, mTrig, flags,
					 dac, stp, dacstrt, dacstop );
    else {
      for( int32_t i = dacstrt; i <= dacstop; i += stp ) {
	tb.SetDAC( dac, i );
	tb.mDelay(100);
	cout << " " << i << flush;
	done =
	  tb.LoopSingleRocOnePixelCalibrate( roc, col, row, mTrig, flags );
      }
      cout << endl;
    }
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  // header = 1 word
  // pixel = +2 words
  // size = 256 * mTrig * (1 or 3) = 7680 for mTrig 10, all respond

  tb.Daq_Stop( ch ); // else ROC 0, 8 garbage (13.2.2015)

  if( ldb ) {
    cout << "  DAQ size " << tb.Daq_GetSize( ch ) << endl;
    cout << "  DAQ fill " << ( int ) tb.Daq_FillLevel( ch ) << endl;
    cout << ( done ? "done" : "not done" ) << endl;
  }

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize( ch ) );

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest, ch ); // 32768 gives zero
    if( ldb )
      cout << "  data size " << data.size()
	   << ", remaining " << rest << endl;
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest, ch );
      data.insert( data.end(), dataB.begin(), dataB.end() );
      if( ldb )
	cout << "  data size " << data.size()
	     << ", remaining " << rest << endl;
      dataB.clear();
    }
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

#ifdef DAQOPENCLOSE
  tb.Daq_Close( ch );
  //tb.Daq_DeselectAll();
#endif

  tb.SetDAC( dac, vdac ); // restore dac value
  tb.roc_Pix_Mask( col, row );
  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  // unpack data:

  if( nrocs == 1 ) {

    if( ldb ) {
      if( tb.linearAddress )
	cout << "PROC600 with linear address encoding" << endl;
      else
	cout << "PSI46dig with Gray code" << endl;
    }

    int pos = 0;

    try {

      int32_t nstp = ( dacstop - dacstrt ) / stp + 1;

      if( ldb ) cout << "  expect " << nstp << " dac steps" << endl;

      int nmin = mTrig;
      int nmax = 0;

      for( int32_t j = 0; j < nstp; ++j ) { // DAC steps

        int cnt = 0;
        int phsum = 0;
        int phsu2 = 0;

        for( int k = 0; k < mTrig; ++k ) {

          int hdr;
          vector <PixelReadoutData> vpix = GetEvent( data, pos, hdr ); // analyzer

          for( size_t ipx = 0; ipx < vpix.size(); ++ipx )

            if( vpix[ipx].x == col && vpix[ipx].y == row ) {
              ++cnt;
              int ph = vpix[ipx].p;
              phsum += ph;
              phsu2 += ph * ph;
            }
	    else
	      cout << "wrong address " <<  vpix[ipx].x
		   << " " <<  vpix[ipx].y
		   << endl;
        } // trig

        nReadouts.push_back( cnt );
        if( cnt > nmax )
          nmax = cnt;
        if( cnt < nmin )
          nmin = cnt;

        double ph = -1.0;
        double rms = -0.1;
        if( cnt > 0 ) {
          ph = ( double ) phsum / cnt;
          rms = sqrt( ( double ) phsu2 / cnt - ph * ph );
        }
        PHavg.push_back( ph );
        PHrms.push_back( rms );

      } // dacs

      if( ldb ) cout << "  final data pos " << pos << endl;
      if( ldb ) cout << "  nReadouts.size " << nReadouts.size() << endl;
      if( ldb ) cout << "  min max responses " << nmin << " " << nmax << endl;

      int thr = nmin + ( nmax - nmin ) / 2; // 50% level
      int thrup = dacstrt;
      int thrdn = dacstop;

      for( size_t i = 0; i < nReadouts.size() - 1; ++i ) {
        bool below = 1;
        if( nReadouts[i] > thr )
          below = 0;
        if( below ) {
          if( nReadouts[i + 1] > thr )
            thrup = dacstrt + i + 1; // first above
        }
        else if( nReadouts[i + 1] < thr )
          thrdn = dacstrt + i + 1; // first below
      }

      if( ldb )
	cout << "  responses min " << nmin << " max " << nmax
	     << ", step above 50% at " << thrup
	     << ", step below 50% at " << thrdn
	     << endl;

    } // try

    catch( int e ) {
      cout << "  Data error " << e << " at pos " << pos << endl;
      for( int i = pos - 8;
           i >= 0 && i < pos + 8 && i < int ( data.size() ); ++i )
        cout << setw( 12 ) << i << hex << setw( 5 ) << data.at( i )
	     << dec << endl;
      return false;
    }

  } // single ROC

  else { // Module:

    bool ldbm = 0;

    int event = 0;

    int cnt = 0;
    int phsum = 0;
    int phsu2 = 0;

    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t kroc = (16/nChannels) * ch - 1; // will start at 0 or 8

    // nDAC * nTrig * (TBM header, some ROC headers, one pixel, more ROC headers, TBM trailer)

    for( size_t i = 0; i < data.size(); ++i ) {

      int d = data[i] & 0xfff; // 12 bits data
      int v = ( data[i] >> 12 ) & 0xe; // 3 flag bits: e = 14 = 1110

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;

      switch ( v ) {

	// TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
	//DecodeTbmHeader(hdr);
        if( ldbm )
          cout << "event " << setw( 6 ) << event;
        kroc = (16/nChannels) * ch - 1; // new event, will start at 0 or 8
	if( L1 ) kroc = ch2roc[ch] - 1;
        break;

	// ROC header data:
      case 4:
        ++kroc;
        break;

	// pixel data:
      case 0:
        raw = d;
        break;
      case 2:
        raw = ( raw << 12 ) + d;
	//DecodePixel(raw);
        ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
        raw >>= 9;
	if( Chip < 600 ) {
	  c = ( raw >> 12 ) & 7;
	  c = c * 6 + ( ( raw >> 9 ) & 7 );
	  r = ( raw >> 6 ) & 7;
	  r = r * 6 + ( ( raw >> 3 ) & 7 );
	  r = r * 6 + ( raw & 7 );
	  y = 80 - r / 2;
	  x = 2 * c + ( r & 1 );
	} // psi46dig
	else {
	  y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
	  x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
	} // proc600
        if( ldbm )
          cout << setw( 3 ) << kroc << setw( 3 ) << x << setw( 3 ) << y <<
            setw( 4 ) << ph;
        if( kroc == roc && x == col && y == row ) {
          ++cnt;
          phsum += ph;
	  phsu2 += ph * ph;
        }
	else
	  cout << " extra roc col row ph " << ( int ) roc
	       << setw(3) << x
	       << setw(3) << y
	       << setw(4) << ph
	       << endl;
        break;

	// TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
	//DecodeTbmTrailer(trl);
        if( event % mTrig == mTrig - 1 ) {
          nReadouts.push_back( cnt );
	  double ph = -1.0;
	  double rms = 0.0;
	  if( cnt > 0 ) {
	    ph = ( double ) phsum / cnt;
	    rms = sqrt( ( double ) phsu2 / cnt - ph * ph );
	  }
          PHavg.push_back( ph );
	  PHrms.push_back( rms );
          cnt = 0;
          phsum = 0;
          phsu2 = 0;
        }
        if( ldbm )
          cout << endl;
        ++event;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    if( ldb )
      cout << "  events " << event
	   << " = " << event / mTrig << " dac values" << endl;

  } // module

  return true;

} // DacScanPix

//------------------------------------------------------------------------------
// utility function: single pixel threshold on module or ROC
int ThrPix( const uint8_t roc, const uint8_t col, const uint8_t row,
            const uint8_t dac, const uint8_t stp, const int16_t nTrig )
{
  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, stp, nTrig, nReadouts, PHavg, PHrms ); // ROC or mod

  int nstp = nReadouts.size();

  // analyze:

  int nmx = 0;

  for( int j = 0; j < nstp; ++j ) {
    int cnt = nReadouts.at( j );
    if( cnt > nmx )
      nmx = cnt;
  }

  int thr = 0;
  for( int j = 0; j < nstp; ++j )
    if( nReadouts.at( j ) <= 0.5 * nmx )
      thr = j; // overwritten until passed
    else
      break; // beyond threshold 18.11.2014

  return thr;

} // ThrPix

//------------------------------------------------------------------------------
bool setcaldel( int roc, int col, int row, int nTrig )
{
  Log.section( "CALDEL", false );
  Log.printf( "for ROC %i col %i row %i trig \n", roc, col, row, nTrig );
  cout << endl;
  cout << "[CalDel] for roc " << roc << " col " << col << " row " << row << endl;
  cout << "  VthrComp " << dacval[roc][VthrComp] << endl;
  cout << "  CtrlReg  " << dacval[roc][CtrlReg] << endl;
  cout << "  Vcal     " << dacval[roc][Vcal] << endl;

  uint8_t dac = CalDel;
  int val = dacval[roc][CalDel];
  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, 1, nTrig, nReadouts, PHavg, PHrms );

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int nstp = dacstop - dacstrt + 1;
  cout << "  DAC steps " << nstp << endl;

  int wbc = dacval[roc][WBC];

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "caldel_%02i_%02i_%02i_wbc%03i", roc, col, row, wbc ),
          Form( "CalDel scan ROC % i col %i row %i WBC %i;CalDel [DAC];responses",
                roc, col, row, wbc ),
	  nstp, dacstrt-0.5, dacstop + 0.5 );
  h11->Sumw2();

  // analyze:

  int nm = 0;
  int i0 = 0;
  int i9 = 0;
  nstp = nReadouts.size();

  for( int j = 0; j < nstp; ++j ) {

    int cnt = nReadouts.at( j );

    cout << " " << cnt;
    h11->Fill( j, cnt );

    if( cnt > nm ) {
      nm = cnt;
      i0 = j; // begin of plateau
    }
    if( cnt >= nm )
      i9 = j; // end of plateau

  } // caldel
  cout << endl;
  cout << "  response plateau " << nm << " from " << i0 << " to " << i9 << endl;
  Log.printf( "  response plateau %i from %i to %i\n", nm, i0, i9 );

  h11->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  bool ok = 1;
  if( nm > nTrig / 2 ) {

    int i2 = i0 + ( i9 - i0 ) / 4; // 25% from left edge

    tb.SetDAC( CalDel, i2 );
    dacval[roc][CalDel] = i2;
    printf( "  set CalDel to %i\n", i2 );
    Log.printf( "  [SETDAC] %i  %i\n", CalDel, i2 );
  }
  else {
    tb.SetDAC( CalDel, val ); // back to default
    cout << "  leave CalDel at " << val << endl;
    ok = 0;
  }
  tb.Flush();

  Log.flush();

  return ok;

} // setcaldel

//------------------------------------------------------------------------------
CMD_PROC( caldel ) // scan and set CalDel using one pixel
{
  if( ierror ) return false;

  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 100;

  return setcaldel( 0, col, row, nTrig );

}

//------------------------------------------------------------------------------
CMD_PROC( caldelmap ) // map caldel left edge, 22 s (nTrig 10), 57 s (nTrig 99)
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  Log.section( "CALDELMAP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int vdac = dacval[0][CalDel];
  const int vctl = dacval[0][CtrlReg];
  const int vcal = dacval[0][Vcal];
  tb.SetDAC( CtrlReg, 4 );
  tb.SetDAC( Vcal, 255 ); // max pulse

  int nTrig = 16;
  int step = 1;

  if( h21 )
    delete h21;
  h21 = new TH2D( "CalDelMap",
                  "CalDel map;col;row;CalDel range left edge [DAC]",
                  52, -0.5, 51.5, 80, -0.5, 79.5 );

  int minthr = 255;
  int maxthr = 0;

  for( int col = 0; col < 52; ++col ) {

    cout << endl << setw( 2 ) << col << " ";
    tb.roc_Col_Enable( col, 1 );

    for( int row = 0; row < 80; ++row ) {
      //for( int row = 20; row < 21; ++row ) {

      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim );

      int thr = ThrPix( 0, col, row, Vcal, step, nTrig );
      /* before FW 4.4
      int thr = tb.PixelThreshold( col, row,
                                   start, step,
                                   thrLevel, nTrig,
                                   CalDel, xtalk, cals );
      // PixelThreshold ends with dclose
      */
      cout << setw( 4 ) << thr << flush;
      if( row % 20 == 19 )
        cout << endl << "   ";
      Log.printf( " %i", thr );
      h21->Fill( col, row, thr );
      if( thr > maxthr )
        maxthr = thr;
      if( thr < minthr )
        minthr = thr;

      tb.roc_Pix_Mask( col, row );
      tb.roc_ClrCal();
      tb.Flush();

    } //rows

    Log.printf( "\n" );
    tb.roc_Col_Enable( col, 0 );
    tb.Flush();

  } // cols

  cout << endl;
  cout << "min thr " << setw( 3 ) << minthr << endl;
  cout << "max thr " << setw( 3 ) << maxthr << endl;

  tb.SetDAC( Vcal, vcal ); // restore dac value
  tb.SetDAC( CalDel, vdac ); // restore dac value
  tb.SetDAC( CtrlReg, vctl );

  tb.Flush();

  //double defaultLeftMargin = c1->GetLeftMargin();
  //double defaultRightMargin = c1->GetRightMargin();
  //c1->SetLeftMargin(0.10);
  //c1->SetRightMargin(0.18);
  h21->Write();
  h21->SetStats( 0 );
  h21->SetMinimum( minthr );
  h21->SetMaximum( maxthr );
  h21->GetYaxis()->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 21" << endl;
  //c1->SetLeftMargin(defaultLeftMargin);
  //c1->SetRightMargin(defaultRightMargin);

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // caldelmap

//------------------------------------------------------------------------------
CMD_PROC( modcaldel ) // set caldel for modules (using one pixel) 
{
  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  Log.section( "MODCALDEL", false );
  Log.printf( "pixel %i %i\n", col, row );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int nTrig = 10;

  int mm[16]; // responses per ROC

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << endl << "ROC " << setw( 2 ) << roc << endl;

    // scan caldel:

    bool ok = setcaldel( roc, col, row, nTrig );
    if( ok )
      mm[roc] = h11->GetMaximum();
    else
      mm[roc] = 0;

  } // rocs

  cout << endl;
  for( int roc = 0; roc < 16; ++roc )
    cout << setw( 2 ) << roc << " CalDel " << setw( 3 ) << dacval[roc][CalDel]
	 << " plateau height " << mm[roc]
	 << endl;

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modcaldel

//------------------------------------------------------------------------------
CMD_PROC( modpixsc ) // S-curve for modules, one pix per ROC
{
  int col, row, nTrig;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( nTrig, 1, 65000 );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // one pix per ROC:

  vector < uint8_t > rocAddress;

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    rocAddress.push_back( roc );
    tb.roc_I2cAddr( roc );
    tb.SetDAC( CtrlReg, 0 ); // small Vcal
    tb.roc_Col_Enable( col, 1 );
    int trim = modtrm[roc][col][row];
    tb.roc_Pix_Trim( col, row, trim );
    //tb.roc_Pix_Cal( col, row, false );
  }
  tb.uDelay( 1000 );
  tb.Flush();

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Open( Blocksize, ch );
#endif
  tb.Daq_Select_Deser400();
  tb.uDelay( 100 );
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Start( ch );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  uint16_t flags = 0; // or flags = FLAG_USE_CALS;
  //flags |= 0x0010; // FLAG_FORCE_MASK (needs trims)

  int dac = Vcal;
  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int nstp = dacstop - dacstrt + 1;

  gettimeofday( &tv, NULL );
  long s1 = tv.tv_sec; // seconds since 1.1.1970
  long u1 = tv.tv_usec; // microseconds

  try {
    tb.LoopMultiRocOnePixelDacScan( rocAddress, col, row, nTrig, flags, dac,
                                    dacstrt, dacstop );
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  gettimeofday( &tv, NULL );
  long s2 = tv.tv_sec; // seconds since 1.1.1970
  long u2 = tv.tv_usec; // microseconds
  double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;

  cout << "LoopMultiRocOnePixelDacScan takes " << dt << " s"
       << " = " << dt / 16 / nstp / nTrig * 1e6 << " us / pix" << endl;

  // read and unpack data:

  int cnt[16][256] = { {0} };

  // TBM channels:

  for( int ch = 0; ch < nChannels; ++ch ) {

    int dSize = tb.Daq_GetSize( ch );

    cout << "DAQ size channel " << ch
	 << " = " << dSize << " words " << endl;

    tb.Daq_Stop( ch );

    vector < uint16_t > data;
    data.reserve( dSize );

    try {
      uint32_t rest;
      tb.Daq_Read( data, Blocksize, rest, ch ); // 32768 gives zero
      cout << "data size " << data.size() << ", remaining " << rest << endl;
      while( rest > 0 ) {
        vector < uint16_t > dataB;
        dataB.reserve( Blocksize );
        tb.uDelay( 5000 );
        tb.Daq_Read( dataB, Blocksize, rest, ch );
        data.insert( data.end(), dataB.begin(), dataB.end() );
        cout << "data size " << data.size()
	     << ", remaining " << rest << endl;
        dataB.clear();
      }
    }
    catch( CRpcError & e ) {
      e.What();
      return 0;
    }

    // decode:

    bool ldb = 0;

    uint32_t nev = 0;
    uint32_t nrh = 0; // ROC headers
    uint32_t npx = 0;
    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
    uint8_t idc = 0;

    // nDAC * nTrig * (TBM header, some ROC headers, one pixel, more ROC headers, TBM trailer)

    for( size_t i = 0; i < data.size(); ++i ) {

      int d = data[i] & 0xfff; // 12 bits data
      int v = ( data[i] >> 12 ) & 0xe; // 3 flag bits

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;

      switch ( v ) {

	// TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
	//DecodeTbmHeader(hdr);
        if( ldb )
          cout << "event " << setw( 6 ) << nev;
        kroc = (16/nChannels) * ch - 1; // new event, will start at 0 or 8
	if( L1 ) kroc = ch2roc[ch] - 1;
        idc = nev / nTrig; // 0..255
        break;

	// ROC header data:
      case 4:
        ++kroc; // start at 0
        if( ldb ) {
          if( kroc > 0 )
            cout << endl;
          cout << "ROC " << setw( 2 ) << kroc;
        }
        if( kroc > 15 ) {
          cout << "Error kroc " << kroc << endl;
          kroc = 15;
        }
        ++nrh;
        break;

	// pixel data:
      case 0:
        raw = d;
        break;
      case 2:
        raw = ( raw << 12 ) + d;
	//DecodePixel(raw);
        ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
        raw >>= 9;
	if( Chip < 600 ) {
	  c = ( raw >> 12 ) & 7;
	  c = c * 6 + ( ( raw >> 9 ) & 7 );
	  r = ( raw >> 6 ) & 7;
	  r = r * 6 + ( ( raw >> 3 ) & 7 );
	  r = r * 6 + ( raw & 7 );
	  y = 80 - r / 2;
	  x = 2 * c + ( r & 1 );
	} // psi46dig
	else {
	  y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
	  x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
	} // proc600
        if( ldb )
          cout << setw( 3 ) << kroc << setw( 3 )
	       << x << setw( 3 ) << y << setw( 4 ) << ph;
        ++npx;
        if( x == col && y == row && kroc >= 0 && kroc < 16 )
          ++cnt[kroc][idc];
        break;

	// TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
	//DecodeTbmTrailer(trl);
        if( ldb )
          cout << endl;
        ++nev;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    cout << "TBM ch " << ch
	 << ": events " << nev
	 << " (expect " << nstp * nTrig << ")"
	 << ", ROC headers " << nrh
	 << " (expect " << nstp * nTrig * 8 << ")" << ", pixels " << npx << endl;

  } // ch

  gettimeofday( &tv, NULL );
  long s3 = tv.tv_sec; // seconds since 1.1.1970
  long u3 = tv.tv_usec; // microseconds
  double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
  cout << "Daq_Read takes " << dtr << " s"
       << endl;

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << "ROC " << setw( 2 ) << roc << ":" << endl;

    int nmx = 0;
    for( int idc = 0; idc < nstp; ++idc )
      if( cnt[roc][idc] > nmx )
	nmx = cnt[roc][idc];

    int i10 = 0;
    int i50 = 0;
    int i90 = 0;

    for( int idc = 0; idc < nstp; ++idc ) {

      int ni = cnt[roc][idc];
      cout << setw( 3 ) << ni;

      if( ni <= 0.1 * nmx )
	i10 = idc; // thr - 1.28155 * sigma
      if( ni <= 0.5 * nmx )
	i50 = idc; // thr
      if( ni <= 0.9 * nmx )
	i90 = idc; // thr + 1.28155 * sigma
      if( ni ==  nmx )
	break;
      //cout<< " idc:"<< idc << " i10: "<< i10<< " i50: "<< i50<< " i90: "<< i90<<endl;
    }
    cout << endl;
    cout << "thr " << i50 << ", width " << (i90 - i10)/2.5631 << endl;
  }

  // all off:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Close( ch );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modpixsc

//------------------------------------------------------------------------------
// utility function: loop over ROC 0 pixels, give triggers, count responses
// user: enable all

int GetEff( int &n01, int &n50, int &n99 )
{
  n01 = 0;
  n50 = 0;
  n99 = 0;

  uint16_t nTrig = 10;
  uint16_t flags = 0; // normal CAL

  //flags = 0x0002; // FLAG_USE_CALS;
  if( flags > 0 )
    cout << "CALS used..." << endl;

  flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  try {
    tb.LoopSingleRocAllPixelsCalibrate( 0, nTrig, flags );
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  // header = 1 word
  // pixel = +2 words
  // size = 4160 * nTrig * 3 = 124'800 words

  tb.Daq_Stop();

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize() );

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest );
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(), dataB.begin(), dataB.end() );
      dataB.clear();
    }
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  // unpack data:

  PixelReadoutData pix;

  int pos = 0;
  int err = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

      int cnt = 0;

      for( int k = 0; k < nTrig; ++k ) {

        err = DecodePixel( data, pos, pix ); // analyzer

        if( err )
          cout << "error " << err << " at trig " << k
	       << ", pix " << col << " " << row << ", pos " << pos << endl;
        if( err )
          break;

        if( pix.n > 0 )
          if( pix.x == col && pix.y == row )
            ++cnt;

      } // trig

      if( err )
        break;
      if( cnt > 0 )
        ++n01;
      if( cnt >= nTrig / 2 )
        ++n50;
      if( cnt == nTrig )
        ++n99;

    } // row

    if( err )
      break;

  } // col

  return 1;

} // GetEff

//------------------------------------------------------------------------------
// utility function: DAC scan for modules
  bool DacScanMod( int dac, int nTrig, int step, int stop, int strt )
{
  uint16_t mTrig = abs( nTrig );

  int nstp = ( stop - strt ) / step + 1;

  Log.section( "DACSCANMOD", true );
  Log.printf( "DAC %i from %i to %i in %i steps of %i with %i triggers\n",
	      dac, strt, stop, nstp, step, mTrig );

  cout << endl
       << "DacScanMod for " << dacName[dac]
       << " with " << mTrig << " triggers"
       << " from " << strt
       << " to " << stop
       << " in " << nstp
       << " steps of " << step << " units"
       << endl;
  cout << "  at CtrlReg " << dacval[0][CtrlReg] << endl;
  if( dac != 25 ) cout << "  Vcal " << dacval[0][Vcal] << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // all pix per ROC:

  vector < uint8_t > rocAddress;
  rocAddress.reserve( 16 );

  cout << "  set ROC";
  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << " " << roc;
    rocAddress.push_back( roc );
    tb.roc_I2cAddr( roc );

    vector < uint8_t > trimvalues( 4160 );

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        int trim = modtrm[roc][col][row];
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // row

    tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  } // roc

  tb.uDelay( 1000 );
  tb.Flush();

  cout << " = " << rocAddress.size() << " ROCs" << endl;

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Open( Blocksize, ch );
#endif
  tb.Daq_Select_Deser400();
  tb.uDelay( 100 );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  uint16_t flags = 0;
  if( nTrig < 0 ) {
    flags = 0x0002; // FLAG_USE_CALS
    cout << "  CALS used..." << endl;
    Log.printf( "Cals used\n" );
  }
  // flags |= 0x0010; // FLAG_FORCE_MASK is FPGA default

  uint8_t dacstrt = strt;
  uint8_t dacstop = stop;

  vector < uint16_t > data[8];
  for( int ch = 0; ch < nChannels; ++ch )
    data[ch].reserve( nstp * mTrig * 4160 * 28 ); // 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail

  bool done = 0;
  double dtloop = 0;
  double dtread = 0;
  int loop = 0;

  while( done == 0 ) { // FPGA sends blocks of data

    ++loop;
    cout << "\tloop " << loop << endl << flush;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Start( ch );
    tb.uDelay( 100 );

    try {
      done =
        tb.LoopMultiRocAllPixelsDacScan( rocAddress, mTrig, flags,
                                         dac, step, dacstrt, dacstop );
      // ~/psi/dtb/pixel-dtb-firmware/software/dtb_expert/trigger_loops.cpp
      // loop cols
      //   loop rows
      //     activate this pix on all ROCs
      //     loop dacs
      //       loop trig
    }
    catch( CRpcError & e ) {
      e.What();
      return 0;
    }

    cout << ( done ? "\tdone" : "\tnot done" ) << endl;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
    dtloop += s2 - s1 + ( u2 - u1 ) * 1e-6;

    // read:

    for( int ch = 0; ch < nChannels; ++ch ) {

      int dSize = tb.Daq_GetSize( ch );

      cout << "\tDAQ size channel " << ch
	   << " = " << dSize << " words " << endl;

      tb.Daq_Stop( ch );

      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );

      try {
        uint32_t rest;
        tb.Daq_Read( dataB, Blocksize, rest, ch );
        data[ch].insert( data[ch].end(), dataB.begin(), dataB.end() );
        cout << "\tdata[" << ch << "] size " << data[ch].size()
	  //<< ", remaining " << rest
	     << " of " << data[ch].capacity()
	     << endl;
        while( rest > 0 ) {
          dataB.clear();
          //tb.uDelay( 5000 );
          tb.Daq_Read( dataB, Blocksize, rest, ch );
          data[ch].insert( data[ch].end(), dataB.begin(), dataB.end() );
          cout << "\tdata[" << ch << "] size " << data[ch].size()
	    //<< ", remaining " << rest
	       << " of " << data[ch].capacity()
	       << endl;
        }
      }
      catch( CRpcError & e ) {
        e.What();
        return 0;
      }

    } // TBM ch

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

  } // while FPGA not done

  cout << "\tLoopMultiRocAllPixelDacScan takes " << dtloop << " s" << endl;
  cout << "\tDaq_Read takes " << dtread << " s"
       << " = " << 2 * ( data[0].size() + data[1].size() ) /
    dtread / 1024 / 1024 << " MiB/s" << endl;

  // decode:

  //int cnt[16][52][80][256] = {{{{0}}}}; // 17'039'360, seg fault (StackOverflow)

  int ****cnt = new int ***[16];
  for( int i = 0; i < 16; ++i )
    cnt[i] = new int **[52];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      cnt[i][j] = new int *[80];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
        cnt[i][j][k] = new int[nstp];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
        for( int l = 0; l < nstp; ++l )
          cnt[i][j][k][l] = 0;

  int ****amp = new int ***[16];
  for( int i = 0; i < 16; ++i )
    amp[i] = new int **[52];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      amp[i][j] = new int *[80];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
        amp[i][j][k] = new int[nstp];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
        for( int l = 0; l < nstp; ++l )
          amp[i][j][k][l] = 0;

  bool ldb = 0;

  for( int ch = 0; ch < nChannels; ++ch ) {

    cout << "\tDAQ size channel " << ch
	 << " = " << data[ch].size() << " words " << endl;

    uint32_t nev = 0;
    uint32_t nrh = 0; // ROC headers
    uint32_t npx = 0;
    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
    uint8_t idc = 0;

    // nDAC * nTrig * ( TBM header, 8 * ( ROC header, one pixel ), TBM trailer )

    for( size_t i = 0; i < data[ch].size(); ++i ) {

      int d = data[ch].at( i ) & 0xfff; // 12 bits data
      int v = ( data[ch].at( i ) >> 12 ) & 0xf; // 3 flag bits

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;

      switch ( v ) {

	// TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
	//DecodeTbmHeader(hdr);
        if( ldb )
          cout << "event " << setw( 6 ) << nev;
        kroc = (16/nChannels) * ch - 1; // new event, will start at 0 or 8
	if( L1 ) kroc = ch2roc[ch] - 1;
        idc = ( nev / mTrig ) % nstp; // 0..nstp-1
        break;

	// ROC header data:
      case 4:
        ++kroc; // start at 0 or 8
        if( ldb ) {
          if( kroc > 0 )
            cout << endl;
          cout << "ROC " << setw( 2 ) << kroc;
        }
        if( kroc > 15 ) {
          cout << "\tError kroc " << kroc << endl;
          kroc = 15;
        }
        ++nrh;
        break;

	// pixel data:
      case 0:
        raw = d;
        break;
      case 2:
        raw = ( raw << 12 ) + d;
	//DecodePixel(raw);
        ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
        raw >>= 9;
	if( Chip < 600 ) {
	  c = ( raw >> 12 ) & 7;
	  c = c * 6 + ( ( raw >> 9 ) & 7 );
	  r = ( raw >> 6 ) & 7;
	  r = r * 6 + ( ( raw >> 3 ) & 7 );
	  r = r * 6 + ( raw & 7 );
	  y = 80 - r / 2;
	  x = 2 * c + ( r & 1 );
	} // psi46dig
	else {
	  y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
	  x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
	} // proc600
        if( ldb )
          cout << setw( 3 ) << kroc << setw( 3 )
	       << x << setw( 3 ) << y << setw( 4 ) << ph;
        ++npx;
        if( x >= 0 && x < 52 && y >= 0 && y < 80 && kroc >= 0 && kroc < 16 ) {
          ++cnt[kroc][x][y][idc];
          amp[kroc][x][y][idc] += ph;
	}
        break;

	// TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
	//DecodeTbmTrailer(trl);
        if( ldb )
          cout << endl;
        ++nev;
        break;

      default:
        printf( "\nunknown case: %X = %d", v, v );
        break;

      } // switch

    } // data

    cout << "\tTBM ch " << ch
	 << ": events " << nev
	 << " (expect " << 4160 * nstp * mTrig << ")"
	 << ", ROC headers " << nrh
	 << " (expect " << 4160 * nstp * mTrig * 8 << ")"
	 << ", pixels " << npx << endl;

  } // ch

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "maxResponseDist",
          "maximum response distribution;maximum responses;pixels",
          mTrig + 1, -0.5, mTrig + 0.5 );
  h11->Sumw2();

  if( h14 )
    delete h14;
  h14 = new
    TH1D( "thrDist",
	  "Threshold distribution;threshold [small Vcal DAC];pixels",
	  255, -0.5, 254.5 );
  h14->Sumw2();

  if( h15 )
    delete h15;
  h15 = new
    TH1D( "noiseDist",
          "Noise distribution;width of threshold curve [small Vcal DAC];pixels",
          51, -0.5, 50.5 );
  h15->Sumw2();

  dacstop = dacstrt + (nstp-1)*step; // 255 or 254 (23.8.2014)
  int ctl = dacval[0][CtrlReg];
  int cal = dacval[0][Vcal];

  if( h21 )
    delete h21;
  if( nTrig < 0 ) // cals
    h21 = new TH2D( Form( "cals_N_DAC%02i_map", dac ),
		    Form( "cals N vs %s map;pixel;%s [DAC];responses",
			  dacName[dac].c_str(),
			  dacName[dac].c_str() ),
		    16*4160, -0.5, 16*4160-0.5,
		    nstp, dacstrt - 0.5*step, dacstop + 0.5*step );
  else
    h21 = new TH2D( Form( "N_DAC%02i_CR%i_Vcal%03i_map", dac, ctl, cal ),
		    Form( "N vs %s CR %i Vcal %i map;pixel;%s [DAC];responses",
			  dacName[dac].c_str(), ctl, cal,
			  dacName[dac].c_str() ),
		    16*4160, -0.5, 16*4160-0.5,
		    nstp, dacstrt - 0.5*step, dacstop + 0.5*step );

  if( h22 )
    delete h22;
  if( nTrig < 0 ) // cals
    h22 = new TH2D( Form( "cals_PH_DAC%02i_map", dac ),
		    Form( "cals PH vs %s map;pixel;%s [DAC];cals PH [ADC]",
			  dacName[dac].c_str(),
			  dacName[dac].c_str() ),
		    16*4160, -0.5, 16*4160-0.5,
		    nstp, dacstrt - 0.5*step, dacstop + 0.5*step );
  else
    h22 = new TH2D( Form( "PH_DAC%02i_CR%i_Vcal%03i_map", dac, ctl, cal ),
		    Form( "PH vs %s CR %i Vcal %i map;pixel;%s [DAC];PH [ADC]",
			  dacName[dac].c_str(), ctl, cal,
			  dacName[dac].c_str() ),
		    16*4160, -0.5, 16*4160-0.5,
		    nstp, dacstrt - 0.5*step, dacstop + 0.5*step );

  if( h23 )
    delete h23;
  h23 = new TH2D( "ModuleResponseMap",
                  "Module response map;col;row;responses",
                  8 * 52, 0, 8 * 52, 2 * 80, 0, 2 * 80 );
  h23->GetYaxis()->SetTitleOffset( 1.3 );

  if( h24 )
    delete h24;
  h24 = new TH2D( "ModuleThrMap",
                  "Module threshold map;col;row;threshold [small Vcal DAC]",
                  8 * 52, 0, 8 * 52, 2 * 80, 0, 2 * 80 );
  h24->GetYaxis()->SetTitleOffset( 1.3 );

  if( h25 )
    delete h25;
  h25 = new TH2D( "ModuleNoiseMap",
                  "Module noise map;col;row;width of threshold curve [small Vcal DAC]",
                  8 * 52, 0, 8 * 52, 2 * 80, 0, 2 * 80 );
  h25->GetYaxis()->SetTitleOffset( 1.3 );

  int iw = 3; // print format
  if( mTrig > 99 )
    iw = 4;

  // plot data:

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    if( ldb )
      cout << "ROC " << setw( 2 ) << roc << ":" << endl;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        int nmx = 0;
        for( int idc = 0; idc < nstp; ++idc )
          if( cnt[roc][col][row][idc] > nmx )
            nmx = cnt[roc][col][row][idc];

        int i10 = 0;
        int i50 = 0;
        int i90 = 0;
        bool ldb = 0;
        if( col == 22 && row == -33 )
          ldb = 1; // one example pix

        for( int idc = 0; idc < nstp; ++idc ) {

          int ni = cnt[roc][col][row][idc];
          if( ldb )
            cout << setw( iw ) << ni;

	  uint32_t vdc = dacstrt + step*idc;

	  int ipx = 4160*roc + 80*col + row; // 0..66559

	  if( ni > 0 )
	    h22->Fill( ipx, vdc, amp[roc][col][row][idc] / ni );

	  h21->Fill( ipx, vdc, ni );

	  if( ni <= 0.1 * nmx )
	    i10 = vdc; // thr - 1.28155 * sigma
	  if( ni <= 0.5 * nmx )
	    i50 = vdc; // thr
	  if( ni <= 0.9 * nmx )
	    i90 = vdc; // thr + 1.28155 * sigma
	}

        if( ldb )
          cout << endl;
        if( ldb )
          cout << "thr " << i50
	       << ", width " << (i90 - i10)/2.5631*step << endl;

        if( dac == 25 && nTrig > 0 && dacval[roc][CtrlReg] == 0 )
	  modthr[roc][col][row] = i50;

        h11->Fill( nmx );
        h14->Fill( i50 );
        h15->Fill( (i90 - i10)/2.5631*step );
        int l = roc % 8; // 0..7
        int m = roc / 8; // 0..1
        int xm = 52 * l + col; // 0..415  rocs 0 1 2 3 4 5 6 7
        if( m == 1 )
          xm = 415 - xm; // rocs 8 9 A B C D E F
        int ym = 159 * m + ( 1 - 2 * m ) * row; // 0..159
        h23->Fill( xm+0.2, ym+0.2, nmx );
        h24->Fill( xm+0.2, ym+0.2, i50 );
        h25->Fill( xm+0.2, ym+0.2, (i90 - i10)/2.5631*step );

      } // row

    } // col

  } // roc

   if( nTrig < 0 && dac == 12 ) { // BB test for module

    if( h12 )
      delete h12;
    h12 = new TH1D( "CalsVthrPlateauWidth",
		    "Width of VthrComp plateau for cals;width of VthrComp plateau for cals [DAC];pixels",
		    160/step, 0, 160 );

    if( h24 )
      delete h24;
    h24 = new TH2D( "BBtestMap",
		    "BBtest map;col;row;cals VthrComp response plateau width",
		    8*52, 0, 8 * 52, 2 * 80, 0, 2 * 80 );

    if( h25 )
      delete h25;
    h25 = new TH2D( "BBqualityMap",
		    "BBtest quality map;col;row;dead missing good",
		    8*52, 0, 8 * 52, 2 * 80, 0, 2 * 80 );

    // Localize missing Bumps from h21

    int nbinx = h21->GetNbinsX(); // pix
    int nbiny = h21->GetNbinsY(); // dac

    int ipx;

    // pixels:

    // get width of Vthrcomp plateau distribution:

    for( int ibin = 1; ibin <= nbinx; ++ibin ) {

      ipx = h21->GetXaxis()->GetBinCenter( ibin );
      int roc = ipx / ( 80 * 52 );
      ipx = ipx - roc * ( 80 * 52);
      // max response:

      int imax = 0;

      for( int j = 1; j <= nbiny; ++j ) {
	int cnt = h21->GetBinContent( ibin, j );
	if( cnt > imax )
	  imax = cnt;
      }

      if( imax >= mTrig / 2 ) {
 
	// Search for the Plateau

	int iEnd = 0;
	int iBegin = 0;

	// search for DAC response plateau:

	for( int jbin = 0; jbin <= nbiny; ++jbin ) {

	  int idac = h21->GetYaxis()->GetBinCenter( jbin );

	  int cnt = h21->GetBinContent( ibin, jbin );

	  if( cnt >= imax / 2 ) {
	    iEnd = idac; // end of plateau
	    if( iBegin == 0 )
	      iBegin = idac; // begin of plateau
	  }
	}

	// cout << "Bin: " << ibin << " Plateau Begins and End in  " << iBegin << " - " << iEnd << endl;
	// narrow plateau is from noise

	h12->Fill( iEnd - iBegin );

      } // nTrig

    } // pix

    // analyze peak in VthrCompPlateauWidth:

    int maxbin = h12->GetMaximumBin();
    double ymax = h12->GetBinContent( maxbin );
    double xmax = h12->GetBinCenter( maxbin );
    cout << "  analyze VthrCompPlateauWidth:" << endl;
    cout << "  max " << ymax << " pixels at " << xmax << endl;

    // go left from main peak:

    double xi = 0;
    double xj = 0;
    double ni = 0;
    double nj = 0;
    double y50 = 0.5*ymax;
    double sigma = h12->GetRMS();
    cout << "  RMS " << sigma << endl;

    for( int ii = maxbin; ii > 1; --ii ) {

      int jj = ii - 1;

      if( h12->GetBinContent(jj) < y50 && // jj is below
	  h12->GetBinContent(ii) >= y50 ) { // ii is above

	ni = h12->GetBinContent(ii);
	nj = h12->GetBinContent(jj);
	xi = h12->GetBinCenter(ii);
	xj = h12->GetBinCenter(jj);
	double dx = xi - xj;
	double dn = ni - nj;
	double x50 = xj + ( y50 - nj ) / dn * dx; // 1.3863 sigma for a Gaussian
	sigma = ( xmax - x50 ) / 1.3863;
	break; // end loop
      }

    } // ii

    cout << "  50% " << nj << " at " << xj << ", sigma " << sigma << endl;

    // Gaussian: N = nmax*exp(-(xmax-x)^2/2/sigma^2)
    // expect N = 1 at x1
    // log(1) = 0 = log(nmax) - (xmax-x1)^2/2/sigma^2
    // => x1 = xmax - sigma*sqrt(2*log(nmax))

    double x1 = xmax - sigma*sqrt(2*log(ymax));

    cout << "  x(1) at " << x1 << endl;

    double cut = x1 - sigma; // be generous

    if( cut > 40 ) cut = 40;
    
    cout << "  cut in VthrCompPlateauWidth at " << cut << endl;
    Log.printf( "cut in VthrCompPlateauWidth at %f\n", cut );

    int nFull = 0;
    int nGood = 0;
    int nWeak = 0;
    int nDead = 0;
    int nMissing = 0;

    for( int ibin = 1; ibin <= nbinx; ++ibin ) {

      ipx = h21->GetXaxis()->GetBinCenter( ibin );
      int roc = ipx / ( 80 * 52 );
      ipx = ipx - roc * ( 80 * 52);

      // max response:

      int imax = 0;
      for( int j = 1; j <= nbiny; ++j ) {
	int cnt = h21->GetBinContent( ibin, j );
	if( cnt > imax )
	  imax = cnt;
      }

      if( imax == 0 ) {
	++nDead;
	cout << "  Dead pixel at roc col row: " << roc
	     << " " << ipx / 80
	     << " " << ipx % 80
	     << endl;
      }
      else if( imax < mTrig / 2 ) {
	++nWeak;
	cout << "  Weak pixel at roc col row: " << roc
	     << " " << ipx / 80
	     << " " << ipx % 80
	     << endl;
      }
      else {
 
	// Search for the Plateau

	int iEnd = 0;
	int iBegin = 0;

	// search for DAC response plateau:

	for( int jbin = 0; jbin <= nbiny; ++jbin ) {

	  int idac = h21->GetYaxis()->GetBinCenter( jbin );

	  int cnt = h21->GetBinContent( ibin, jbin );

	  if( cnt >= imax / 2 ) {
	    iEnd = idac; // end of plateau
	    if( iBegin == 0 )
	      iBegin = idac; // begin of plateau
	  }
	}

	// cout << "Bin: " << ibin << " Plateau Begins and End in  " << iBegin << " - " << iEnd << endl;
	// narrow plateau is from noise

	int row = ipx % 80;
	int col = ipx / 80;
	int l = roc % 8; // 0..7
	int m = roc / 8; // 0..1
	int xm = 52 * l + col; // 0..415 rocs 0 1 2 3 4 5 6 7
	if( m == 1 )
	  xm = 415 - xm; // rocs 8 9 A B C D E F
	int ym = 159 * m + ( 1 - 2 * m ) * row; // 0..159

	h24->Fill(  xm+0.2, ym+0.2, iEnd - iBegin );

	if( iEnd - iBegin < cut ) {

	  ++nMissing;
	  cout << "  Missing bump at roc col row: " << roc
	       << " " << ipx / 80
	       << " " << ipx % 80
	       << endl;
	  Log.printf( "[Missing Bump at roc col row:] %i %i %i\n", roc, ipx / 80, ipx % 80 );
	  h25->Fill( xm, ym, 1 ); // red
	}
	else {
	  h25->Fill( xm+0.2, ym+0.2, 2 ); // green
	  if( imax == mTrig )
	    ++nFull;
	  else
	    ++nGood;
	} // plateau width

      } // active imax

    } // x-bins

    h12->Write();
    h21->Draw( "colz" );

    Log.printf( "Perfect bump bonds [100%]: %i\n", nFull );
    Log.printf( "Active bump bonds [above 50%]: %i\n", nGood );
    Log.printf( "Missing bump bonds: %i\n", nMissing );
    Log.printf( "Weak pixel: %i\n", nWeak );
    Log.printf( "Dead pixel: %i\n", nDead );

    cout << "  Perfect bump bonds: " << nFull << endl;
    cout << "  Active  bump bonds: " << nGood << endl;
    cout << "  Missing bump bonds: " << nMissing << endl;
    cout << "  Weak pixels:        " << nWeak << endl;
    cout << "  Dead pixels:        " << nDead << endl;

    h24->Write();
    h25->Write();

    h25->SetStats( 0 );
    h25->SetMinimum(0);
    h25->SetMaximum(2);
    h25->Draw( "colz" );
    c1->Update();

  } // BB test

  // De-allocate memory to prevent memory leak

  for( int i = 0; i < 16; ++i ) {
    for( int j = 0; j < 52; ++j ) {
      for( int k = 0; k < 80; ++k )
        delete[]cnt[i][j][k];
      delete[]cnt[i][j];
    }
    delete[]cnt[i];
  }
  delete[]cnt;

  for( int i = 0; i < 16; ++i ) {
    for( int j = 0; j < 52; ++j ) {
      for( int k = 0; k < 80; ++k )
        delete[]amp[i][j][k];
      delete[]amp[i][j];
    }
    delete[]amp[i];
  }
  delete[]amp;

  h11->Write();
  h14->Write();
  h15->Write();
  h21->Write();
  h22->Write();
  h23->Write();
  c1->Update();
  cout << "  histos 11, 12, 14, 15, 21 big, 22 big, 23, 24, 25" << endl;

  // all off:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    tb.SetDAC( Vcal, dacval[roc][dac] ); // restore
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Close( ch );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // DacScanMod

//------------------------------------------------------------------------------
CMD_PROC( dacscanmod ) // DAC scan for modules, all pix
// S-curve: dacscanmod 25 16 2 99 takes 124 s
// dacscanmod dac [[-]ntrig] [step] [stop] [strt]
{
  int dac;
  PAR_INT( dac, 1, 32 ); // only DACs, not registers

  if( dacval[0][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  int nTrig;
  PAR_INT( nTrig, -65000, 65000 );

  int step = 1;
  if( !PAR_IS_INT( step, 1, 255 ) )
    step = 1;

  int stop;
  if( !PAR_IS_INT( stop, 1, 255 ) )
    stop = dacStop( dac );

  int strt;
  if( !PAR_IS_INT( strt, 0, 255 ) )
    strt = dacStrt( dac );

  return DacScanMod( dac, nTrig, step, stop, strt );

} // dacscanmod

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: print ROC threshold map

void printThrMap( const bool longmode, int roc, int &nok )
{
  int sum = 0;
  nok = 0;
  int su2 = 0;
  int vmin = 255;
  int vmax = 0;
  int colmin = -1;
  int rowmin = -1;
  int colmax = -1;
  int rowmax = -1;

  cout << "thresholds for ROC: " << setw( 2 ) << roc << endl;

  for( int row = 79; row >= 0; --row ) {

    if( longmode )
      cout << setw( 2 ) << row << ":";

    for( int col = 0; col < 52; ++col ) {

      int thr = modthr[roc][col][row];
      if( longmode )
        cout << " " << thr;
      if( col == 25 )
        if( longmode )
          cout << endl << "   ";
      Log.printf( " %i", thr );

      if( thr < 255 ) {
        sum += thr;
        su2 += thr * thr;
        ++nok;
      }
      else
        cout << "  thr " << setw( 3 ) << thr
	     << " for pix " << setw( 2 ) << col << setw( 3 ) << row
	     << " at trim " << setw( 2 ) << modtrm[roc][col][row]
	     << endl;

      if( thr > 0 && thr < vmin ) {
        vmin = thr;
        colmin = col;
        rowmin = row;
      }

      if( thr > vmax && thr < 255 ) {
        vmax = thr;
        colmax = col;
        rowmax = row;
      }

    } // cols

    if( longmode )
      cout << endl;
    Log.printf( "\n" );

  } // rows

  Log.flush();

  cout << "  valid thresholds " << nok << endl;
  if( nok > 0 ) {
    cout << "  min thr " << vmin
	 << " at " << setw( 2 ) << colmin << setw( 3 ) << rowmin << endl;
    cout << "  max thr " << vmax
	 << " at " << setw( 2 ) << colmax << setw( 3 ) << rowmax << endl;
    double mid = ( double ) sum / ( double ) nok;
    double rms = sqrt( ( double ) su2 / ( double ) nok - mid * mid );
    cout << "  mean thr " << mid << ", rms " << rms << endl;
  }
  cout << "  VthrComp " << dacval[roc][VthrComp] << endl;
  cout << "  Vtrim " << dacval[roc][Vtrim] << endl;
  cout << "  CalDel " << dacval[roc][CalDel] << endl;

} // printThrMap

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 7.7.2014: module threshold map

void ModThrMap( int strt, int stop, int step, int nTrig, int xtlk, int cals )
{
  if( strt < 0 ) {
    cout << "ModThrMap: bad start " << strt << endl;
    return;
  }
  if( stop > 255 ) {
    cout << "ModThrMap: bad stop " << stop << endl;
    return;
  }
  if( strt > stop ) {
    cout << "ModThrMap: sttr " << strt << " > stop " << stop << endl;
    return;
  }
  if( step < 1 ) {
    cout << "ModThrMap: bad step " << step << endl;
    return;
  }
  if( nTrig < 1 ) {
    cout << "ModThrMap: bad nTrig " << nTrig << endl;
    return;
  }

  // all pix per ROC:

  vector < uint8_t > rocAddress;
  rocAddress.reserve( 16 );

  cout << "set ROC";

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << " " << roc;
    rocAddress.push_back( roc );
    tb.roc_I2cAddr( roc );

    tb.SetDAC( CtrlReg, 0 ); // small Vcal

    vector < uint8_t > trimvalues( 4160 );

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        int trim = modtrm[roc][col][row];
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // row

    tb.SetTrimValues( roc, trimvalues ); // load into FPGA (done before?)

  } // roc

  tb.uDelay( 1000 );
  tb.Flush();

  cout << " = " << rocAddress.size() << " ROCs" << endl;

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Open( Blocksize, ch );
#endif
  tb.Daq_Select_Deser400();
  tb.uDelay( 100 );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  uint16_t flags = 0;
  if( cals )
    flags = 0x0002; // FLAG_CALS
  if( xtlk )
    flags |= 0x0004; // FLAG_XTALK, see pxar/core/api/api.h

  //flags |= 0x0010; // FLAG_FORCE_MASK (is default)

  uint8_t dac = Vcal;
  int nstp = ( stop - strt ) / step + 1;

  cout << "scan DAC " << int ( dac )
       << " from " << int ( strt )
       << " to " << int ( stop )
       << " in " << nstp << " steps of " << int ( step )
       << endl;

  vector < uint16_t > data[8];
  for( int ch = 0; ch < nChannels; ++ch )
    data[ch].reserve( nstp * nTrig * 4160 * 28 ); // 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail

  bool done = 0;
  double dtloop = 0;
  double dtread = 0;
  timeval tv;
  int loop = 0;

  while( done == 0 ) {

    ++loop;
    cout << "\tloop " << loop << endl << flush;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Start( ch );
    tb.uDelay( 100 );

    try {
      done =
        tb.LoopMultiRocAllPixelsDacScan( rocAddress, nTrig, flags,
                                         dac, step, strt, stop );
      // ~/psi/dtb/pixel-dtb-firmware/software/dtb_expert/trigger_loops.cpp
      // loop cols
      //   loop rows
      //     activate this pix on all ROCs
      //     loop dacs
      //       loop trig
    }
    catch( CRpcError & e ) {
      e.What();
      return;
    }

    cout << ( done ? "\tdone" : "\tnot done" ) << endl;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
    dtloop += s2 - s1 + ( u2 - u1 ) * 1e-6;

    // read:

    for( int ch = 0; ch < nChannels; ++ch ) {

      int dSize = tb.Daq_GetSize( ch );
      cout << "\tDAQ size channel " << ch
	   << " = " << dSize << " words " << endl;

      tb.Daq_Stop( ch );

      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );

      try {
        uint32_t rest;
        tb.Daq_Read( dataB, Blocksize, rest, ch );
        data[ch].insert( data[ch].end(), dataB.begin(), dataB.end() );
        cout << "\tdata[" << ch << "] size " << data[ch].size()
	  //<< ", remaining " << rest
	     << " of " << data[ch].capacity()
	     << endl;
        while( rest > 0 ) {
          dataB.clear();
          tb.uDelay( 5000 );
          tb.Daq_Read( dataB, Blocksize, rest, ch );
          data[ch].insert( data[ch].end(), dataB.begin(), dataB.end() );
          cout << "\tdata[" << ch << "] size " << data[ch].size()
	    //<< ", remaining " << rest
	       << " of " << data[ch].capacity()
	       << endl;
        }
      }
      catch( CRpcError & e ) {
        e.What();
        return;
      }

    } // TBM ch

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

  } // while not done

  cout << "\tLoopMultiRocAllPixelDacScan takes " << dtloop << " s" << endl;
  cout << "\tDaq_Read takes " << dtread << " s"
       << " = " << 2 * ( data[0].size() +
			 data[1].size() ) / dtread / 1024 /
    1024 << " MiB/s" << endl;

  // decode:

  //int cnt[16][52][80][256] = {{{{0}}}}; // 17'039'360, seg fault (StackOverflow)

  int ****cnt = new int ***[16];
  for( int i = 0; i < 16; ++i )
    cnt[i] = new int **[52];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      cnt[i][j] = new int *[80];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
	//cnt[i][j][k] = new int[256];
        cnt[i][j][k] = new int[nstp];
  for( int i = 0; i < 16; ++i )
    for( int j = 0; j < 52; ++j )
      for( int k = 0; k < 80; ++k )
        for( int l = 0; l < nstp; ++l )
          cnt[i][j][k][l] = 0;

  bool ldb = 0;

  for( int ch = 0; ch < nChannels; ++ch ) {

    cout << "\tDAQ size channel " << ch
	 << " = " << data[ch].size() << " words " << endl;

    uint32_t nev = 0;
    uint32_t nrh = 0; // ROC headers
    uint32_t npx = 0;
    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
    uint8_t idc = 0; // 0..255

    // nDAC * nTrig * ( TBM header, 8 * ( ROC header, one pixel ), TBM trailer )

    for( size_t i = 0; i < data[ch].size(); ++i ) {

      int d = data[ch].at( i ) & 0xfff; // 12 bits data
      int v = ( data[ch].at( i ) >> 12 ) & 0xf; // 3 flag bits

      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;

      switch ( v ) {

	// TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
	//DecodeTbmHeader(hdr);
        if( ldb )
          cout << "event " << setw( 6 ) << nev;
        kroc = (16/nChannels) * ch - 1; // new event, will start at 0 or 8
	if( L1 ) kroc = ch2roc[ch] - 1;
        idc = ( nev / nTrig ) % nstp; // 0..nstp-1
        break;

	// ROC header data:
      case 4:
        ++kroc; // start at 0 or 8
        if( ldb ) {
          if( kroc > 0 )
            cout << endl;
          cout << "ROC " << setw( 2 ) << kroc;
        }
        if( kroc > 15 ) {
          cout << "Error kroc " << kroc << endl;
          kroc = 15;
        }
        ++nrh;
        break;

	// pixel data:
      case 0:
        raw = d;
        break;
      case 2:
        raw = ( raw << 12 ) + d;
	//DecodePixel(raw);
        ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
        raw >>= 9;
	if( Chip < 600 ) {
	  c = ( raw >> 12 ) & 7;
	  c = c * 6 + ( ( raw >> 9 ) & 7 );
	  r = ( raw >> 6 ) & 7;
	  r = r * 6 + ( ( raw >> 3 ) & 7 );
	  r = r * 6 + ( raw & 7 );
	  y = 80 - r / 2;
	  x = 2 * c + ( r & 1 );
	} // psi46dig
	else {
	  y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
	  x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
	} // proc600
        if( ldb )
          cout << setw( 3 ) << kroc << setw( 3 )
	       << x << setw( 3 ) << y << setw( 4 ) << ph;
        ++npx;
        if( x >= 0 && x < 52 && y >= 0 && y < 80 && kroc >= 0 && kroc < 16 )
          ++cnt[kroc][x][y][idc];
        break;

	// TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
	//DecodeTbmTrailer(trl);
        if( ldb )
          cout << endl;
        ++nev;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    cout << "\tTBM ch " << ch
	 << ": events " << nev
	 << " (expect " << 4160 * nstp * nTrig << ")"
	 << ", ROC headers " << nrh
	 << " (expect " << 4160 * nstp * nTrig * 8 << ")"
	 << ", pixels " << npx << endl;

  } // ch

  int iw = 3;
  if( nTrig > 99 )
    iw = 4;

  // analyze S-curves:

  bool lex = 0; // example

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;
    if( lex ) cout << "\tROC " << setw( 2 ) << roc << ":" << endl;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        int nmx = 0;
        for( int idc = 0; idc < nstp; ++idc )
          if( cnt[roc][col][row][idc] > nmx )
            nmx = cnt[roc][col][row][idc];

        int i10 = 0;
        int i50 = 0;
        int i90 = 0;
        bool ldb = 0;
        if( lex && col == 35 && row == 16 )
          ldb = 1; // one example pix

        for( int idc = 0; idc < nstp; ++idc ) {

          int ni = cnt[roc][col][row][idc];

          if( ldb )
            cout << setw( iw ) << ni;
	  if( ni <= 0.1 * nmx )
	    i10 = idc; // thr - 1.28155 * sigma
	  if( ni <= 0.5 * nmx )
	    i50 = idc; // thr
	  if( ni <= 0.9 * nmx )
	    i90 = idc; // thr + 1.28155 * sigma
	  if( ni == nmx )
            break;
        }

        if( ldb )
          cout << endl;
        if( ldb )
          cout << "\tthr " << i50 * step
	       << ", width " << ( (i90 - i10)/2.5631 ) * step << endl;

        modthr[roc][col][row] = i50 * step;

      } // row

    } // col

    int nok = 0;
    printThrMap( 0, roc, nok );

  } // roc

  // De-Allocate memory to prevent leaks:

  for( int i = 0; i < 16; ++i ) {
    for( int j = 0; j < 52; ++j ) {
      for( int k = 0; k < 80; ++k )
        delete[]cnt[i][j][k];
      delete[]cnt[i][j];
    }
    delete[]cnt[i];
  }
  delete[]cnt;

  // all off:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Close( ch );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  cout << endl; // after PrintThrMap

  if( h11 )
    delete h11;
  gStyle->SetOptStat( 1110 );
  h11 = new
    TH1D( Form( "mod_thr_dist" ),
	  Form( "Module threshold distribution;threshold [small Vcal DAC];pixels" ),
	  nstp, strt-0.5, stop+0.5 );
  h11->Sumw2();

  if( h21 )
    delete h21;
  h21 = new TH2D( "ModuleThresholdMap",
                  "Module threshold map;col;row;threshold [small Vcal DAC]",
                  8*52, -0.5, 8*52 - 0.5, 2*80, -0.5, 2*80 - 0.5 );
  h21->GetYaxis()->SetTitleOffset( 1.3 );

  int sum = 0;
  int nok = 0;
  int su2 = 0;
  int vmin = 255;
  int vmax = 0;
  int colmin = -1;
  int rowmin = -1;
  int rocmin = -1;
  int colmax = -1;
  int rowmax = -1;
  int rocmax = -1;

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    for( int col = 0; col < 52; ++col )

      for( int row = 0; row < 80; ++row ) {

	int l = roc % 8; // 0..7
	int xm = 52 * l + col; // 0..415  rocs 0 1 2 3 4 5 6 7
	int ym = row; // 0..79
	if( roc > 7 ) {
	  xm = 415 - xm; // rocs 8 9 A B C D E F
	  ym = 159 - row; // 80..159
	}

	int thr = modthr[roc][col][row];

	h11->Fill( thr );
	h21->Fill( xm, ym, thr );

	if( thr == 255 )
	  cout << "  thr " << setw( 3 ) << thr
	       << " for ROC col row trim "
	       << setw( 2 ) << roc
	       << setw( 3 ) << col
	       << setw( 3 ) << row
	       << setw( 3 ) << modtrm[roc][col][row]
	       << endl;
	else if( thr == 0 )
	  cout << "  thr " << setw( 3 ) << thr
	       << " for ROC col row trim "
	       << setw( 2 ) << roc
	       << setw( 3 ) << col
	       << setw( 3 ) << row
	       << setw( 3 ) << modtrm[roc][col][row]
	       << endl;
	else {
	  sum += thr;
	  su2 += thr * thr;
	  ++nok;
	  if( thr > 0 && thr < vmin ) {
	    vmin = thr;
	    colmin = col;
	    rowmin = row;
	    rocmin = roc;
	  }
	  if( thr > vmax && thr < 255 ) {
	    vmax = thr;
	    colmax = col;
	    rowmax = row;
	    rocmax = roc;
	  }

	} // valid

      } // rows

  } // rocs

  cout << "module valid thresholds " << nok << endl;
  if( nok > 0 ) {
    cout << "  min thr " << vmin
	 << " at ROC " << setw( 2 ) << rocmin
	 << " col " << setw( 2 ) << colmin
	 << " row " << setw( 2 ) << rowmin
	 << endl;
    cout << "  max thr " << vmax
	 << " at ROC " << setw( 2 ) << rocmax
	 << " col " << setw( 2 ) << colmax
	 << " row " << setw( 2 ) << rowmax
	 << endl;
    double mid = ( double ) sum / ( double ) nok;
    double rms = sqrt( ( double ) su2 / ( double ) nok - mid * mid );
    cout << "  mean thr " << mid << endl;
    cout << "  thr rms  " << rms << endl;
  }

  h11->Write();
  h21->Write();
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 21" << endl;

} // ModThrMap

//------------------------------------------------------------------------------
CMD_PROC( modthrmap ) // 211 s
{
  Log.section( "MODTHRMAP", true );

  int stop; // Vcal scan range
  if( !PAR_IS_INT( stop, 30, 255 ) )
    stop = 127; // Vcal range

  int step; // Vcal
  if( !PAR_IS_INT( step, 1, 16 ) )
    step = 1;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int strt = 0;

  const int nTrig = 10;
  const int xtlk = 0;
  const int cals = 0;

  cout << "measuring Vcal threshold map in range "
       << strt << " to " << stop
       << " in steps of " << step << " with " << nTrig << " triggers" << endl;

  ModThrMap( strt, stop, step, nTrig, xtlk, cals ); // fills modthr

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modthrmap

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 8.7.2014: set global threshold per ROC
CMD_PROC( modvthrcomp )
{
  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  Log.section( "MODVTHRCOMP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // load trim 15:

  int trim = 15; // 15 = off

  cout << "set all trim bits to " << trim << endl;

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    tb.roc_I2cAddr( roc );

    vector < uint8_t > trimvalues( 4160 );

    for( int row = 0; row < 80; ++row )
      for( int col = 0; col < 52; ++col ) {
        modtrm[roc][col][row] = trim;
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // row

    tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  } // rocs

  tb.uDelay( 1000 );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // take threshold map, find softest pixel

  int strt = 0;
  int stop = 255; // Vcal scan range
  int step = 4; // coarse
  if( target < 55 ) {
    stop = 127; // assume reasonable VthrComp
    step = 2; // finer
  }
  const int nTrig = 10;
  const int xtlk = 0;
  const int cals = 0;

  cout << "measuring Vcal threshold map in range "
       << strt << " to " << stop
       << " in steps of " << step << " with " << nTrig << " triggers" << endl;

  ModThrMap( strt, stop, step, nTrig, xtlk, cals ); // fills modthr

  cout << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // one pixel per ROC:

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    int colmin = -1;
    int rowmin = -1;
    int vmin = 255;

    for( int row = 0; row < 80; ++row )
      for( int col = 0; col < 52; ++col ) {

        int thr = modthr[roc][col][row];
	if( thr < 11 ) continue; // ignore
        if( thr < vmin ) {
          vmin = thr;
          colmin = col;
          rowmin = row;
        }

      } // cols

    cout << "ROC " << roc << endl;
    cout << "  min thr " << vmin << " at " << colmin << " " << rowmin << endl;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // setVthrComp using min pixel:

    tb.roc_I2cAddr( roc );

    tb.SetDAC( CtrlReg, 0 ); // small Vcal

    int dac = Vcal;
    step = 1; // fine Vcal

    // VthrComp has negative slope: higher = softer

    int vstp = -1; // towards harder threshold
    if( vmin > target )
      vstp = 1; // towards softer threshold

    int vthr = dacval[roc][VthrComp];

    for( ; vthr < 255 && vthr > 0; vthr += vstp ) {

      tb.SetDAC( VthrComp, vthr );
      tb.uDelay( 100 );
      tb.Flush();

      int thr = ThrPix( roc, colmin, rowmin, dac, step, nTrig );

      cout << "VthrComp " << setw( 3 ) << vthr << " thr " << setw( 3 ) << thr
	   << endl;
      if( vstp * thr <= vstp * target ) // signed
        break;

    } // vthr

    vthr -= 1; // safety margin towards harder threshold
    tb.SetDAC( VthrComp, vthr );
    cout << "set VthrComp to " << vthr << endl;
    dacval[roc][VthrComp] = vthr;
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    tb.Flush();

  } // rocs

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << "ROC " << setw( 2 ) << roc
	 << " VthrComp " << setw( 3 ) << dacval[roc][VthrComp]
	 << endl;
  }

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modvthrcomp

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 30.6.2016: set Vtrim from highest treshold pixel
CMD_PROC( modvtrim )
{
  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  Log.section( "MODVTRIM", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // load trim 15:

  int trim = 15; // 15 = off

  cout << "set all trim bits to " << trim << endl;

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    tb.roc_I2cAddr( roc );

    vector < uint8_t > trimvalues( 4160 );

    for( int row = 0; row < 80; ++row )
      for( int col = 0; col < 52; ++col ) {
        modtrm[roc][col][row] = trim;
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // row

    tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  } // rocs

  tb.uDelay( 1000 );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // take threshold map:

  int strt = 0;
  int stop = 127; // Vcal scan range
  int step = 2;   // medium

  const int nTrig = 10;
  const int xtlk = 0;
  const int cals = 0;

  cout << "measuring Vcal threshold map in range "
       << strt << " to " << stop
       << " in steps of " << step << " with " << nTrig << " triggers" << endl;

  ModThrMap( strt, stop, step, nTrig, xtlk, cals ); // fills modthr

  cout << endl;

  if( h11 )
    delete h11;
  h11 = new TH1D( "vtrim",
		  "Vtrim DAC;ROC;Vtrim [DAC]",
		  16, -0.5, 15.5 );

  // find hardest pixel:

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    tb.roc_I2cAddr( roc );
    cout << "ROC " << roc << endl;

    int colmax = -1;
    int rowmax = -1;
    int vmin = 255;
    int vmax = 0;

    for( int row = 0; row < 80; ++row )
      for( int col = 0; col < 52; ++col ) {

        int thr = modthr[roc][col][row];
        if( thr > vmax && thr < 255 ) {
          vmax = thr;
          colmax = col;
          rowmax = row;
        }
        if( thr < vmin && thr > 0 )
          vmin = thr;
      } // cols

    if( vmin < target ) {
      cout << "  min threshold " << vmin
	   << " below target on ROC " << roc
	   << endl;
      //continue; // skip this ROC
    }

    cout << "  max thr " << vmax << " at " << colmax << " " << rowmax << endl;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // setVtrim using max pixel:

    modtrm[roc][colmax][rowmax] = 0; // 0 = strongest

    tb.SetDAC( CtrlReg, 0 ); // this ROC, small Vcal

    int dac = Vcal;
    step = 1; // fine Vcal

    int itrim = 1;

    for( ; itrim < 253; itrim += 2 ) {

      tb.SetDAC( Vtrim, itrim );
      tb.uDelay( 100 );
      tb.Flush();

      int thr = ThrPix( roc, colmax, rowmax, dac, step, nTrig );

      //cout << setw( 3 ) << itrim << "  " << setw( 3 ) << thr << endl;
      if( thr < target )
	break; // done

    } // itrim

    itrim += 2; // margin
    // CS safety if something failed go to default value:
    if( itrim < 10 ) itrim = 180;
    tb.SetDAC( Vtrim, itrim );
    cout << "  set Vtrim to " << itrim << endl;
    dacval[roc][Vtrim] = itrim;
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    h11->Fill( roc, itrim );

    modtrm[roc][colmax][rowmax] = 15; // 15 = off

    tb.roc_Pix_Mask( colmax, rowmax );
    tb.roc_Col_Enable( colmax, 0 );
    tb.roc_ClrCal();
    tb.Flush();

  } // rocs

  h11->Draw( );
  c1->Update();
  h11->Write();
  cout << "  histos 11" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modvtrim

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 30.6.2016: scale trim bits
CMD_PROC( modtrimscale )
{
  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  Log.section( "MODTRIMSCALE", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // activate ROCs:

  int trim = 15; // 15 = off

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    tb.roc_I2cAddr( roc );

    vector < uint8_t > trimvalues( 4160 );

    for( int row = 0; row < 80; ++row )
      for( int col = 0; col < 52; ++col ) {
        modtrm[roc][col][row] = trim;
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // col

    tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  } // rocs

  tb.uDelay( 1000 );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // take threshold map:

  int strt = 0;
  int stop = 127; // Vcal scan range
  int step = 2;   // medium

  const int nTrig = 10;
  const int xtlk = 0;
  const int cals = 0;

  cout << "measuring Vcal threshold map in range "
       << strt << " to " << stop
       << " in steps of " << step << " with " << nTrig << " triggers" << endl;

  ModThrMap( strt, stop, step, nTrig, xtlk, cals ); // fills modthr

  // plot trim bits:

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "mod_trim_bits_scaled",
          "Module trim bits scaled;trim bits;pixels",
          16, -0.5, 15.5 );
  h12->Sumw2();

  // scale trim bits:

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    double scale = 1;

    int rocmaxthr = 0;

    for( int col = 0; col < 52; ++col )

      for( int row = 0; row < 80; ++row ) {

	int thr = modthr[roc][col][row];

	if( thr > rocmaxthr )
	  rocmaxthr = thr;
      }

    if( target >= rocmaxthr ) {
      cout << "ROC " << roc
	   << " max thr " << rocmaxthr
	   << " is below target " << target << endl
	   << "will not trim this ROC: request lower target or reduce VthrComp"
	   << endl;
      continue;
    }

    scale = ( rocmaxthr - target ) / 15.0; // trim bits
    cout << "  ROC " << roc
	 << " max thr " << rocmaxthr
	 << " trimbit scale " << scale << " Vcal/bit"
	 << endl;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        int thr = modthr[roc][col][row];

        int trim = modtrm[roc][col][row];

	trim += ( target - thr ) / scale; // linear extrapol

        if( trim < 0 )
          trim = 0;
        else if( trim > 15 )
          trim = 15;

        modtrm[roc][col][row] = trim;

	h12->Fill( trim );

      } // rows

    } // cols

  } // rocs

  h12->Write();
  h12->Draw( "hist" );
  c1->Update();

  cout << "  histos 11, 12, 21" << endl;

  // update:

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    tb.roc_I2cAddr( roc );

    vector < uint8_t > trimvalues( 4160 ); // uint8 in 2.15
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	int i = 80 * col + row;
	trimvalues[i] = modtrm[roc][col][row];
      }
    tb.SetTrimValues( roc, trimvalues );
  }

  tb.Flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modtrimscale

int trimiter = 1;

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 8.7.2014: adjust trim bits
CMD_PROC( modtrimstep )
{
  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  int correction = 1;
  int tol = 0; // Vcal thr tolerance

  Log.section( "MODTRIMSTEP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // activate ROCs:

  int maxthr = 0;
  int n0 = 0;
  int n255 = 0;

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    tb.roc_I2cAddr( roc );

    vector < uint8_t > trimvalues( 4160 );

    for( int row = 0; row < 80; ++row )
      for( int col = 0; col < 52; ++col ) {
        int trim = modtrm[roc][col][row];
        int i = 80 * col + row;
        trimvalues[i] = trim;
        int thr = modthr[roc][col][row];
        if( thr > maxthr )
          maxthr = thr;
	if( thr == 0 ) ++n0;
	if( thr == 255 ) ++n255;
      } // col

    tb.SetTrimValues( roc, trimvalues ); // load into FPGA

  } // rocs

  cout << n0 << " pixels have thr 0" << endl;
  cout << n255 << " pixels have thr 255" << endl;

  tb.uDelay( 1000 );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // take threshold map:

  int strt = 0;
  int step = 1; // fine
  int stop = maxthr + 4 * step; // Vcal scan range
  if( maxthr == 255 ) stop = 63; // assume good modtrimscale

  const int nTrig = 10;
  const int xtlk = 0;
  const int cals = 0;

  cout << "measuring Vcal threshold map in range "
       << strt << " to " << stop
       << " in steps of " << step << " with " << nTrig << " triggers" << endl;

  ModThrMap( strt, stop, step, nTrig, xtlk, cals ); // fills modthr

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // plot trim bits:

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "mod_trim_bits_%02i", trimiter ),
          Form( "Module trim bits step %02i;trim bits;pixels", trimiter ),
          16, -0.5, 15.5 );
  h12->Sumw2();

  // adjust trim bits:

  int nadj = 0;

  cout << endl
       << "TrimStep with trim bit correction " << correction
       << endl;

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    for( int row = 0; row < 80; ++row )

      for( int col = 0; col < 52; ++col ) {

        int thr = modthr[roc][col][row];

        int trim = modtrm[roc][col][row];

	if(      thr > target + tol && thr < 255 ) {
          trim -= correction; // make softer
	  ++nadj;
	}
        else if( thr < target - tol ) {
          trim += correction; // make harder
	  ++nadj;
	}

        if( trim < 0 )
          trim = 0;
        else if( trim > 15 )
          trim = 15;

        modtrm[roc][col][row] = trim;

	h12->Fill( trim );

      } // col

  } // rocs

  cout << "  trim bits adjusted for " << nadj << " pixels" << endl;

  h12->Write();
  h12->Draw( "hist" );
  c1->Update();

  cout << "  histos 11, 12, 21" << endl;

  // update:

  for( int roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    tb.roc_I2cAddr( roc );

    vector < uint8_t > trimvalues( 4160 ); // uint8 in 2.15
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	int i = 80 * col + row;
	trimvalues[i] = modtrm[roc][col][row];
      }
    tb.SetTrimValues( roc, trimvalues );
  }

  tb.Flush();

  ++trimiter;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  cout << "take one more modthrmap " << maxthr+4 << " to see final values" << endl;

  return true;

} // modtrimstep

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 20.7.2015, adjust trimbits of noisy pixels
CMD_PROC( modtrimbits ) // pgt
{
  int cut;
  if( !PAR_IS_INT( cut, 1, 9100200 ) )
    cut = 10; // hits/px in 1M trig

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int period = 400; // trigger rate = 40 MHz / period = 100 kHz

  int iter = 0;

  // plot trim bits:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "mod_trim_bits_%02i", iter ),
	  Form( "Module trim bits distribution %02i;trim bits;pixels", iter ),
	  16, -0.5, 15.5 );
  h11->Sumw2();

  // enable all pixels:

  int masked = 0;

  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.roc_ClrCal();
      for( int col = 0; col < 52; ++col ) {
        tb.roc_Col_Enable( col, 1 );
        for( int row = 0; row < 80; ++row ) {
          int trim = modtrm[roc][col][row];
	  h11->Fill( trim );
	  if( trim > 15 ) {
	    tb.roc_Pix_Mask( col, row );
	    ++masked;
	    cout << "mask roc col row "
		 << setw(2) << roc
		 << setw(3) << col
		 << setw(3) << row
		 << endl;
	  }
	  else
          tb.roc_Pix_Trim( col, row, trim );
        }
      }
    }

  tb.Flush();

  h11->Write();
  h11->Draw( "hist" );
  c1->Update();

  bool again = 0;

  do {

    unsigned int nev[8] = {  };
    unsigned int ndq = 0;
    unsigned int got = 0;
    unsigned int rst = 0;
    unsigned int npx = 0;
    unsigned int nrr[8] = {  };

    int PX[16] = { 0 };
    int ER[16] = { 0 };
    uint32_t NN[16][52][80] = { {{0}} };

    if( h21 )
      delete h21;
    h21 = new TH2D( "ModuleHitMap",
		    "Module response map;col;row;hits",
		    8 * 52, -0.5, 8 * 52 - 0.5,
		    2 * 80, -0.5, 2 * 80 - 0.5 );
    gStyle->SetOptStat( 10 ); // entries
    h21->GetYaxis()->SetTitleOffset( 1.3 );

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Open( Blocksize, ch );
#endif

    tb.Daq_Select_Deser400();
    tb.Daq_Deser400_Reset( 3 );
    tb.uDelay( 100 );

    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Start( ch );
    tb.uDelay( 1000 );

    tb.Pg_Loop( period ); // start triggers

    tb.Flush();

    uint32_t trl = 0; // need to remember from previous daq_read

    while( nev[0] < 1000*1000 && (int) npx < cut*66560 ) {

      tb.mDelay( 500 ); // once per second

      tb.Pg_Stop(); // stop triggers, necessary for clean data

      vector <uint16_t > data[8];

      for( int ch = 0; ch < nChannels; ++ch ) {

	tb.Daq_Stop( ch );

	data[ch].reserve( Blocksize );
	uint32_t remaining = 0;

	tb.Daq_Read( data[ch], Blocksize, remaining, 0 );

	got += data[ch].size();
	rst += remaining;

      } // ch

      ++ndq;
      tb.Daq_Deser400_Reset( 3 ); // more stable ?
      for( int ch = 0; ch < nChannels; ++ch )
	tb.Daq_Start( ch );

      tb.Pg_Loop( period ); // start triggers
      tb.Flush();

      bool ldb = 0;

      gettimeofday( &tv, NULL );
      long s2 = tv.tv_sec; // seconds since 1.1.1970
      long u2 = tv.tv_usec; // microseconds

      // decode data:

      for( int ch = 0; ch < nChannels; ++ch ) {

	int size = data[ch].size();
	uint32_t raw = 0;
	uint32_t hdr = 0;
	int32_t kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
	unsigned int npxev = 0;

	for( int ii = 0; ii < size; ++ii ) {

	  int d = 0;
	  int v = 0;
	  d = data[ch][ii] & 0xfff; // 12 bits data
	  v = ( data[ch][ii] >> 12 ) & 0xe; // 3 flag bits: e = 14 = 1110
	  int c = 0;
	  int r = 0;
	  int x = 0;
	  int y = 0;

	  switch ( v ) {

	    // TBM header:
	  case 10:
	    hdr = d;
	    npxev = 0;
	    if( nev[ch] > 0 && trl == 0 ) {
	      cout << "TBM error: header without previous trailer in event "
		   << nev[ch]
		   << ", channel " << ch
		   << endl;
	      ++nrr[ch];
	    }
	    trl = 0;
	    break;
	  case 8:
	    hdr = ( hdr << 8 ) + d;
	    if( ldb ) {
	      cout << "event " << nev[ch] << endl;
	      cout << "TBM header";
	      DecodeTbmHeader( hdr );
	      cout << endl;
	    }
	    kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
	    if( L1 ) kroc = ch2roc[ch] - 1;
	    break;

	    // ROC header data:
	  case 4:
	    ++kroc; // start at 0
	    if( ldb ) {
	      if( kroc > 0 )
		cout << endl;
	      cout << "ROC " << setw( 2 ) << kroc;
	    }
	    if( kroc > 15 ) {
	      cout << "Error kroc " << kroc << endl;
	      kroc = 15;
	      ++nrr[ch];
	    }
	    if( kroc == 0 && hdr == 0 ) {
	      cout << "TBM error: no header in event " << nev[ch]
		   << " ch " << ch
		   << endl;
	      ++nrr[ch];
	    }
	    hdr = 0;
	    break;

	    // pixel data:
	  case 0:
	    raw = d;
	    break;
	  case 2:
	    raw = ( raw << 12 ) + d;
	    raw >>= 9;
	    if( Chip < 600 ) {
	      c = ( raw >> 12 ) & 7;
	      c = c * 6 + ( ( raw >> 9 ) & 7 );
	      r = ( raw >> 6 ) & 7;
	      r = r * 6 + ( ( raw >> 3 ) & 7 );
	      r = r * 6 + ( raw & 7 );
	      y = 80 - r / 2;
	      x = 2 * c + ( r & 1 );
	    } // psi46dig
	    else {
	      y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
	      x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
	    } // proc600
	    if( ldb )
	      cout << " " << x << "." << y;
	    if( kroc < 0 || ( L4 && ch == 1 && kroc < 8 ) || kroc > 15 ) {
	      cout << "ROC data with wrong ROC count " << kroc
		   << " in event " << nev[ch]
		   << endl;
	      ++nrr[ch];
	    }
	    else if( y > -1 && y < 80 && x > -1 && x < 52 ) {
	      ++npx;
	      ++npxev;
	      ++PX[kroc];
	      NN[kroc][x][y] += 1;
	      int l = kroc % 8; // 0..7
	      int xm = 52 * l + x; // 0..415  rocs 0 1 2 3 4 5 6 7
	      int ym = y; // 0..79
	      if( kroc > 7 ) {
		xm = 415 - xm; // rocs 8 9 A B C D E F
		ym = 159 - y; // 80..159
	      }
	      h21->Fill( xm, ym );
	    } // valid px
	    else {
	      ++nrr[ch];
	      ++ER[kroc];
	      /*
	      cout << "invalid col row " << setw(2) << x
		   << setw(3) << y
		   << " in ROC " << setw(2) << kroc
		   << endl;
	      */
	    }
	    break;

	    // TBM trailer:
	  case 14:
	    trl = d;
	    if( L4 && ch == 0 && kroc != 7 ) {
	      cout
		<< "wrong ROC count " << kroc
		<< " in event " << nev[ch] << " ch 0" 
		<< endl;
	      ++nrr[ch];
	    }
	    else if( L4 && ch == 1 && kroc != 15 ) {
	      cout
		<< "wrong ROC count " << kroc
		<< " in event " << nev[ch] << " ch 1" 
		<< endl;
	      ++nrr[ch];
	    }
	    break;
	  case 12:
	    trl = ( trl << 8 ) + d;
	    if( ldb ) {
	      cout << endl;
	      cout << "TBM trailer";
	      DecodeTbmTrailer( trl );
	      cout << endl;
	    }
	    ++nev[ch];
	    trl = 1; // flag
	    break;

	  default:
	    printf( "\nunknown data: %X = %d", v, v );
	    ++nrr[ch];
	    break;

	  } // switch

	} // data

      } // ch

      if( ldb )
	cout << endl;

      h21->Draw( "colz" );
      c1->Update();

      cout << s2 - s0 + ( u2 - u0 ) * 1e-6 << " s"
	   << ",  calls " << ndq
	   << ",  last " << data[0].size() + data[1].size()
	   << ",  trig " << nev[0]
	   << ",  byte " << got
	   << ",  rest " << rst
	   << ",  pix " << npx
	   << ",  err " << nrr
	   << endl;

    } // while nev

    tb.Pg_Stop(); // stop triggers, necessary for clean data
    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Stop( ch );
    tb.Flush();

    // analyze hit map:

    cout << nev[0] << " triggers, noise cut " << cut << endl;

    again = 0;
    ++iter;

    for( int roc = 0; roc < 16; ++roc ) {

      if( roclist[roc] == 0 )
	continue;

      for( int col = 0; col < 52; ++col )

	for( int row = 0; row < 80; ++row ) {

	  int nn = NN[roc][col][row];

	  if( nn > cut ) {

	    again = 1;

	    hot[roc][col][row] = 1;

	    int trim = modtrm[roc][col][row];

	    cout << "roc col row trim n"
		 << setw( 3 ) << roc
		 << setw( 3 ) << col
		 << setw( 3 ) << row
		 << setw( 3 ) << trim
		 << setw( 7 ) << nn
		 << endl;

	    ++trim; // trim bits

	    modtrm[roc][col][row] = trim; // will be used next
	    tb.roc_I2cAddr( roc );
	    if( trim > 15 ) {
	      tb.roc_Pix_Mask( col, row );
	      ++masked;
	      cout << "mask roc col row "
		   << setw(2) << roc
		   << setw(3) << col
		   << setw(3) << row
		   << endl;
	    }
	    else
	      tb.roc_Pix_Trim( col, row, trim );

	  } // noisy

	} // rows

    } // rocs

    tb.Flush();

    // plot trim bits:

    if( h11 )
      delete h11;
    h11 = new
      TH1D( Form( "mod_trim_bits_%02i", iter ),
	    Form( "Module trim bits distribution %02i;trim bits;pixels", iter ),
	    16, -0.5, 15.5 );
    h11->Sumw2();

    unsigned int nht = 0;
    for( int roc = 0; roc < 16; ++roc ) {
      if( roclist[roc] == 0 )
	continue;
      for( int col = 0; col < 52; ++col )
        for( int row = 0; row < 80; ++row ) {
          h11->Fill( modtrm[roc][col][row] );
          if( hot[roc][col][row] )
            ++nht;
        }
    }
    h11->Write();
    h11->Draw( "hist" );
    c1->Update();

    cout << "masked pixels: " << masked << endl;

    cout << "hot " << nht << endl;

    cout << "iter " << iter << endl;

  }
  while( again && iter < 11 );

  if( again )
    cout << "reached iter limit: please run again to continue" << endl;

  // all off:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Close( ch );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  cout << "  histos 11" << endl;

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modtrimbits

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 27.6.2016, subtract 1 trim bit
CMD_PROC( subtb )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;

  // plot trim bits:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "mod_trim_bits",
	  "Module trim bits distribution;trim bits;pixels",
	  16, -0.5, 15.5 );
  h11->Sumw2();

  int masked = 0;

  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] )
      for( int col = 0; col < 52; ++col )
        for( int row = 0; row < 80; ++row ) {
          int trim = modtrm[roc][col][row];
          h11->Fill( trim );
          if( trim > 15 ) {
            ++masked;
            cout << "mask roc col row "
                 << setw(2) << roc
                 << setw(3) << col
                 << setw(3) << row
                 << endl;
          }
          else if( trim > 0 && hot[roc][col][row] == 0 )
            --modtrm[roc][col][row];
        }
  h11->Write();
  h11->Draw( "hist" );
  c1->Update();

  return true;

} // subtb

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 21.7.2015, adjust trimbits to noise limit
CMD_PROC( modnoise )
{
  int cut;
  if( !PAR_IS_INT( cut, 1, 9100200 ) )
    cut = 10; // hits/px in 1M trig

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int period = 400; // trigger rate = 40 MHz / period = 100 kHz

  int iter = 0;

  // plot trim bits:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "mod_trim_bits_%02i", iter ),
	  Form( "Module trim bits distribution %02i;trim bits;pixels", iter ),
	  16, -0.5, 15.5 );
  h11->Sumw2();

  // enable all pixels:

  int masked = 0;

  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.roc_ClrCal();
      for( int col = 0; col < 52; ++col ) {
        tb.roc_Col_Enable( col, 1 );
        for( int row = 0; row < 80; ++row ) {
          int trim = modtrm[roc][col][row];
	  h11->Fill( trim );
	  if( trim > 15 ) {
	    tb.roc_Pix_Mask( col, row );
	    ++masked;
	    cout << "mask roc col row "
		 << setw(2) << roc
		 << setw(3) << col
		 << setw(3) << row
		 << endl;
	  }
	  else
	    tb.roc_Pix_Trim( col, row, trim );
        }
      }
    }
  tb.Flush();
  h11->Write();
  h11->Draw( "hist" );
  c1->Update();

  bool again = 0;

  do {

    unsigned int nev[8] = {  };
    unsigned int ndq = 0;
    unsigned int got = 0;
    unsigned int rst = 0;
    unsigned int npx = 0;
    unsigned int nrr = 0;

    int PX[16] = { 0 };
    int ER[16] = { 0 };
    uint32_t NN[16][52][80] = { {{0}} };

    if( h21 )
      delete h21;
    h21 = new TH2D( "ModuleHitMap",
		    "Module hit map;col;row;hits",
		    8 * 52, -0.5, 8 * 52 - 0.5,
		    2 * 80, -0.5, 2 * 80 - 0.5 );
    gStyle->SetOptStat( 10 ); // entries
    h21->GetYaxis()->SetTitleOffset( 1.3 );

#ifdef DAQOPENCLOSE
    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Open( Blocksize, ch );
#endif

    tb.Daq_Select_Deser400();
    tb.Daq_Deser400_Reset( 3 );
    tb.uDelay( 100 );

    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Start( ch );
    tb.uDelay( 1000 );

    tb.Pg_Loop( period ); // start triggers

    tb.Flush();

    uint32_t trl = 0; // need to remember from previous daq_read

    while( nev[0] < 1000*1000 && (int) npx < cut*66560 ) {

      tb.mDelay( 500 ); // once per second

      tb.Pg_Stop(); // stop triggers, necessary for clean data

      vector <uint16_t > data[8];

      for( int ch = 0; ch < nChannels; ++ch ) {

	tb.Daq_Stop( ch );

	data[ch].reserve( Blocksize );
	uint32_t remaining = 0;
	tb.Daq_Read( data[ch], Blocksize, remaining, 0 );

	got += data[ch].size();
	rst += remaining;

      } // ch

      ++ndq;

      tb.Daq_Deser400_Reset( 3 ); // more stable ?
      for( int ch = 0; ch < nChannels; ++ch )
	tb.Daq_Start( ch );

      tb.Pg_Loop( period ); // start triggers
      tb.Flush();

      bool ldb = 0;

      gettimeofday( &tv, NULL );
      long s2 = tv.tv_sec; // seconds since 1.1.1970
      long u2 = tv.tv_usec; // microseconds

      // decode data:

      for( int ch = 0; ch < nChannels; ++ch ) {

	int size = data[ch].size();
	uint32_t raw = 0;
	uint32_t hdr = 0;
	int32_t kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
	unsigned int npxev = 0;

	for( int ii = 0; ii < size; ++ii ) {

	  int d = 0;
	  int v = 0;
	  d = data[ch][ii] & 0xfff; // 12 bits data
	  v = ( data[ch][ii] >> 12 ) & 0xe; // 3 flag bits: e = 14 = 1110
	  int c = 0;
	  int r = 0;
	  int x = 0;
	  int y = 0;

	  switch ( v ) {

	    // TBM header:
	  case 10:
	    hdr = d;
	    npxev = 0;
	    if( nev[ch] > 0 && trl == 0 )
	      cout << "TBM error: header without previous trailer in event "
		   << nev[ch]
		   << ", channel " << ch
		   << endl;
	    trl = 0;
	    break;
	  case 8:
	    hdr = ( hdr << 8 ) + d;
	    if( ldb ) {
	      cout << "event " << nev[ch] << endl;
	      cout << "TBM header";
	      DecodeTbmHeader( hdr );
	      cout << endl;
	    }
	    kroc = (16/nChannels) * ch - 1; // will start at 0 or 8
	    if( L1 ) kroc = ch2roc[ch] - 1;
	    break;

	    // ROC header data:
	  case 4:
	    ++kroc; // start at 0
	    if( ldb ) {
	      if( kroc > 0 )
		cout << endl;
	      cout << "ROC " << setw( 2 ) << kroc;
	    }
	    if( kroc > 15 ) {
	      cout << "Error kroc " << kroc << endl;
	      kroc = 15;
	    }
	    if( kroc == 0 && hdr == 0 )
	      cout << "TBM error: no header " << nev[ch] << endl;
	    hdr = 0;
	    break;

	    // pixel data:
	  case 0:
	    raw = d;
	    break;
	  case 2:
	    raw = ( raw << 12 ) + d;
	    raw >>= 9;
	    if( Chip < 600 ) {
	      c = ( raw >> 12 ) & 7;
	      c = c * 6 + ( ( raw >> 9 ) & 7 );
	      r = ( raw >> 6 ) & 7;
	      r = r * 6 + ( ( raw >> 3 ) & 7 );
	      r = r * 6 + ( raw & 7 );
	      y = 80 - r / 2;
	      x = 2 * c + ( r & 1 );
	    } // psi46dig
	    else {
	      y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
	      x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
	    } // proc600
	    if( ldb )
	      cout << " " << x << "." << y;
	    if( y > -1 && y < 80 && x > -1 && x < 52 ) {
	      ++npx;
	      ++npxev;
	      ++PX[kroc];
	      NN[kroc][x][y] += 1;
	      int l = kroc % 8; // 0..7
	      int xm = 52 * l + x; // 0..415  rocs 0 1 2 3 4 5 6 7
	      int ym = y; // 0..79
	      if( kroc > 7 ) {
		xm = 415 - xm; // rocs 8 9 A B C D E F
		ym = 159 - y; // 80..159
	      }
	      h21->Fill( xm, ym );
	    } // valid px
	    else {
	      ++nrr;
	      ++ER[kroc];
	      cout << "invalid col row " << setw(2) << x
		   << setw(3) << y
		   << " in ROC " << setw(2) << kroc
		   << endl;
	    }
	    break;

	    // TBM trailer:
	  case 14:
	    trl = d;
	    break;
	  case 12:
	    trl = ( trl << 8 ) + d;
	    if( ldb ) {
	      cout << endl;
	      cout << "TBM trailer";
	      DecodeTbmTrailer( trl );
	      cout << endl;
	    }
	    ++nev[ch];
	    trl = 1; // flag
	    break;

	  default:
	    printf( "\nunknown data: %X = %d", v, v );
	    break;

	  } // switch

	} // data

      } // ch

      if( ldb )
	cout << endl;

      h21->Draw( "colz" );
      c1->Update();

      cout << s2 - s0 + ( u2 - u0 ) * 1e-6 << " s"
	   << ",  calls " << ndq
	   << ",  last " << data[0].size() + data[1].size()
	   << ",  trig " << nev[0]
	   << ",  byte " << got
	   << ",  rest " << rst
	   << ",  pix " << npx
	   << ",  err " << nrr
	   << endl;

    } // while nev

    tb.Pg_Stop(); // stop triggers, necessary for clean data
    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Stop( ch );
    tb.Flush();

    // analyze hit map:

    cout << nev[0] << " triggers, noise cut " << cut << endl;

    again = 0;
    ++iter;

    for( int roc = 0; roc < 16; ++roc ) {

      if( roclist[roc] == 0 )
	continue;

      for( int col = 0; col < 52; ++col )

	for( int row = 0; row < 80; ++row ) {

	  int nn = NN[roc][col][row];

	  int trim = modtrm[roc][col][row];

	  if( nn > cut ) {

	    hot[roc][col][row] = 1;

	    cout << "roc col row"
		 << setw( 3 ) << roc
		 << setw( 3 ) << col
		 << setw( 3 ) << row
		 << " trim " << setw( 2 ) << trim
		 << " hits " << nn
		 << endl;

	    ++trim; // trim bits

	    modtrm[roc][col][row] = trim;
	    tb.roc_I2cAddr( roc );
	    if( trim > 15 ) {
	      tb.roc_Pix_Mask( col, row );
	      ++masked;
	      cout << "mask roc col row "
		   << setw(2) << roc
		   << setw(3) << col
		   << setw(3) << row
		   << endl;
	    }
	    else
	      tb.roc_Pix_Trim( col, row, trim );

	  } // noisy

	  else if( nn <= cut/10 && trim > 0 && hot[roc][col][row] == 0 ) {

	    again = 1;

	    --trim; // trim bits

	    modtrm[roc][col][row] = trim;
	    tb.roc_I2cAddr( roc );
	    tb.roc_Pix_Trim( col, row, trim );

	  } // quiet

	} // rows

    } // rocs

    tb.Flush();

    // plot trim bits:

    if( h11 )
      delete h11;
    h11 = new
      TH1D( Form( "mod_trim_bits_%02i", iter ),
	    Form( "Module trim bits distribution %02i;trim bits;pixels", iter ),
	    16, -0.5, 15.5 );
    h11->Sumw2();

    unsigned int nht = 0;
    for( int roc = 0; roc < 16; ++roc ) {
      if( roclist[roc] == 0 )
	continue;
      for( int col = 0; col < 52; ++col )
	for( int row = 0; row < 80; ++row ) {
	  h11->Fill( modtrm[roc][col][row] );
	  if( hot[roc][col][row] )
	    ++nht;
	}
    }
    h11->Write();
    h11->Draw( "hist" );
    c1->Update();

    cout << "masked pixels: " << masked << endl;

    cout << "hot " << nht << endl;

    cout << "iter " << iter << endl;

  }
  while( again && iter < 11 );

  if( again )
    cout << "reached iter limit: please run again to continue" << endl;

  // all off:

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Close( ch );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  cout << "  histos 11" << endl;

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modnoise

//------------------------------------------------------------------------------
CMD_PROC( phdac ) // phdac col row dac [stp] [nTrig] [roc] (PH vs dac)
{
  if( ierror ) return false;

  int col, row, dac;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers
  int stp;
  if( !PAR_IS_INT( stp, 1, 255 ) )
    stp = 1;
  if( dac > 30 && stp < 10 ) stp = 10; // VD, VA [mV]
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 10;
  int roc = 0;
  if( !PAR_IS_INT( roc, 0, 15 ) )
    roc = 0;

  if( dacval[roc][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  Log.section( "PHDAC", false );
  Log.printf( "ROC %i pixel %i %i DAC %i step %i ntrig %i \n",
	      roc, col, row, dac, stp, nTrig );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, stp, nTrig, nReadouts, PHavg, PHrms );

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int nstp = (dacstop - dacstrt)/stp + 1;

  if( h10 )
    delete h10;
  h10 = new
    TH1D( Form( "N_dac%02i_roc%02i_%02i_%02i", dac, roc, col, row ),
          Form( "responses vs %s ROC %i col %i row %i;%s [DAC];responses",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h10->Sumw2();

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "ph_dac%02i_roc%02i_%02i_%02i", dac, roc, col, row ),
          Form( "PH vs %s ROC %i col %i row %i;%s [DAC];<PH> [ADC]",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "vcal_dac%02i_roc%02i_%02i_%02i", dac, roc, col, row ),
          Form( dacval[0][CtrlReg] == 0 ?
                "Vcal vs %s ROC %i col %i row %i;%s [DAC];calibrated PH [small Vcal DAC]"
                :
                "Vcal vs %s ROC %i col %i row %i;%s [DAC];calibrated PH [large Vcal DAC]",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new
    TH1D( Form( "rms_dac%02i_roc%02i_%02i_%02i", dac, roc, col, row ),
          Form( "RMS vs %s ROC %i col %i row %i;%s [DAC];PH RMS [ADC]",
                dacName[dac].c_str(), row, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h13->Sumw2();

  // tb.CalibrateDacScan runs on the FPGA
  // kinks in PH vs DAC
  // update DAC correction table?
  /*
    bool slow = 0;
    if( dac != 25 ) { // 25 = Vcal is corrected on FPGA
    slow = 1;
    cout << "use slow PH with corrected DAC" << endl;
    int trim = 15;
    int nTrig = 10;
    int32_t dacstrt = dacStrt( dac );
    int32_t dacstop = dacStop( dac );
    nstp = dacstop - dacstrt + 1;
    PHavg.resize(nstp);
    nReadouts.resize(nstp);
    for( int32_t i = dacstrt; i <= dacstop; ++i ) {
    tb.SetDAC( dac, i ); // corrected, less kinky
    tb.mDelay(1);
    int ph = tb.PH( col, row, trim, nTrig ); // works on FW 1.15
    PHavg.at(i) = (ph > -0.5) ? ph : -1.0; // overwrite
    nReadouts.at(i) = nTrig;
    }
    }
  */
  // print:

  double phmin = 255;

  nstp = nReadouts.size();

  for( int32_t i = 0; i < nstp; ++i ) {

    int idac = dacStrt( dac ) + i * stp;

    double ph = PHavg.at( i );
    //cout << setw(4) << ((ph > -0.1 ) ? int(ph+0.5) : -1) << "(" << setw(3) << nReadouts.at(i) << ")";
    Log.printf( " %i", ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 );
    if( ph > -0.5 && ph < phmin )
      phmin = ph;
    h10->Fill( idac, nReadouts.at( i ) );
    if( nReadouts.at( i ) > 0 ) {
      h11->Fill( idac, ph );
      h13->Fill( idac, PHrms.at( i ) );
      double vc = PHtoVcal( ph, 0, col, row );
      h12->Fill( idac, vc );
      //cout << setw(3) << idac << "  " << ph << "  " << vc << endl;
    }
  } // dacs
  cout << endl;
  Log.printf( "\n" );
  cout << "min PH " << phmin << endl;
  Log.flush();

  h10->Write();
  h11->Write();
  h12->Write();
  h13->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 10, 11, 12, 13" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // phdac

//------------------------------------------------------------------------------
CMD_PROC( calsdac ) // calsdac col row dac (cals PH vs dac)
{
  int col, row, dac;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 10;
  int roc;
  if( !PAR_IS_INT( roc, 0, 15 ) )
    roc = 0;

  if( dacval[0][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  int stp = 1;

  Log.section( "CALSDAC", false );
  Log.printf( "pixel %i %i DAC %i\n", col, row, dac );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  tb.SetDAC( CtrlReg, 4 ); // want large Vcal

  DacScanPix( roc, col, row, dac, stp, -nTrig, nReadouts, PHavg, PHrms ); // negative nTrig = cals

  tb.SetDAC( CtrlReg, dacval[0][CtrlReg] ); // restore
  tb.Flush();

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int nstp = dacstop - dacstrt + 1;

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "cals_dac%02i_%02i_%02i_%02i", dac, roc, col, row ),
          Form( "CALS vs %s roc %i col %i row %i;%s [DAC];<CALS> [ADC]",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "resp_cals_dac%02i_%02i_%02i_%02i", dac, roc, col, row ),
          Form( "CALS responses vs %s roc %i col %i row %i;%s [DAC];responses",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h12->Sumw2();

  // print:

  double phmin = 255;

  nstp = nReadouts.size();

  for( int32_t i = 0; i < nstp; ++i ) {

    int idac = dacStrt( dac ) + i * stp;

    double ph = PHavg.at( i );
    cout << setw( 4 ) << ( ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 )
	 <<"(" << setw( 3 ) << nReadouts.at( i ) << ")";
    Log.printf( " %i", ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 );
    if( ph > -0.5 && ph < phmin )
      phmin = ph;
    h11->Fill( idac, ph );
    h12->Fill( idac, nReadouts.at( i ) );

  } // dacs
  cout << endl;
  Log.printf( "\n" );
  cout << "min PH " << phmin << endl;
  Log.flush();

  h11->Write();
  h12->Write();
  h12->SetStats( 0 );
  h12->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 12" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // calsdac

//------------------------------------------------------------------------------
int effvsdac( int col, int row, int dac, int stp, int nTrig, int roc )
{
  Log.section( "EFFDAC", false );
  Log.printf( "ROC %i pixel %i %i DAC %i step %i ntrig %i \n",
	      roc, col, row, dac, stp, nTrig );
  cout << endl;
  cout << "  [EFFDAC] pixel " << col << " " << row
       << " vs DAC " << dac << " step " << stp
       << ", nTrig " << nTrig
       << " for ROC " << roc
       << endl;
  cout << "  CalDel   " << dacval[0][CalDel] << endl;
  cout << "  VthrComp " << dacval[0][VthrComp] << endl;
  cout << "  CtrlReg  " << dacval[0][CtrlReg] << endl;
  cout << "  Vcal     " << dacval[0][Vcal] << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, stp, nTrig, nReadouts, PHavg, PHrms );

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int nstp = (dacstop - dacstrt)/stp + 1;

  cout << nstp << " steps from " << dacStrt(dac)
       << " to " << dacStop(dac) << endl;

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "eff_dac%02i_roc%02i_col%02i_row%02i_stp%02i",
		dac, roc, col, row, stp ),
          Form( "Responses vs %s ROC %i col %i row %i;%s [DAC] step %i;responses",
                dacName[dac].c_str(), roc, col, row, dacName[dac].c_str(), stp ),
	  nstp, dacStrt(dac)-0.5*stp, dacStop(dac) + 0.5*stp );
  h11->Sumw2();

  // print and plot:

  int i10 = 0;
  int i50 = 0;
  int i90 = 0;

  nstp = nReadouts.size();

  for( int32_t i = 0; i < nstp; ++i ) {

    int cnt = nReadouts.at( i );
    cout << setw( 4 ) << cnt;
    int idac = dacStrt( dac ) + i * stp;
    Log.printf( "%i %i\n", idac, cnt );
    h11->Fill( idac, cnt );

    if( cnt <= 0.1 * nTrig )
      i10 = idac; // thr - 1.28155 * sigma
    if( cnt <= 0.5 * nTrig )
      i50 = idac; // thr
    if( cnt <= 0.9 * nTrig )
      i90 = idac; // thr + 1.28155 * sigma

  } // dacs
  cout << endl;
  Log.printf( "\n" );
  Log.flush();

  if( dac == Vcal )
    cout << "  thr " << i50 << ", sigma " << (i90 - i10)/2.5631*stp << endl;

  h11->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return i50;

} // effvsdac

//------------------------------------------------------------------------------
CMD_PROC( effdac ) // effdac col row dac [stp] [nTrig] [roc] (efficiency vs dac)
{
  int col, row, dac;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers
  int stp;
  if( !PAR_IS_INT( stp, 1, 255 ) )
    stp = 1;
  if( dac > 30 && stp < 10 ) stp = 10; // VD, VA [mV]
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 100;
  int roc = 0;
  if( !PAR_IS_INT( roc, 0, 15 ) )
    roc = 0;

  if( dacval[roc][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  effvsdac( col, row, dac, stp, nTrig, roc );

  return true;

} // effdac

//------------------------------------------------------------------------------
CMD_PROC( thrdac ) // thrdac col row dac (thr vs dac)
{
  int col, row, dac;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers

  if( dacval[0][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  Log.section( "THRDAC", false );
  Log.printf( "pixel %i %i DAC %i\n", col, row, dac );

  const int vdac = dacval[0][dac];

  tb.SetDAC( CtrlReg, 0 ); // small Vcal

  int nTrig = 20;
  int step = 1;
  int trim = modtrm[0][col][row];

  // scan dac:

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int32_t dacstep = dacStep( dac );
  int nstp = ( dacstop - dacstrt ) / dacstep + 1;

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "thr_dac%02i_%02i_%02i_bits%02i", dac, col, row, trim ),
          Form
          ( "thr vs %s col %i row %i bits %i;%s [DAC];threshold [small Vcal DAC]",
            dacName[dac].c_str(), col, row, trim, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*dacstep, dacStop(dac) + 0.5*dacstep );
  h11->Sumw2();

  tb.roc_Col_Enable( col, true );
  tb.roc_Pix_Trim( col, row, trim );

  tb.Flush();

  int tmin = 255;
  int imin = 0;

  for( int32_t i = dacstrt; i <= dacstop; i += dacstep ) {

    tb.SetDAC( dac, i );
    tb.uDelay( 1000 );
    tb.Flush();

    int thr = ThrPix( 0, col, row, Vcal, step, nTrig );
    /* before FW 4.4
       int thr = tb.PixelThreshold( col, row,
       start, step,
       thrLevel, nTrig,
       Vcal, xtalk, cals );
       //PixelThreshold ends with dclose
    */
    h11->Fill( i, thr );
    cout << " " << thr << flush;
    Log.printf( "%i %i\n", i, thr );

    if( thr < tmin ) {
      tmin = thr;
      imin = i;
    }

  } // dacs

  cout << endl;
  cout << "small Vcal [DAC]" << endl;
  cout << "min Vcal threshold " << tmin << " at dac " << dac << " " << imin <<
    endl;

  h11->Write();
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  tb.roc_Col_Enable( col, 0 );
  tb.roc_ClrCal();
  tb.roc_Pix_Mask( col, row );
  tb.SetDAC( dac, vdac ); // restore dac value
  tb.SetDAC( CtrlReg, dacval[0][CtrlReg] );

  tb.Flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // thrdac

//------------------------------------------------------------------------------
CMD_PROC( modthrdac ) // modthrdac col row dac (thr vs dac on module)
{
  int roc = 0;
  int col, row, dac;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  PAR_INT( dac, 1, 32 ); // only DACs, not registers

  if( dacval[roc][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  Log.section( "MODTHRDAC", false );
  Log.printf( "pixel %i %i DAC %i\n", col, row, dac );

  const int vdac = dacval[roc][dac];

  tb.roc_I2cAddr( roc );
  tb.SetDAC( CtrlReg, 0 ); // small Vcal
  tb.Flush();

  int nTrig = 20; // for S-curve
  int step = 1;
  //bool xtlk = 0;
  //bool cals = 0;

  // scan dac:

  int32_t dacstrt = dacStrt( dac );
  int32_t dacstop = dacStop( dac );
  int32_t dacstep = dacStep( dac );
  int nstp = ( dacstop - dacstrt ) / dacstep + 1;
  int trim = modtrm[roc][col][row];

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "thr_dac%02i_%02i_%02i_bits%02i", dac, col, row, trim ),
          Form
          ( "thr vs %s col %i row %i bits %i;%s [DAC];threshold [small Vcal DAC]",
            dacName[dac].c_str(), col, row, trim, dacName[dac].c_str() ),
          nstp, dacStrt(dac)-0.5*dacstep, dacStop(dac) + 0.5*dacstep );
  h11->Sumw2();

  int tmin = 255;
  int imin = 0;

  for( int32_t i = dacstrt; i <= dacstop; i += dacstep ) {

    tb.SetDAC( dac, i );
    tb.uDelay( 1000 );
    tb.Flush();

    int thr = ThrPix( roc, col, row, Vcal, step, nTrig );

    h11->Fill( i, thr );
    cout << " " << thr << flush;
    Log.printf( "%i %i\n", i, thr );

    if( thr < tmin ) {
      tmin = thr;
      imin = i;
    }

  } // dacs

  cout << endl;
  cout << "small Vcal [DAC]" << endl;
  cout << "min Vcal threshold " << tmin << " at dac " << dac << " " << imin <<
    endl;

  h11->Write();
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  tb.SetDAC( dac, vdac ); // restore dac value
  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] );

  tb.Flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modthrdac

//------------------------------------------------------------------------------
// utility function for ROC 0 PH and eff map
bool GetRocData( int nTrig, vector < int16_t > &nReadouts,
                 vector < double > &PHavg, vector < double > &PHrms )
{
  uint16_t flags = 0;
  if( nTrig < 0 )
    flags = 0x0002; // FLAG_USE_CALS

  flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  uint16_t mTrig = abs( nTrig );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );

  // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 1 );
    /*
      for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim ); // done on FPGA in TriggerLoops
      }
    */
  }
  tb.uDelay( 1000 );
  tb.Flush();

  cout << "  GetRocData mTrig " << mTrig << ", flags " << flags << endl;

  try {
    tb.LoopSingleRocAllPixelsCalibrate( 0, mTrig, flags );
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  tb.Daq_Stop();

  // header = 1 word
  // pixel = +2 words
  // size = 4160 * nTrig * 3 = 124'800 words

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize() );
  cout << "  DAQ size " << tb.Daq_GetSize() << endl;

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest ); // 32768 gives zero
    cout << "  data size " << data.size()
	 << ", remaining " << rest << endl;
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(), dataB.begin(), dataB.end() );
      cout << "  data size " << data.size()
	   << ", remaining " << rest << endl;
      dataB.clear();
    }
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  // unpack data:

  PixelReadoutData pix;

  int pos = 0;
  int err = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

      int cnt = 0;
      int phsum = 0;
      int phsu2 = 0;

      for( int k = 0; k < mTrig; ++k ) {

        err = DecodePixel( data, pos, pix ); // analyzer

        if( err )
          cout << "  error " << err << " at trig " << k
	       << ", pix " << col << " " << row << ", pos " << pos << endl;
        if( err )
          break;

        if( pix.n > 0 )
          if( pix.x == col && pix.y == row ) {
            ++cnt;
            phsum += pix.p;
            phsu2 += pix.p * pix.p;
          }
      } // trig

      if( err )
        break;

      nReadouts.push_back( cnt ); // 0 = no response
      double ph = -1.0;
      double rms = -0.1;
      if( cnt > 0 ) {
        ph = ( double ) phsum / cnt;
        rms = sqrt( ( double ) phsu2 / cnt - ph * ph );
      }
      PHavg.push_back( ph );
      PHrms.push_back( rms );

    } // row

    if( err )
      break;

  } // col

  return 1;

} // getrocdata

//------------------------------------------------------------------------------
bool geteffmap( int nTrig )
{
  const int vctl = dacval[0][CtrlReg];
  const int vcal = dacval[0][Vcal];

  Log.section( "EFFMAP", false );
  Log.printf( "nTrig %i Vcal %i CtrlReg %i\n", nTrig, vcal, vctl );
  cout << endl;
  cout << "  [EFFMAP] with " << nTrig << " triggers"
       << " at Vcal " << vcal << ", CtrlReg " << vctl << endl;
  
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // measure:

  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  GetRocData( nTrig, nReadouts, PHavg, PHrms );

  gettimeofday( &tv, NULL );
  long s2 = tv.tv_sec; // seconds since 1.1.1970
  long u2 = tv.tv_usec; // microseconds
  double dt = s2 - s0 + ( u2 - u0 ) * 1e-6;

  cout << "LoopSingleRocAllPixelsCalibrate takes " << dt << " s"
       << " = " << dt / 4160 / nTrig * 1e6 << " us / pix" << endl;

  if( h11 )
    delete h11;
  h11 = new TH1D( Form( "Responses_Vcal%i_CR%i", vcal, vctl ),
                  Form( "Responses at Vcal %i, CtrlReg %i;responses;pixels",
                        vcal, vctl ), nTrig + 1, -0.5, nTrig + 0.5 );
  h11->Sumw2();

  if( h21 )
    delete h21;
  h21 = new TH2D( Form( "AliveMap_Vcal%i_CR%i", vcal, vctl ),
                  Form( "Alive map at Vcal %i, CtrlReg %i;col;row;responses",
                        vcal, vctl ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

  int nok = 0;
  int nActive = 0;
  int nPerfect = 0;

  size_t j = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

      int cnt = nReadouts.at( j );

      h11->Fill( cnt );
      h21->Fill( col, row, cnt );
      if( cnt > 0 )
        ++nok;
      if( cnt >= nTrig / 2 )
        ++nActive;
      if( cnt == nTrig )
        ++nPerfect;
      if( cnt < nTrig )
        cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row
	     << " responses " << cnt << endl;
      Log.printf( " %i", cnt );

      ++j;
      if( j == nReadouts.size() )
        break;

    } // row

    Log.printf( "\n" );
    if( j == nReadouts.size() )
      break;

  } // col

  h11->Write();
  h21->Write();
  h21->SetStats( 0 );
  h21->SetMinimum( 0 );
  h21->SetMaximum( nTrig );
  h21->GetYaxis()->SetTitleOffset( 1.3 );
  h21->Draw( "colz" );
  c1->Update();
  cout << "  histos 11, 21" << endl;

  cout << endl;
  cout << " >0% pixels: " << nok << endl;
  cout << ">50% pixels: " << nActive << endl;
  cout << "100% pixels: " << nPerfect << endl;

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // geteffmap

//------------------------------------------------------------------------------
CMD_PROC( effmap ) // single ROC response map ('PixelAlive')
{
  if( ierror ) return false;

  int nTrig; // DAQ size 1'248'000 at nTrig = 100 if fully efficient
  PAR_INT( nTrig, 1, 65000 );

  geteffmap( nTrig );

  return 1;

} // effmap

//------------------------------------------------------------------------------
// CS 2014: utility function for module response map

void ModPixelAlive( int nTrig )
{
  for( int roc = 0; roc < 16; ++roc )
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	modcnt[roc][col][row] = 0;
	modamp[roc][col][row] = 0;	
      }

  // all on:

  vector < uint8_t > rocAddress;

  for( size_t roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    rocAddress.push_back( roc );
    vector < uint8_t > trimvalues( 4160 );
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        int trim = modtrm[roc][col][row];
        int i = 80 * col + row;
        trimvalues[i] = trim;
      } // row
    tb.SetTrimValues( roc, trimvalues ); // load into FPGA
  } // roc
  tb.uDelay( 1000 );
  tb.Flush();

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Open( Blocksize, ch );
#endif
  tb.Daq_Select_Deser400();
  tb.uDelay( 100 );
  tb.Daq_Deser400_Reset( 3 );
  tb.uDelay( 100 );

  uint16_t flags = 0;           // or flags = FLAG_USE_CALS;

  //flags |= 0x0100; // FLAG_FORCE_UNMASKED: noisy

  vector <uint16_t > data[8];
  // 28 = 2 TBM head + 8 ROC head + 8*2 pix + 2 TBM trail
  for( int ch = 0; ch < nChannels; ++ch )
    data[ch].reserve( nTrig * 4160 * 28 );

  // measure:

  bool done = 0;
  double dtloop = 0;
  double dtread = 0;

  timeval tv;

  while( done == 0 ) { // should be per channel

    cout << "        loop..." << endl << flush;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec;        // seconds since 1.1.1970
    long u1 = tv.tv_usec;       // microseconds

    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Start( ch );
    tb.uDelay( 100 );

    try {
      done = tb.LoopMultiRocAllPixelsCalibrate( rocAddress, nTrig, flags );
    }
    catch( CRpcError & e ) {
      e.What();
      return;
    }

    cout << "        " << ( done ? "done" : "not done" ) << endl;

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec;        // seconds since 1.1.1970
    long u2 = tv.tv_usec;       // microseconds
    dtloop += s2 - s1 + ( u2 - u1 ) * 1e-6;

    // read TBM channels:

    for( int ch = 0; ch < nChannels; ++ch ) {

      cout << "        DAQ size channel " << ch
	   << " = " << tb.Daq_GetSize( ch ) << " words " << endl;

      //cout << "DAQ_Stop..." << endl;
      tb.Daq_Stop( ch );
      tb.uDelay( 100 );

      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );

      try {
        uint32_t rest;
        tb.Daq_Read( dataB, Blocksize, rest, ch );
        data[ch].insert( data[ch].end(), dataB.begin(),
                            dataB.end() );
        cout << "        data[" << ch << "] size " << data[ch].size()
	     << ", remaining " << rest << endl;
        while( rest > 0 ) {
          dataB.clear();
          tb.uDelay( 5000 );
          tb.Daq_Read( dataB, Blocksize, rest, ch );
          data[ch].insert( data[ch].end(), dataB.begin(),
                              dataB.end() );
          cout << "        data[" << ch << "] size " << data[ch].size()
	       << ", remaining " << rest << endl;
        }
      }
      catch( CRpcError & e ) {
        e.What();
        return;
      }

    } // ch

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec;        // seconds since 1.1.1970
    long u3 = tv.tv_usec;       // microseconds
    dtread += s3 - s2 + ( u3 - u2 ) * 1e-6;

  } // while not done

  cout << "        LoopMultiRocAllPixelsCalibrate takes "
       << dtloop << " s" << endl;
  cout << "        Daq_Read takes " << dtread << " s" << " = "
       << 2 * ( data[0].size() + data[1].size() ) / dtread / 1024 / 1024
       << " MiB/s" << endl;

  // read and analyze:

  int PX[16] = { 0 };
  int event = 0;

  //try to match pixel address
  // loop cols
  //   loop rows
  //     activate this pix on all ROCs
  //     loop dacs
  //       loop trig

  // TBM channels:

  bool ldb = 0;
  long countLoop = -1;    
  for( int ch = 0; ch < nChannels; ++ch ) {
    countLoop = -1;    
    cout << "        DAQ size channel " << ch
	 << " = " << data[ch].size() << " words " << endl;

    uint32_t raw = 0;
    uint32_t hdr = 0;
    uint32_t trl = 0;
    int32_t kroc = (16/nChannels) * ch - 1; // will start at 0 or 8

    // nDAC * nTrig * (TBM header, some ROC headers, one pixel, more ROC headers, TBM trailer)

    for( size_t i = 0; i < data[ch].size(); ++i ) {

      int d = data[ch].at( i ) & 0xfff; // 12 bits data
      int v = ( data[ch].at( i ) >> 12 ) & 0xe; // 3 flag bits
      
      uint32_t ph = 0;
      int c = 0;
      int r = 0;
      int x = 0;
      int y = 0;
      bool addressmatch = true;
      switch ( v ) {

	// TBM header:
      case 10:
        hdr = d;
        break;
      case 8:
        hdr = ( hdr << 8 ) + d;
	//DecodeTbmHeader(hdr);
	if( ldb )
	  cout << "TBM header" << endl;
        kroc = (16/nChannels) * ch-1; // new event, will start at 0 or 8
	if( L1 ) kroc = ch2roc[ch] - 1;
	++countLoop;
	if( ldb )
          cout << "event " << setw( 6 ) << event << " countloop " << countLoop << endl;

        break;

	// ROC header data:
      case 4:
	++kroc;
	if( ldb )
	  cout << "ROC header" << endl;
        if( ldb ) {
          if( kroc >= 0 )
          cout << "ROC " << setw( 2 ) << kroc << endl;
        }
        if( kroc > 15 ) {
          cout << "Error kroc " << kroc << endl;
          kroc = 15;
        }

        break;

	// pixel data:
      case 0:
        raw = d;
        break;
      case 2:
        raw = ( raw << 12 ) + d;
	//DecodePixel(raw);
        ph = ( raw & 0x0f ) + ( ( raw >> 1 ) & 0xf0 );
        raw >>= 9;
	if( Chip < 600 ) {
	  c = ( raw >> 12 ) & 7;
	  c = c * 6 + ( ( raw >> 9 ) & 7 );
	  r = ( raw >> 6 ) & 7;
	  r = r * 6 + ( ( raw >> 3 ) & 7 );
	  r = r * 6 + ( raw & 7 );
	  y = 80 - r / 2;
	  x = 2 * c + ( r & 1 );
	} // psi46dig
	else {
	  y = ((raw >>  0) & 0x07) + ((raw >> 1) & 0x78); // F7
	  x = ((raw >>  8) & 0x07) + ((raw >> 9) & 0x38); // 77
	} // proc600
	//        if( ldb )         
	{
	  long xi = (countLoop / nTrig ) ; 
	  int row = ( xi ) % 80;
	  int col = (xi - row) / 80;

	  if( col != x  or row != y ) {
	    addressmatch = false;
	    if( ldb ) {
	      cout << " pixel address mismatch " << endl;
	      cout << "from data " << " x " << x << " y " << y << " ph " << ph << endl;
	      cout << "from loop " << " col " << col << " row " << row << endl;
	    }
	  }
	} // dummy scope
	if( addressmatch ) {
          ++modcnt[kroc][x][y];
          modamp[kroc][x][y] += ph;
	  ++PX[kroc];
	} // dummy scope
        break;

	// TBM trailer:
      case 14:
        trl = d;
        break;
      case 12:
        trl = ( trl << 8 ) + d;
	//DecodeTbmTrailer(trl);
        if( ldb )
          cout << endl;
        if( ch == 0 )
          ++event;
	if( ldb )
	  cout << "TBM trailer" << endl;
        break;

      default:
        printf( "\nunknown data: %X = %d", v, v );
        break;

      } // switch

    } // data

    cout << "        TBM ch " << ch
	 << " events " << event
	 << " = " << event / nTrig << " pixels / ROC" << endl;

  } // ch

  // all off:

  for( size_t roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    tb.roc_I2cAddr( roc );
    for( int col = 0; col < 52; col += 2 ) // DC
      tb.roc_Col_Enable( col, 0 );
    tb.roc_Chip_Mask();
    tb.roc_ClrCal();
  }
  tb.Flush();

#ifdef DAQOPENCLOSE
  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Close( ch );
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  Log.flush();

  // print:

  for ( size_t roc = 0; roc < 16; ++roc )
    cout << "ROC " << setw( 2 ) << roc << ": hits " << PX[roc] << endl;

} // ModPixelAlive

set < int > sick; // Global

//------------------------------------------------------------------------------
bool tunePHmod( int col, int row, int roc )
{
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // scan Vcal for one pixel

  int dac = Vcal;
  int16_t nTrig = 9;

  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, 1, nTrig, nReadouts, PHavg, PHrms );

  if( nReadouts.size() < 256 ) {
    cout << "  only " << nReadouts.size() << " Vcal points"
	 << ". choose different pixel, check CalDel, check Ia, or give up"
	 << endl;
    return 0;
  }

  if( nReadouts.at( nReadouts.size() - 1 ) < nTrig ) {
    cout << "  only " << nReadouts.at( nReadouts.size() - 1 )
	 << " responses at " << nReadouts.size() - 1
	 << ". choose different pixel, check CalDel, check Ia, or give up" <<
      endl;
    return 0;
  }

  // scan from end, search for smallest responding Vcal:

  int minVcal = 255;

  for( int idac = nReadouts.size() - 1; idac >= 0; --idac )
    if( nReadouts.at( idac ) == nTrig )
      minVcal = idac;
  cout << "  min responding Vcal " << minVcal << endl;
  minVcal += 1; // safety

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // offset and gain DACs:

  int offsdac = VoffsetOp;      // digV2, positive slope
  int gaindac = VIref_ADC;
  if( Chip >= 400 ) {
    offsdac = PHOffset; // digV2.1, positive slope
    gaindac = PHScale;
  }

  cout << "  start offset dac " << offsdac
       << " at " << dacval[roc][offsdac] << endl;
  cout << "  start gain   dac " << gaindac
       << " at " << dacval[roc][gaindac] << endl;

  // set gain to minimal (at DAC 255), to avoid overflow or underflow:

  tb.SetDAC( gaindac, 255 );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Scan PH vs offset at Vcal 255:

  tb.SetDAC( Vcal, 255 ); // max Vcal
  tb.Flush();
  vector < double > PHmax;
  PHmax.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();
  DacScanPix( roc, col, row, offsdac, 1, nTrig, nReadouts, PHmax, PHrms );

  // Scan PH vs offset at minVcal:

  tb.SetDAC( Vcal, minVcal );
  tb.Flush();
  vector < double > PHmin;
  PHmin.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();
  DacScanPix( roc, col, row, offsdac, 1, nTrig, nReadouts, PHmin, PHrms );

  // use offset to center PH at 132:

  int offs = 0;
  double phmid = 0;
  for( size_t idac = 0; idac < PHmin.size(); ++idac ) {
    double phmean = 0.5 * ( PHmin.at( idac ) + PHmax.at( idac ) );
    if( fabs( phmean - 132 ) < fabs( phmid - 132 ) ) {
      offs = idac;
      phmid = phmean;
    }
  }

  cout << "  mid PH " << phmid << " at offset " << offs << endl << endl;

  tb.SetDAC( offsdac, offs );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // scan PH vs gain at Vcal 255 and at minVcal

  tb.SetDAC( Vcal, 255 );
  tb.Flush();
  vector < double > PHtop;
  PHtop.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();

  DacScanPix( roc, col, row, gaindac, 1, nTrig, nReadouts, PHtop, PHrms );

  tb.SetDAC( Vcal, minVcal );
  tb.Flush();
  vector < double > PHbot;
  PHbot.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();

  DacScanPix( roc, col, row, gaindac, 1, nTrig, nReadouts, PHbot, PHrms );

  // set gain:

  int gain = PHtop.size() - 1;
  for( ; gain >= 1; --gain ) {
    if( PHtop.at( gain ) > 233 )
      break;
    if( PHbot.at( gain ) < 22 )
      break;
  }
  cout << "  top PH " << PHtop.at( gain )
       << " at gain " << gain
       << " for Vcal 255" << " for pixel " << col << " " << row << endl;
  cout << "  bot PH " << PHbot.at( gain )
       << " at gain " << gain
       << " for Vcal " << minVcal << " for pixel " << col << " " << row << endl
       << endl;

  tb.SetDAC( gaindac, gain );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check for all pixels:

  tb.SetDAC( Vcal, 255 );
  tb.Flush();

  bool again = 0;

  int colmax = 0;
  int rowmax = 0;
  double phmax = 0;

  do { // again

    ModPixelAlive( nTrig ); // fills modcnt and modamp

    phmax = 0;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

	if( sick.count( 4160*roc+80*col+row ) )
	  continue;

	if( modcnt[roc][col][row] < nTrig / 2 )
          cout << "    weak pixel " << col << " " << row << endl;

        else {
          double ph = (double) modamp[roc][col][row] / modcnt[roc][col][row];
	  if( ph > phmax ) {
	    phmax = ph;
	    colmax = col;
	    rowmax = row;
	  }
	}

      } // row

    } // col

    cout << "  max PH " << phmax << " at " << colmax << " " << rowmax << endl;

    if( phmax > 252 && gain < 254 ) {

      vector < int16_t > nReadouts; // size 0
      vector < double > PHavg;
      vector < double > PHrms;
      nReadouts.reserve( 256 ); // size 0, capacity 256
      PHavg.reserve( 256 );
      PHrms.reserve( 256 );

      DacScanPix( roc, colmax, rowmax, gaindac, 1, nTrig, nReadouts, PHavg, PHrms );

      int nstp = nReadouts.size();

      // analyze: negative PH slope in high range

      int idac = 0;

      for( idac = 0; idac < nstp; ++idac ) {

	if( nReadouts.at( idac ) < nTrig/2 ) continue;

	if( PHavg.at( idac ) < 248 ) break;

      }

      cout << "    PH " << PHavg.at( idac ) << " at " << dacName[gaindac]
	   << " " << gain << endl;

      if( idac > gain )
	gain = idac; // reduce gain
      else {
	gain += 2; // D4037 ROC 13
	offs -= 2;
	tb.SetDAC( offsdac, offs );
	cout << "    offs dac " << dacName[offsdac]
	     << " set to " << offs << endl;
      }
      tb.SetDAC( gaindac, gain );
      tb.Flush();
      cout << "    gain dac " << dacName[gaindac] << " set to " << gain << endl
	   << endl;

      again = 1;

    }
    else
      again = 0;

  } while( again );

  cout << "  no overflows at " << dacName[gaindac] << " = " << gain << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check all pixels for underflow at minVcal:

  cout << endl
       << "check at minVcal " << minVcal << endl;

  tb.SetDAC( Vcal, minVcal );
  tb.Flush();

  again = 0;

  int colmin = 0;
  int rowmin = 0;
  double phmin = 255;

  do { // again

    ModPixelAlive( nTrig ); // fills modcnt and modamp

    phmin = 255;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

	if( sick.count( 4160*roc+80*col+row ) )
	  continue;

        if( modcnt[roc][col][row] < nTrig / 2 )
          cout << "  pixel " << col << " " << row
	       << " below threshold at Vcal "
	       << minVcal << ". ignored"
	       << endl;

        else {
          double ph = 1.0*modamp[roc][col][row] / modcnt[roc][col][row];
          if( ph < phmin ) {
            phmin = ph;
            colmin = col;
            rowmin = row;
          }
        }

      } // row

    } // col

    cout << "  min PH " << phmin << " at " << colmin << " " << rowmin << endl;

    if( phmin < 3 && gain > 0 ) {

      vector < int16_t > nReadouts; // size 0
      vector < double > PHavg;
      vector < double > PHrms;
      nReadouts.reserve( 256 ); // size 0, capacity 256
      PHavg.reserve( 256 );
      PHrms.reserve( 256 );

      DacScanPix( roc, colmin, rowmin, gaindac, 1, nTrig, nReadouts, PHavg, PHrms );

      int nstp = nReadouts.size();

      // analyze: positive PH slope in low range

      int idac = 0;

      for( idac = 0; idac < nstp; ++idac ) {

	if( nReadouts.at( idac ) < nTrig/2 ) continue;

	if( PHavg.at( idac ) > 3 ) break;

      }

      gain += 1; // safety
      tb.SetDAC( gaindac, gain );
      tb.Flush();
      cout << "  gain dac " << dacName[gaindac] << " set to " << gain << endl;

      again = 1;

    }
    else
      again = 0;

  } while( again );

  cout << "  no underflows at " << dacName[gaindac] << " = " << gain << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // we have min and max.
  // adjust gain and offset such that max-min = 200, mid = 132

  cout << endl;
  cout << "  max PH " << phmax << " at " << colmax << " " << rowmax << endl;
  cout << "  min PH " << phmin << " at " << colmin << " " << rowmin << endl;

  again = 0;
  int iter = 0;

  do {

    cout << endl << "  " << iter << endl;

    tb.SetDAC( Vcal, minVcal );
    tb.Flush();

    int cnt;
    double rms;
    double phmin = 255;
    GetPixData( roc, colmin, rowmin, nTrig, cnt, phmin, rms );

    tb.SetDAC( Vcal, 255 );
    tb.Flush();
    double phmax = 0;
    GetPixData( roc, colmax, rowmax, nTrig, cnt, phmax, rms );

    again = 0;

    double phmid = 0.5 * ( phmax + phmin );

    cout << "  PH mid " << phmid << endl;

    if( phmid > 132 + 13 && offs > 0 ) { // 13 is margin of convergence
      offs -= 1;
      again = 1;
    }
    else if( phmid < 132 - 13 && offs < 255 ) {
      offs += 1;
      again = 1;
    }
    if( again ) {
      tb.SetDAC( offsdac, offs );
      tb.Flush();
      cout << "    offs dac " << dacName[offsdac] << " set to " << offs << endl;
    }

    double range = phmax - phmin;

    cout << "  PH rng " << range << endl;

    if( range > 200 + 3 && gain < 255 ) { // 3 is margin of convergence
      gain += 1; // reduce gain
      again = 1;
    }
    else if( range < 200 - 3 && gain > 0 ) {
      gain -= 1; // more gain
      again = 1;
    }
    if( again ) {
      tb.SetDAC( gaindac, gain );
      tb.Flush();
      cout << "    gain dac " << dacName[gaindac] << " set to " << gain << endl;
    }
    ++iter;
  }
  while( again );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // done

  cout << "  final offs dac " << dacName[offsdac] << " set to " << offs << endl;
  cout << "  final gain dac " << dacName[gaindac] << " set to " << gain << endl;

  dacval[roc][offsdac] = offs;
  dacval[roc][gaindac] = gain;

  Log.printf( "[SETDAC] %i  %i\n", gaindac, gain );
  Log.printf( "[SETDAC] %i  %i\n", offsdac, offs );
  Log.flush();

  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.Flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // tunePHmod

//------------------------------------------------------------------------------
CMD_PROC( modtune ) // adjust PH gain and offset to fit into ADC range
{
  if( ierror ) return false;
  
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  int offsdac = VoffsetOp;      // digV2, positive slope
  int gaindac = VIref_ADC;
  if( Chip >= 400 ) {
    offsdac = PHOffset; // digV2.1, positive slope
    gaindac = PHScale;
  }

  sick.clear();

  // drive into overflow:

  for( int roc = 0; roc < 16; ++roc ) {
    tb.roc_I2cAddr( roc );
    tb.SetDAC( CtrlReg, 4 ); // large Vcal
    tb.SetDAC( Vcal, 255 ); // max Vcal
    tb.SetDAC( gaindac, 33 ); // max gain
    tb.SetDAC( offsdac, 255 ); // max offset
  }
  tb.Flush();

  int16_t nTrig = 9;
  ModPixelAlive( nTrig ); // fills modcnt and modamp

  for( int roc = 0; roc < 16; ++roc ) {

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        if( modcnt[roc][col][row] < nTrig / 2 ) {
          cout << "    weak pixel "
	       << roc << " "
	       << col << " "
	       << row << endl;
	  sick.insert( roc*4160+col*80+row );
        }
        else {
          double ph = (double) modamp[roc][col][row] / modcnt[roc][col][row];
	  if( ph < 250 ) {
	    cout << "    sick pixel "
		 << roc << " "
		 << col << " "
		 << row << " "
		 << ph
		 << endl;
	    sick.insert( roc*4160+col*80+row );
	  }
	}

      } // row

    } // col

  } // roc

  cout << "total " << sick.size() << " sick pixels" << endl;

  // drive into underflow:

  for( int roc = 0; roc < 16; ++roc ) {
    tb.roc_I2cAddr( roc );
    tb.SetDAC( Vcal, 11 ); // large Vcal just above threshold
    tb.SetDAC( gaindac, 33 ); // max gain
    tb.SetDAC( offsdac, 0 ); // min offset
  }
  tb.Flush();

  ModPixelAlive( nTrig ); // fills modcnt and modamp

  for( int roc = 0; roc < 16; ++roc ) {

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        if( modcnt[roc][col][row] < nTrig / 2 ) {
          cout << "    weak pixel "
	       << roc << " "
	       << col << " "
	       << row << endl;
	  sick.insert( roc*4160+col*80+row );
        }
        else {
          double ph = (double) modamp[roc][col][row] / modcnt[roc][col][row];
	  if( ph > 25 ) {
	    cout << "    sick pixel "
		 << roc << " "
		 << col << " "
		 << row << " "
		 << ph
		 << endl;
	    sick.insert( roc*4160+col*80+row );
	  }
	}

      } // row

    } // col

  } // roc

  cout << "total " << sick.size() << " sick pixels" << endl;

  for( int roc = 0; roc < 16; ++roc ) {
    tb.roc_I2cAddr( roc );
    tb.SetDAC( offsdac, dacval[roc][offsdac] ); // restore
    tb.SetDAC( gaindac, dacval[roc][gaindac] ); // restore
    tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  }
  tb.Flush();

  //- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( h11 )
    delete h11;
  h11 = new TH1D( "phoffset",
		  "PH offset DAC;ROC;PH offset [DAC]",
		  16, -0.5, 15.5 );

  if( h12 )
    delete h12;
  h12 = new TH1D( "phscale",
		  "PH scale DAC;ROC;PH scale [DAC]",
		  16, -0.5, 15.5 );

  // tune per ROC:

  for( int roc = 0; roc < 16; ++roc ) {

    cout << endl << "PH tuning for ROC " << roc << endl;

    tb.roc_I2cAddr( roc );

    // check that col row is not sick:

    bool again = 0;
    do {
      if( sick.count( 4160*roc+80*col+row ) ) {
	col += 2;
	row += 2;
	if( col > 51 ) col -= 52; // choose different seed pixel
	if( row > 79 ) row -= 80;
	again = 1;
      }
    }
    while( again );

    tunePHmod( col, row, roc );

    h11->Fill( roc, dacval[roc][offsdac] );
    h12->Fill( roc, dacval[roc][gaindac] );

  } // rocs

  h12->Draw( );
  c1->Update();

  h11->Write();
  h12->Write();
  cout << "  histos 11, 12" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // modtune

//------------------------------------------------------------------------------
bool ModMap( int nTrig ) // pixelAlive for modules
{
  Log.section( "MODMAP", false );
  Log.printf( "nTrig %i\n", nTrig );
  cout << endl << "modmap with " << nTrig << " triggers" << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec;          // seconds since 1.1.1970
  long u0 = tv.tv_usec;         // microseconds

  ModPixelAlive( nTrig ); // fills modcnt and modamp

  if( h11 )
    delete h11;
  h11 = new TH1D( Form( "Responses_Vcal%i_CR%i",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  Form( "Responses at Vcal %i, CtrlReg %i;responses;pixels",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  nTrig + 1, -0.5, nTrig + 0.5 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new TH1D( Form( "Mod_PH_spectrum_Vcal%i_CR%i",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  Form( "PH at Vcal %i, CtrlReg %i;PH [ADC];pixels",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  256, -0.5, 255.5 );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new TH1D( Form( "Mod_Vcal_spectrum_Vcal%i_CR%i",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  Form( "Vcal at Vcal %i, CtrlReg %i;PH [Vcal DAC];pixels",
                        dacval[0][Vcal], dacval[0][CtrlReg] ),
                  256, -0.5, 255.5 );
  h13->Sumw2();

  if( h21 )
    delete h21;
  h21 = new TH2D( "ModuleHitMap",
                  "Module response map;col;row;hits",
                  8*52, -0.5, 8*52 - 0.5, 2*80, -0.5, 2*80 - 0.5 );
  gStyle->SetOptStat( 10 ); // entries
  h21->GetYaxis()->SetTitleOffset( 1.3 );

  if( h22 )
    delete h22;
  h22 = new TProfile2D( "ModulePHmap",
			"Module PH map;col;row;<PH> [ADC]",
			8 * 52, -0.5, 8 * 52 - 0.5,
			2 * 80, -0.5, 2 * 80 - 0.5, -1, 256 );

  if( h23 )
    delete h23;
  h23 = new TProfile2D( "ModuleVcalMap",
			"Module Vcal map;col;row;<PH> [Vcal DAC]",
			8 * 52, -0.5, 8 * 52 - 0.5,
			2 * 80, -0.5, 2 * 80 - 0.5, -1, 256 );

  int ndead = 0;
  int nweak = 0;
  int ngood = 0;
  int nfull = 0;

  for( int roc = 0; roc < 16; ++roc ) {
    if( roclist[roc] == 0 )
      continue;
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	int l = roc % 8;   // 0..7
	int xm = 52 * l + col; // 0..415  rocs 0 1 2 3 4 5 6 7
	int ym = row; // 0..79
	if( roc > 7 ) {
	  xm = 415 - xm; // rocs 8 9 A B C D E F
	  ym = 159 - row; // 80..159
	}
	double ph = modamp[roc][col][row] / nTrig;
	double vc = PHtoVcal( ph , roc, col, row );
	int cnt = modcnt[roc][col][row];

	if( cnt == 0 ) {
	  ++ndead;
	  cout << "dead " << roc << " " << col << " " << row << endl;
	}
	else if( cnt < nTrig/2 ) {
	  ++nweak;
	  cout << "weak " << roc << " " << col << " " << row << endl;
	}
	else if( cnt == nTrig )
	  ++nfull;
	else
	  ++ngood;

	h13->Fill( vc );
	h11->Fill( cnt );
	h21->Fill( xm, ym, cnt );
	h12->Fill( ph );
	h22->Fill( xm, ym, ph );
	h23->Fill( xm, ym, vc );

      } // row

  } // rocs

  cout << "full " << nfull << endl;
  cout << "good " << ngood << endl;
  cout << "weak " << nweak << endl;
  cout << "dead " << ndead << endl;
  Log.printf( "  full %i\n", nfull );
  Log.printf( "  good %i\n", ngood );
  Log.printf( "  weak %i\n", nweak );
  Log.printf( "  dead %i\n", ndead );

  h11->Write();
  h12->Write();
  h13->Write();
  h21->Write();
  h22->Write();
  h23->Write();
  h21->SetMinimum( 0 );
  h21->SetMaximum( nTrig );
  h21->Draw( "colz" );
  c1->Update();
  cout << "histos 11, 12, 13, 21, 22, 23" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec;          // seconds since 1.1.1970
  long u9 = tv.tv_usec;         // microseconds
  cout << "test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // ModMap

//------------------------------------------------------------------------------
CMD_PROC( modmap ) // pixelAlive for modules
{
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 65500 ) )
    nTrig = 20;

  bool ok = ModMap( nTrig );
  
  return ok;

} // modmap

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: measure ROC threshold Vcal map

void RocThrMap( int roc, int step, int nTrig )
{
  if( roc < 0 ) {
    cout << "invalid roc " << roc << endl;
    return;
  }
  if( roc > 15 ) {
    cout << "invalid roc " << roc << endl;
    return;
  }

  tb.roc_I2cAddr( roc ); // just to be sure

  tb.Flush();

  for( int col = 0; col < 52; ++col ) {

    tb.roc_Col_Enable( col, true );
    cout << setw( 3 ) << col;
    cout.flush();

    for( int row = 0; row < 80; ++row ) {

      int trim = modtrm[roc][col][row];
      tb.roc_Pix_Trim( col, row, trim );

      int thr = ThrPix( 0, col, row, Vcal, step, nTrig );
      /* before FW 4.4
      int thr = tb.PixelThreshold( col, row,
      start, step,
      thrLevel, nTrig,
      Vcal, xtalk, cals );

      if( thr == 255 ) // try again
        thr = tb.PixelThreshold( col, row,
                                 start, step,
                                 thrLevel, nTrig, Vcal, xtalk, cals );
      */
      modthr[roc][col][row] = thr;

      tb.roc_Pix_Mask( col, row );

    } // rows

    tb.roc_Col_Enable( col, 0 );

  } // cols

  cout << endl;
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore

  tb.Flush();

} // rocthrmap

//------------------------------------------------------------------------------
CMD_PROC( thrmap ) // for single ROCs
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  Log.section( "THRMAP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int nTrig = 20;

  const int step = 1;

  int npx[16] = { 0 };

  const int vthr = dacval[0][VthrComp];
  const int vtrm = dacval[0][Vtrim];

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "thr_dist_Vthr%i_Vtrm%i", vthr, vtrm ),
	  Form( "Threshold distribution Vthr %i Vtrim %i;threshold [small Vcal DAC];pixels", vthr, vtrm ),
	  255, -0.5, 254.5 ); // 255 = overflow
  h11->Sumw2();

  if( h21 )
    delete h21;
  h21 = new TH2D( Form( "ThrMap_Vthr%i_Vtrm%i", vthr, vtrm ),
                  Form
                  ( "Threshold map at Vthr %i, Vtrim %i;col;row;threshold [small Vcal DAC]",
                    vthr, vtrm ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

  // loop over ROCs:

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    cout << setw( 2 ) << "ROC " << roc << endl;

    tb.roc_I2cAddr( roc );
    tb.SetDAC( CtrlReg, 0 ); // measure thresholds at ctl 0

    RocThrMap( roc, step, nTrig ); // fills modthr

    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    tb.Flush();

    int nok = 0;
    printThrMap( 1, roc, nok );
    npx[roc] = nok;

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        h11->Fill( modthr[roc][col][row] );
        h21->Fill( col, row, modthr[roc][col][row] );
      }
  } // rocs

  cout << endl;
  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] )
      cout << "ROC " << setw( 2 ) << roc << " valid thr " << npx[roc]
	   << endl;

  h11->Write();
  h21->Write();

  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 21" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // thrmap

//------------------------------------------------------------------------------
CMD_PROC( thrmapsc ) // raw data S-curve: 60 s / ROC
{
  int stop = 255;
  int ils = 0; // small Vcal

  if( !PAR_IS_INT( stop, 30, 255 ) ) {
    stop = 255;
    ils = 4; // large Vcal
  }

  int dac = Vcal;

  Log.section( "THRMAPSC", false );
  Log.printf( "stop at %i\n", stop );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int roc = 0;

  tb.SetDAC( CtrlReg, ils ); // measure at small or large Vcal

  int nTrig = 20;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );

  // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 1 );
    for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim );
    }
  }
  tb.uDelay( 1000 );
  tb.Flush();

  uint16_t flags = 0; // normal CAL
  if( ils )
    flags = 0x0002; // FLAG_USE_CALS;
  if( flags > 0 )
    cout << "CALS used..." << endl;

  if( dac == 11 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac == 12 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac == 25 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  int16_t dacLower1 = 0;
  int16_t dacUpper1 = stop;
  int32_t nstp = dacUpper1 - dacLower1 + 1;

  // measure:

  cout << "pulsing 4160 pixels with " << nTrig << " triggers for "
       << nstp << " DAC steps may take " << int ( 4160 * nTrig * nstp * 6e-6 ) +
    1 << " s..." << endl;

  // header = 1 word
  // pixel = +2 words
  // size = 256 dacs * 4160 pix * nTrig * 3 words = 32 MW

  vector < uint16_t > data;
  data.reserve( nstp * nTrig * 4160 * 3 );

  bool done = 0;
  double tloop = 0;
  double tread = 0;

  while( done == 0 ) {

    cout << "loop..." << endl << flush;

    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    tb.Daq_Start();
    tb.uDelay( 100 );

    try {
      done =
        tb.LoopSingleRocAllPixelsDacScan( roc, nTrig, flags, dac, dacLower1,
                                          dacUpper1 );
      // ~/psi/dtb/pixel-dtb-firmware/software/dtb_expert/trigger_loops.cpp
      // loop cols
      //   loop rows
      //     activate this pix on all ROCs
      //     loop dacs
      //       loop trig
    }
    catch( CRpcError & e ) {
      e.What();
      return 0;
    }

    int dSize = tb.Daq_GetSize();

    tb.Daq_Stop(); // avoid extra (noise) data

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
    double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;
    tloop += dt;
    cout << "LoopSingleRocAllPixelsDacScan takes " << dt << " s"
	 << " = " << tloop / 4160 / nTrig / nstp * 1e6 << " us / pix" << endl;
    cout << ( done ? "done" : "not done" ) << endl;

    cout << "DAQ size " << dSize << " words" << endl;

    vector < uint16_t > dataB;
    dataB.reserve( Blocksize );

    try {
      uint32_t rest;
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(), dataB.begin(), dataB.end() );
      cout << "data size " << data.size()
	   << ", remaining " << rest << endl;
      while( rest > 0 ) {
        dataB.clear();
	tb.uDelay( 5000 );
        tb.Daq_Read( dataB, Blocksize, rest );
        data.insert( data.end(), dataB.begin(), dataB.end() );
        cout << "data size " << data.size()
	     << ", remaining " << rest << endl;
      }
    }
    catch( CRpcError & e ) {
      e.What();
      return 0;
    }

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
    tread += dtr;
    cout << "Daq_Read takes " << tread << " s"
	 << " = " << 2 * dSize / tread / 1024 / 1024 << " MiB/s" << endl;

  } // while not done

  // all off:

  for( int col = 0; col < 52; ++col )
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] );
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "thr_dist",
          "Threshold distribution;threshold [small Vcal DAC];pixels",
          256, -0.5, 255.5 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "noisedist",
          "Noise distribution;width of threshold curve [small Vcal DAC];pixels",
          51, -0.5, 50.5 );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new
    TH1D( "maxdist",
          "Response distribution;responses;pixels",
          nTrig + 1, -0.5, nTrig + 0.5 );
  h13->Sumw2();

  // unpack data:

  PixelReadoutData pix;

  int pos = 0;
  int err = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

      //cout << endl << setw(2) << col << setw(3) << row << endl;

      int cnt[256] = { 0 };
      int nmx = 0;

      for( int32_t j = 0; j < nstp; ++j ) { // DAC steps

        uint32_t idc = dacLower1 + j;

        for( int k = 0; k < nTrig; ++k ) {

          err = DecodePixel( data, pos, pix ); // analyzer

          if( err )
            cout << "error " << err << " at trig " << k
		 << ", stp " << j
		 << ", pix " << col << " " << row << ", pos " << pos << endl;
          if( err )
            break;

          if( pix.n > 0 ) {
            if( pix.x == col && pix.y == row ) {
              ++cnt[idc];
            }
          }

        } // trig

        if( err )
          break;

        if( cnt[idc] > nmx )
          nmx = cnt[idc];

      } // dac

      if( err )
        break;

      // analyze S-curve:

      int i10 = 0;
      int i50 = 0;
      int i90 = 0;
      bool ldb = 0;
      if( col == 22 && row == 22 )
        ldb = 1; // one example pix
      for( int idc = 0; idc < nstp; ++idc ) {
        int ni = cnt[idc];
        if( ldb )
          cout << setw( 3 ) << ni;
        if( ni <= 0.1 * nmx )
          i10 = idc; // -1.28155 sigma
        if( ni <= 0.5 * nmx )
          i50 = idc;
        if( ni <= 0.9 * nmx )
          i90 = idc; // +1.28155 sigma
      }
      if( ldb )
        cout << endl;
      if( ldb )
        cout << "thr " << i50 << ", width " << (i90 - i10)/2.5631 << endl;
      modthr[roc][col][row] = i50;
      h11->Fill( i50 );
      h12->Fill( (i90 - i10)/2.5631 );
      h13->Fill( nmx );

    } // row

    if( err )
      break;

  } // col

  Log.flush();

  int nok = 0;
  printThrMap( 1, roc, nok );

  h11->Write();
  h12->Write();
  h13->Write();
  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 12, 13" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // thrmapsc

//------------------------------------------------------------------------------
// global:

int rocthr[52][80] = { {0} };
int roctrm[52][80] = { {0} };

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: trim bit correction

void TrimStep( int roc,
               int target, double correction,
               int step, int nTrig )
{
  cout << endl
       << "TrimStep for ROC " << roc
       << " with correction " << correction << endl;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80; ++row ) {

      int thr = modthr[roc][col][row];

      rocthr[col][row] = thr; // remember

      int trim = modtrm[roc][col][row]; // present trim bit value

      roctrm[col][row] = trim; // remember

      if( correction < 0 ) { // use slope dthr/bit
	trim -= (target-thr)/correction;
      }
      else {
	if( thr > target && thr < 255 )
	  trim -= correction; // make softer
	else
	  trim += correction; // make harder
      }

      if( trim < 0 )
        trim = 0;
      if( trim > 15 )
        trim = 15;

      modtrm[roc][col][row] = trim;

    } // rows

  } // cols

  // measure new result:

  RocThrMap( roc, step, nTrig ); // fills modthr

  int nok = 0;
  printThrMap( 0, roc, nok );

  for( int col = 0; col < 52; ++col ) {
    for( int row = 0; row < 80; ++row ) {

      int thr = modthr[roc][col][row];
      int old = rocthr[col][row];

      if( thr == 255 ) {
        modtrm[roc][col][row] = roctrm[col][row]; // go back
        modthr[roc][col][row] = old;
      }
      else
	if( abs( thr - target ) > abs( old - target ) ) {
	  modtrm[roc][col][row] = roctrm[col][row]; // go back
	  modthr[roc][col][row] = old;
	}

    } // rows

  } // cols

} // TrimStep

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 10.8.2014: set ROC global threshold using 5 pixels
CMD_PROC( vthrcomp5 )
{
  if( ierror ) return false;

  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  Log.section( "VTHRCOMP5", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int nTrig = 20;
  const int step = 1;

  size_t roc = 0;

  tb.roc_I2cAddr( roc );

  int vthr = dacval[roc][VthrComp];
  int vtrm = dacval[roc][Vtrim];

  cout << endl
       << setw( 2 ) << "ROC " << roc
       << ", VthrComp " << vthr << ", Vtrim " << vtrm << endl;

  tb.SetDAC( CtrlReg, 0 ); // this ROC, small Vcal

  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // look at a few pixels:

  const int ntst = 5;
  int coltst[ntst] = {  1,  2, 26, 49, 50 };
  int rowtst[ntst] = {  1, 78, 40,  1, 78 };

  int vmin = 255;
  int colmin = -1;
  int rowmin = -1;

  for( int itst = 0; itst < ntst; ++itst ) {

    int col = coltst[itst];
    int row = rowtst[itst];
    tb.roc_Col_Enable( col, true );
    int trim = modtrm[roc][col][row];
    tb.roc_Pix_Trim( col, row, trim );

    int thr = ThrPix( 0, col, row, Vcal, step, nTrig );
    /* before FW 4.4
       int thr = tb.PixelThreshold( col, row,
       guess, step,
       thrLevel, nTrig,
       Vcal, xtalk, cals );
    */
    tb.roc_Pix_Mask( col, row );
    tb.roc_Col_Enable( col, 0 );
    cout
      << setw(2) << col
      << setw(3) << row
      << setw(4) << thr
      << endl;

    if( thr < vmin ) {
      vmin = thr;
      colmin = col;
      rowmin = row;
    }

  } // tst

  cout << "min thr " << vmin << " at " << colmin << " " << rowmin << endl;
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore

  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // setVthrComp using min pixel:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "thr_vs_VthrComp_col%i_row%i", colmin, rowmin ),
	  Form
	  ( "Threshold vs VthrComp pix %i %i target %i;VthrComp [DAC];threshold [small Vcal DAC]",
	    colmin, rowmin, target ), 200, 0, 200 );
  h11->Sumw2();

  // VthrComp has negative slope: higher = softer

  int vstp = -1; // towards harder threshold
  if( vmin > target )
    vstp = 1; // towards softer threshold

  tb.roc_Col_Enable( colmin, true );
  int tbits = modtrm[roc][colmin][rowmin];
  tb.roc_Pix_Trim( colmin, rowmin, tbits );

  tb.Flush();

  for( ; vthr < 255 && vthr > 0; vthr += vstp ) {

    tb.SetDAC( VthrComp, vthr );
    tb.uDelay( 1000 );
    tb.Flush();

    int thr = ThrPix( 0, colmin, rowmin, Vcal, step, nTrig );
    /* before FW 4.4
       int thr =
       tb.PixelThreshold( colmin, rowmin,
       guess, step,
       thrLevel, nTrig,
       Vcal, xtalk, cals );
    */
    h11->Fill( vthr, thr );
    cout << setw( 3 ) << vthr << setw( 4 ) << thr << endl;

    if( vstp * thr <= vstp * target ) // signed
      break;

  } // vthr

  h11->Draw( "hist" );
  c1->Update();
  h11->Write();

  vthr -= 1; // margin towards harder threshold
  tb.SetDAC( VthrComp, vthr );
  cout << "set VthrComp to " << vthr << endl;
  dacval[roc][VthrComp] = vthr;

  tb.roc_Pix_Mask( colmin, rowmin );
  tb.roc_Col_Enable( colmin, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.Flush();
  cout << "roc " << roc << " back to Vcal " << dacval[roc][Vcal]
       << ", CtrlReg " << dacval[roc][CtrlReg]
       << endl;

  cout << "  histos 10,11" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // vthrcomp5

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 2.6.2014: set ROC global threshold
CMD_PROC( vthrcomp )
{
  if( ierror ) return false;

  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  Log.section( "VTHRCOMP", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int nTrig = 20;
  const int step = 1;

  size_t roc = 0;

  tb.roc_I2cAddr( roc );

  int vthr = dacval[roc][VthrComp];
  int vtrm = dacval[roc][Vtrim];

  cout << endl
       << setw( 2 ) << "ROC " << roc
       << ", VthrComp " << vthr << ", Vtrim " << vtrm << endl;

  tb.SetDAC( CtrlReg, 0 ); // this ROC, small Vcal

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // take threshold map, find softest pixel

  RocThrMap( roc, step, nTrig ); // fills modthr

  int nok = 0;
  printThrMap( 0, roc, nok );

  if( h10 )
    delete h10;
  h10 = new
    TH1D( Form( "thr_dist_Vthr%i_Vtrm%i", vthr, vtrm ),
	  Form( "Threshold distribution Vthr %i Vtrim %i;threshold [small Vcal DAC];pixels", vthr, vtrm ),
	  255, -0.5, 254.5 ); // 255 = overflow
  h10->Sumw2();

  for( int col = 0; col < 52; ++col )
    for( int row = 0; row < 80; ++row )
      h10->Fill( modthr[roc][col][row] );

  h10->Draw( "hist" );
  c1->Update();
  h10->Write();

  int vmin = 255;
  int colmin = -1;
  int rowmin = -1;

  for( int row = 0; row < 80; ++row )
    for( int col = 0; col < 52; ++col ) {

      int thr = modthr[roc][col][row];
      if( thr < vmin ) {
	vmin = thr;
	colmin = col;
	rowmin = row;
      }
    } // cols

  cout << "min thr " << vmin << " at " << colmin << " " << rowmin << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // setVthrComp using min pixel:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "thr_vs_VthrComp_col%i_row%i", colmin, rowmin ),
	  Form
	  ( "Threshold vs VthrComp pix %i %i target %i;VthrComp [DAC];threshold [small Vcal DAC]",
	    colmin, rowmin, target ), 200, 0, 200 );
  h11->Sumw2();

  // VthrComp has negative slope: higher = softer

  int vstp = -1; // towards harder threshold
  if( vmin > target )
    vstp = 1; // towards softer threshold

  tb.roc_Col_Enable( colmin, true );
  int tbits = modtrm[roc][colmin][rowmin];
  tb.roc_Pix_Trim( colmin, rowmin, tbits );

  tb.Flush();

  for( ; vthr < 255 && vthr > 0; vthr += vstp ) {

    tb.SetDAC( VthrComp, vthr );
    tb.uDelay( 1000 );
    tb.Flush();

    int thr = ThrPix( 0, colmin, rowmin, Vcal, step, nTrig );
    /* before FW 4.4
       int thr =
       tb.PixelThreshold( colmin, rowmin,
       guess, step,
       thrLevel, nTrig,
       Vcal, xtalk, cals );
    */
    h11->Fill( vthr, thr );
    cout << setw( 3 ) << vthr << setw( 4 ) << thr << endl;

    if( vstp * thr <= vstp * target ) // signed
      break;

  } // vthr

  h11->Draw( "hist" );
  c1->Update();
  h11->Write();

  vthr -= 1; // margin towards harder threshold
  tb.SetDAC( VthrComp, vthr );
  cout << "set VthrComp to " << vthr << endl;
  dacval[roc][VthrComp] = vthr;

  tb.roc_Pix_Mask( colmin, rowmin );
  tb.roc_Col_Enable( colmin, 0 );
  tb.roc_ClrCal();
  tb.Flush();

  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.Flush();
  cout << "roc " << roc << " back to Vcal " << dacval[roc][Vcal]
       << ", CtrlReg " << dacval[roc][CtrlReg]
       << endl;

  cout << "  histos 10,11" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // vthrcomp

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 25.1.2014: trim procedure, 83 s / ROC
CMD_PROC( trim )
{
  if( ierror ) return false;

  int target;
  PAR_INT( target, 0, 255 ); // Vcal [DAC] threshold target

  Log.section( "TRIM", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int nTrig = 20;
  const int step = 1;

  size_t roc = 0;

  tb.roc_I2cAddr( roc );

  const int vthr = dacval[roc][VthrComp];
  int vtrm = dacval[roc][Vtrim];

  cout << endl
       << setw( 2 ) << "ROC " << roc << ", VthrComp " << vthr << endl;

  tb.SetDAC( CtrlReg, 0 ); // this ROC, small Vcal

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // take threshold map, find hardest pixel

  int tbits = 15; // 15 = off

  for( int row = 0; row < 80; ++row )
    for( int col = 0; col < 52; ++col )
      modtrm[roc][col][row] = tbits;

  RocThrMap( roc, step, nTrig ); // fills modthr

  int nok = 0;
  printThrMap( 0, roc, nok );

  if( h10 )
    delete h10;
  h10 = new
    TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i", vthr, vtrm, tbits ),
	  Form( "Thresholds Vthr %i Vtrim %i bits %i;threshold [small Vcal DAC];pixels", vthr, vtrm, tbits ),
	  255, -0.5, 254.5 ); // 255 = overflow
  h10->Sumw2();

  for( int col = 0; col < 52; ++col )
    for( int row = 0; row < 80; ++row )
      h10->Fill( modthr[roc][col][row] );

  h10->Draw( "hist" );
  c1->Update();
  h10->Write();

  cout << endl;

  bool contTrim = true;

  if( nok < 1 ) {
    cout << "no responses" << endl;
    contTrim = 0;
  }

  int vmin = 255;
  int vmax = 0;
  int colmax = -1;
  int rowmax = -1;

  for( int row = 0; row < 80; ++row )
    for( int col = 0; col < 52; ++col ) {

      int thr = modthr[roc][col][row];
      //if( thr > vmax && thr < 255 ) {
      if( thr > vmax && thr < 233 ) { // for 205i
	vmax = thr;
	colmax = col;
	rowmax = row;
      }
      if( thr < vmin )
	vmin = thr;
    } // cols

  if( vmin < target ) {
    cout << "min threshold " << vmin << " below target on ROC " << roc
	 << ": skipped (try lower VthrComp)" << endl;
    contTrim = 0;
  }

  cout << "max thr " << vmax << " at " << colmax << " " << rowmax << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // setVtrim using max pixel:

  if( contTrim ) {

    tbits = 0; // 0 = strongest

    modtrm[roc][colmax][rowmax] = tbits;

    tb.roc_Col_Enable( colmax, true );

    tb.Flush();

    if( h11 )
      delete h11;
    h11 = new
      TH1D( Form( "thr_vs_Vtrim_col%i_row%i", colmax, rowmax ),
	    Form
	    ( "Threshold vs Vtrim pix %i %i;Vtrim [DAC];threshold [small Vcal DAC]",
	      colmax, rowmax ), 128, 0, 256 );
    h11->Sumw2();

    vtrm = 1;
    for( ; vtrm < 253; vtrm += 2 ) {

      tb.SetDAC( Vtrim, vtrm );
      tb.uDelay( 100 );
      tb.Flush();

      int thr = ThrPix( 0, colmax, rowmax, Vcal, step, nTrig );
      /* before FW 4.4
	 tb.roc_Pix_Trim( colmax, rowmax, tbits );
	 int thr = tb.PixelThreshold( colmax, rowmax,
	 guess, step,
	 thrLevel, nTrig,
	 Vcal, xtalk, cals );
      */
      h11->Fill( vtrm, thr );
      cout << setw(3) << vtrm << "  " << setw(3) << thr << endl;
      if( thr < target )
	break;
      if( thr > 254 )
	break;

    } // vtrm

    double trmslp = double(vmax-target) / 15; // delta_threshold/trimbit for this Vtrim for one pix
    if( trmslp < 1 ) trmslp = 1;

    h11->Draw( "hist" );
    c1->Update();
    h11->Write();

    vtrm += 2; // margin
    tb.SetDAC( Vtrim, vtrm );
    cout << "set Vtrim to " << vtrm << endl;
    dacval[roc][Vtrim] = vtrm;

    cout << "threshold trim slope " << trmslp << " Vcal DAC per trim bit" << endl;

    tb.roc_Pix_Mask( colmax, rowmax );
    tb.roc_Col_Enable( colmax, 0 );
    tb.roc_ClrCal();
    tb.Flush();

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // iterate trim bits:

    //tbits = 7; // half way, sometimes too much
    //tbits = 10; // 25.5.2015
    tbits = 15; // no trim, 26.6.2015

    for( int row = 0; row < 80; ++row )
      for( int col = 0; col < 52; ++col ) {
	rocthr[col][row] = modthr[roc][col][row]; // remember, BUG fixed
	roctrm[col][row] = modtrm[roc][col][row]; // old
	modtrm[roc][col][row] = tbits;
      }

    double correction = -trmslp; // flag

    TrimStep( roc, target, correction, step, nTrig ); // fills modtrm, fills modthr

    if( h12 )
      delete h12;
    h12 = new
      TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i", vthr, vtrm, int(trmslp) ),
	    Form( "Thresholds Vthr %i Vtrim %i bits %f;threshold [small Vcal DAC];pixels", vthr, vtrm, trmslp ),
	    255, -0.5, 254.5 ); // 255 = overflow
    h12->Sumw2();

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
	h12->Fill( modthr[roc][col][row] );

    h12->Draw( "hist" );
    c1->Update();
    h12->Write();

    correction = 2;

    TrimStep( roc, target, correction, step, nTrig ); // fills modtrm, fills modthr

    if( h13 )
      delete h13;
    h13 = new
      TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i_2", vthr, vtrm, int(trmslp) ),
	    Form( "Thresholds Vthr %i Vtrim %i bits %f 2;threshold [small Vcal DAC];pixels", vthr, vtrm, trmslp ),
	    255, -0.5, 254.5 ); // 255 = overflow
    h13->Sumw2();

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
	h13->Fill( modthr[roc][col][row] );

    h13->Draw( "hist" );
    c1->Update();
    h13->Write();

    correction = 1;

    TrimStep( roc, target, correction, step, nTrig ); // fills modtrm, fills modthr

    if( h14 )
      delete h14;
    h14 = new
      TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i_2_1", vthr, vtrm, int(trmslp) ),
	    Form( "Thresholds Vthr %i Vtrim %i bits %f 2 1;threshold [small Vcal DAC];pixels", vthr, vtrm, trmslp ),
	    255, -0.5, 254.5 ); // 255 = overflow
    h14->Sumw2();

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
	h14->Fill( modthr[roc][col][row] );

    h14->Draw( "hist" );
    c1->Update();
    h14->Write();

    correction = 1;

    TrimStep( roc, target, correction, step, nTrig ); // fills modtrm, fills modthr

    if( h15 )
      delete h15;
    h15 = new
      TH1D( Form( "thr_dist_Vthr%i_Vtrm%i_bits%i_2_1_1", vthr, vtrm, int(trmslp) ),
	    Form( "Thresholds Vthr %i Vtrim %i bits %f 2 1 1;threshold [small Vcal DAC];pixels", vthr, vtrm, trmslp ),
	    255, -0.5, 254.5 ); // 255 = overflow
    h15->Sumw2();

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row )
	h15->Fill( modthr[roc][col][row] );

    h15->Draw( "hist" );
    c1->Update();
    h15->Write();

    vector < uint8_t > trimvalues( 4160 ); // uint8 in 2.15
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	int i = 80 * col + row;
	trimvalues[i] = roctrm[col][row];
      }
    tb.SetTrimValues( roc, trimvalues );
    cout << "SetTrimValues in FPGA" << endl;

    tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
    tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
    tb.Flush();
    cout << "roc " << roc << " back to Vcal " << dacval[roc][Vcal]
	 << ", CtrlReg " << dacval[roc][CtrlReg]
	 << endl;

  } // contTrim

  cout << "  histos 10, 11, 12, 13, 14, 15" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // trim

//------------------------------------------------------------------------------
// Daniel Pitzl, DESY, 30.5.2014: fine tune trim bits (full efficiency)
CMD_PROC( trimbits )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  if( ierror ) return false;

  Log.section( "TRIMBITS", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int nTrig = 100;

  size_t roc = 0;

  if( roclist[roc] == 0 )
    return 0;

  tb.roc_I2cAddr( roc );

  cout << endl
       << setw( 2 ) << "ROC " << roc << ", VthrComp " << dacval[roc][VthrComp]
       << ", Vtrim " << dacval[roc][Vtrim]
       << endl;

  // measure efficiency map:

  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  GetRocData( nTrig, nReadouts, PHavg, PHrms );

  // analyze:

  size_t j = 0;

  for( int col = 0; col < 52; ++col ) {

    for( int row = 0; row < 80 && j < nReadouts.size(); ++row ) {

      int cnt = nReadouts.at( j );
      ++j;

      //if( cnt == nTrig )
      if( cnt >= 0.98 * nTrig )
        continue; // perfect

      int trm = modtrm[roc][col][row];

      cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row
	   << " trimbits " << trm << " responses " << cnt << endl;

      if( trm == 15 )
        continue;

      // increase trim bits:

      ++trm;

      for( ; trm <= 15; ++trm ) {

        modtrm[roc][col][row] = trm;

        vector < uint8_t > trimvalues( 4160 );

        for( int ix = 0; ix < 52; ++ix )
          for( int iy = 0; iy < 80; ++iy ) {
            int i = 80 * ix + iy;
            trimvalues[i] = modtrm[roc][ix][iy];
          } // iy
        tb.SetTrimValues( roc, trimvalues ); // load into FPGA
        tb.Flush();

        double ph;
        double rms;
        GetPixData( roc, col, row, nTrig, cnt, ph, rms );

        cout << "pixel " << setw( 2 ) << col << setw( 3 ) << row
	     << " trimbits " << trm << " responses " << cnt << endl;

        if( cnt == nTrig )
          break;

      } // trm

    } // row

  } // col

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // trimbits

//------------------------------------------------------------------------------
bool tunePH( int col, int row, int roc )
{
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  //tb.SetDAC( CtrlReg, 4 ); // large Vcal
  //tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // scan Vcal for one pixel

  int dac = Vcal;
  int16_t nTrig = 9;

  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 256 ); // size 0, capacity 256
  PHavg.reserve( 256 );
  PHrms.reserve( 256 );

  DacScanPix( roc, col, row, dac, 1, nTrig, nReadouts, PHavg, PHrms );

  if( nReadouts.size() < 256 ) {
    cout << "only " << nReadouts.size() << " Vcal points"
	 << ". choose different pixel, check CalDel, check Ia, or give up"
	 << endl;
    return 0;
  }

  if( nReadouts.at( nReadouts.size() - 1 ) < nTrig ) {
    cout << "only " << nReadouts.at( nReadouts.size() - 1 )
	 << " responses at " << nReadouts.size() - 1
	 << ". choose different pixel, check CalDel, check Ia, or give up" <<
      endl;
    return 0;
  }

  // scan from end, search for smallest responding Vcal:

  int minVcal = 255;

  for( int idac = nReadouts.size() - 1; idac >= 0; --idac )
    if( nReadouts.at( idac ) == nTrig )
      minVcal = idac;
  cout << "min responding Vcal " << minVcal << endl;
  minVcal += 1; // safety

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // offset and gain DACs:

  int offsdac = VoffsetOp; // digV2, positive slope
  int gaindac = VIref_ADC;
  if( Chip >= 400 ) {
    offsdac = VoffsetRO; // digV2.1, positive slope
    gaindac = PHScale;
  }

  cout << "start offset dac " << offsdac
       << " at " << dacval[roc][offsdac] << endl;
  cout << "start gain   dac " << gaindac
       << " at " << dacval[roc][gaindac] << endl;

  // set gain to minimal (at DAC 255), to avoid overflow or underflow:

  tb.SetDAC( gaindac, 255 );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Scan PH vs offset at Vcal 255:

  tb.SetDAC( Vcal, 255 ); // max Vcal
  vector< double > PHmax;
  PHmax.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();
  DacScanPix( roc, col, row, offsdac, 1, nTrig, nReadouts, PHmax, PHrms );

  // Scan PH vs offset at minVcal:

  tb.SetDAC( Vcal, minVcal );
  vector< double > PHmin;
  PHmin.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();
  DacScanPix( roc, col, row, offsdac, 1, nTrig, nReadouts, PHmin, PHrms );

  // use offset to center PH at 132:

  int offs = 0;
  double phmid = 0;
  for( size_t idac = 0; idac < PHmin.size(); ++idac ) {
    double phmean = 0.5 * ( PHmin.at( idac ) + PHmax.at( idac ) );
    if( fabs( phmean - 132 ) < fabs( phmid - 132 ) ) {
      offs = idac;
      phmid = phmean;
    }
  }

  cout << "mid PH " << phmid << " at offset " << offs << endl;

  tb.SetDAC( offsdac, offs );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // scan PH vs gain at Vcal 255 and at minVcal

  tb.SetDAC( Vcal, 255 );
  vector < double > PHtop;
  PHtop.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();

  DacScanPix( roc, col, row, gaindac, 1, nTrig, nReadouts, PHtop, PHrms );

  tb.SetDAC( Vcal, minVcal );
  vector < double > PHbot;
  PHbot.reserve( 256 );
  nReadouts.clear();
  PHrms.clear();

  DacScanPix( roc, col, row, gaindac, 1, nTrig, nReadouts, PHbot, PHrms );

  // set gain:

  int gain = PHtop.size() - 1;
  for( ; gain >= 1; --gain ) {
    if( PHtop.at( gain ) > 233 )
      break;
    if( PHbot.at( gain ) < 22 )
      break;
  }
  cout << "top PH " << PHtop.at( gain )
       << " at gain " << gain
       << " for Vcal 255" << " for pixel " << col << " " << row << endl;
  cout << "bot PH " << PHbot.at( gain )
       << " at gain " << gain
       << " for Vcal " << minVcal << " for pixel " << col << " " << row << endl;

  tb.SetDAC( gaindac, gain );
  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check for all pixels

  vector < int16_t > nResponses; // size 0
  vector < double > QHmax;
  vector < double > QHrms;
  nResponses.reserve( 4160 ); // size 0, capacity 4160
  QHmax.reserve( 4160 );
  QHrms.reserve( 4160 );

  tb.SetDAC( Vcal, 255 );
  tb.Flush();

  bool again = 0;

  int colmax = 0;
  int rowmax = 0;
  double phmax = 0;

  do { // no overflows

    GetRocData( nTrig, nResponses, QHmax, QHrms );

    size_t j = 0;
    phmax = 0;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        double ph = QHmax.at( j );
        if( ph > phmax ) {
          phmax = ph;
          colmax = col;
          rowmax = row;
        }

        ++j;
        if( j == nReadouts.size() )
          break;

      } // row

      if( j == nReadouts.size() )
        break;

    } // col

    cout << "max PH " << phmax << " at " << colmax << " " << rowmax << endl;

    if( phmax > 252 && gain < 255 ) {

      gain += 1; // reduce gain
      tb.SetDAC( gaindac, gain );
      tb.Flush();

      cout << "gain dac " << dacName[gaindac] << " set to " << gain << endl;

      again = 1;
      nResponses.clear();
      QHmax.clear();
      QHrms.clear();

    }
    else
      again = 0;

  } while( again );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // check all pixels for underflow at minVcal:

  vector < double > QHmin;
  QHmin.reserve( 4160 );
  nResponses.clear();
  QHrms.clear();

  tb.SetDAC( Vcal, minVcal );
  tb.Flush();

  again = 0;

  int colmin = 0;
  int rowmin = 0;
  double phmin = 255;

  do { // no underflows

    GetRocData( nTrig, nResponses, QHmin, QHrms );

    size_t j = 0;
    phmin = 255;

    for( int col = 0; col < 52; ++col ) {

      for( int row = 0; row < 80; ++row ) {

        if( nResponses.at( j ) < nTrig / 2 ) {
          cout << "pixel " << col << " " << row << " below threshold at Vcal "
	       << minVcal << endl;
        }
        else {
          double ph = QHmin.at( j );
          if( ph < phmin ) {
            phmin = ph;
            colmin = col;
            rowmin = row;
          }
        }

        ++j;
        if( j == nReadouts.size() )
          break;

      } // row

      if( j == nReadouts.size() )
        break;

    } // col

    cout << "min PH " << phmin << " at " << colmin << " " << rowmin << endl;

    if( phmin < 3 && gain > 0 ) {

      gain += 1; // reduce gain
      tb.SetDAC( gaindac, gain );
      tb.Flush();

      cout << "gain dac " << dacName[gaindac] << " set to " << gain << endl;

      again = 1;
      nResponses.clear();
      QHmin.clear();
      QHrms.clear();

    }
    else
      again = 0;

  } while( again );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // we have min and max:

  cout << "max PH " << phmax << " at " << colmax << " " << rowmax << endl;
  cout << "min PH " << phmin << " at " << colmin << " " << rowmin << endl;

  // adjust gain and offset such that max-min = 200, mid = 132

  again = 0;

  do {

    int cnt;
    double rms;

    tb.SetDAC( Vcal, minVcal );
    tb.Flush();
    double phmin = 255;
    GetPixData( roc, colmin, rowmin, nTrig, cnt, phmin, rms );

    tb.SetDAC( Vcal, 255 );
    tb.Flush();
    double phmax = 0;
    GetPixData( roc, colmax, rowmax, nTrig, cnt, phmax, rms );

    double phmid = 0.5 * ( phmax + phmin );
    double range = phmax - phmin;
    cout << "PH rng " << range << endl;
    cout << "PH mid " << phmid << endl;

    again = 0;

    if( phmid > 132 + 3 && offs > 0 ) { // 3 is margin of convergence
      offs -= 1;
      again = 1;
    }
    else if( phmid < 132 - 3 && offs < 255 ) {
      offs += 1;
      again = 1;
    }
    if( again ) {
      tb.SetDAC( offsdac, offs );
      tb.Flush();
      cout << "offs dac " << dacName[offsdac] << " set to " << offs << endl;
    }

    if( range > 200 + 3 && gain < 255 ) { // 3 is margin of convergence
      gain += 1; // reduce gain
      again = 1;
    }
    else if( range < 200 - 3 && gain > 0 ) {
      gain -= 1; // more gain
      again = 1;
    }
    if( again ) {
      tb.SetDAC( gaindac, gain );
      tb.Flush();
      cout << "gain dac " << dacName[gaindac] << " set to " << gain << endl;
    }

  }
  while( again );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // done

  cout << "final offs dac " << dacName[offsdac] << " set to " << offs << endl;
  cout << "final gain dac " << dacName[gaindac] << " set to " << gain << endl;

  dacval[roc][offsdac] = offs;
  dacval[roc][gaindac] = gain;

  Log.printf( "[SETDAC] %i  %i\n", gaindac, gain );
  Log.printf( "[SETDAC] %i  %i\n", offsdac, offs );
  Log.flush();

  tb.SetDAC( Vcal, dacval[roc][Vcal] ); // restore
  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.Flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // tunePH

//------------------------------------------------------------------------------
CMD_PROC( tune ) // adjust PH gain and offset to fit into ADC range
{
  if( ierror ) return false;

  int roc = 0;
  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );

  tunePH( col, row, roc );

  return true;
}

//------------------------------------------------------------------------------
CMD_PROC( phmap ) // check gain tuning and calibration
{
  if( ierror ) return false;

  int nTrig;
  PAR_INT( nTrig, 1, 65000 );

  const int vctl = dacval[0][CtrlReg];
  const int vcal = dacval[0][Vcal];

  Log.section( "PHMAP", false );
  Log.printf( "CtrlReg %i Vcal %i nTrig %i\n", vctl, vcal, nTrig );
  cout << "PHmap with " << nTrig << " triggers"
       << " at Vcal " << vcal << ", CtrlReg " << vctl << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // measure:

  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  GetRocData( nTrig, nReadouts, PHavg, PHrms );

  // analyze:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( Form( "ph_dist_Vcal%i_CR%i", vcal, vctl ),
          Form( "PH distribution at Vcal %i, CtrlReg %i;<PH> [ADC];pixels",
                vcal, vctl ), 256, -0.5, 255.5 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( Form( "vcal_dist_Vcal%i_CR%i", vcal, vctl ),
          Form( dacval[0][CtrlReg] == 0 ?
                "PH Vcal distribution at Vcal %i, CtrlReg %i;calibrated PH [small Vcal DAC];pixels"
                :
                "PH Vcal distribution at Vcal %i, CtrlReg %i;calibrated PH [large Vcal DAC];pixels",
                vcal, vctl ), 256, 0, 256 );
  h12->Sumw2();

  if( h13 )
    delete h13;
  h13 = new
    TH1D( Form( "rms_dist_Vcal%i_CR%i", vcal, vctl ),
          Form
          ( "PH RMS distribution at Vcal %i, CtrlReg %i;PH RMS [ADC];pixels",
            vcal, vctl ), 100, 0, 2 );
  h13->Sumw2();

  if( h21 )
    delete h21;
  h21 = new TH2D( Form( "PHmap_Vcal%i_CR%i", vcal, vctl ),
                  Form( "PH map at Vcal %i, CtrlReg %i;col;row;<PH> [ADC]",
                        vcal, vctl ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

  if( h22 )
    delete h22;
  h22 = new TH2D( Form( "VcalMap_Vcal%i_CR%i", vcal, vctl ),
                  Form( dacval[0][CtrlReg] == 0 ?
                        "PH Vcal map at Vcal %i, CtrlReg %i;col;row;calibrated <PH> [small Vcal DAC]"
                        :
                        "PH Vcal map at Vcal %i, CtrlReg %i;col;row;calibrated <PH> [large Vcal DAC]",
                        vcal, vctl ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

  if( h23 )
    delete h23;
  h23 = new TH2D( Form( "RMSmap_Vcal%i_CR%i", vcal, vctl ),
                  Form( "RMS map at Vcal %i, CtrlReg %i;col;row;PH RMS [ADC]",
                        vcal, vctl ), 52, -0.5, 51.5, 80, -0.5, 79.5 );

  int nok = 0;
  double sum = 0;
  double su2 = 0;
  double phmin = 256;
  double phmax = -1;
  int colmin = 0;
  int colmax = 0;
  int rowmin = 0;
  int rowmax = 0;
  
  size_t j = 0;

  for( int col = 0; col < 52; ++col ) {

    //cout << endl << setw( 2 ) << col << " ";

    for( int row = 0; row < 80; ++row ) {

      int cnt = nReadouts.at( j );
      double ph = PHavg.at( j );
      double rms = PHrms.at( j );
      /*
      cout << setw( 4 ) << ( ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 )
	   <<"(" << setw( 3 ) << cnt << ")";
      if( row % 10 == 9 )
        cout << endl << "   ";
      */
      Log.printf( " %i", ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 );

      if( cnt > 0 ) {
        ++nok;
        sum += ph;
        su2 += ph * ph;
        if( ph < phmin ) {
	  colmin = col;
	  rowmin = row;
          phmin = ph;
	}
        if( ph > phmax ) {
	  colmax = col;
	  rowmax = row;
          phmax = ph;
	}
        h11->Fill( ph );
        double vc = PHtoVcal( ph, 0, col, row );
        h12->Fill( vc );
        h13->Fill( rms );
        h21->Fill( col, row, ph );
        h22->Fill( col, row, vc );
        h23->Fill( col, row, rms );
      }

      ++j;
      if( j == nReadouts.size() )
        break;

    } // row

    Log.printf( "\n" );
    if( j == nReadouts.size() )
      break;

  } // col

  cout << endl;

  h11->Write();
  h12->Write();
  h13->Write();
  h21->Write();
  h22->Write();
  h23->Write();
  cout << "  histos 11, 12, 13, 21, 22" << endl;

  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw( "hist" );
  h12->SetLineColor( 2 );
  h12->SetMarkerColor( 2 );
  h12->Draw( "same" );
  c1->Update();

  cout << endl;
  cout << "CtrlReg " << dacval[0][CtrlReg]
       << ", Vcal " << dacval[0][Vcal]
       << endl;

  cout << "Responding pixels " << nok << endl;
  if( nok > 0 ) {
    cout << "PH min " << phmin << " at " << colmin << "  " << rowmin << endl;
    cout << "PH max " << phmax << " at " << colmax << "  " << rowmax << endl;
    double mid = sum / nok;
    double rms = sqrt( su2 / nok - mid * mid );
    cout << "PH mean " << mid << ", rms " << rms << endl;
  }
  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // phmap

//------------------------------------------------------------------------------
CMD_PROC( calsmap ) // test PH map through sensor
{
  if( ierror ) return false;

  int nTrig;
  PAR_INT( nTrig, 1, 65000 );

  Log.section( "CALSMAP", false );
  Log.printf( "CtrlReg %i Vcal %i nTrig %i\n",
              dacval[0][CtrlReg], dacval[0][Vcal], nTrig );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // measure:

  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  GetRocData( -nTrig, nReadouts, PHavg, PHrms ); // -nTRig = CALS flag

  // analyze:

  if( h11 )
    delete h11;
  h11 = new TH1D( "phroc", "PH distribution;<PH> [ADC];pixels", 256, 0, 256 );
  h11->Sumw2();

  int nok = 0;
  double sum = 0;
  double su2 = 0;
  double phmin = 256;
  double phmax = -1;

  size_t j = 0;

  for( int col = 0; col < 52; ++col ) {
    cout << endl << setw( 2 ) << col << " ";
    for( int row = 0; row < 80 && j < nReadouts.size(); ++row ) {
      int cnt = nReadouts.at( j );
      double ph = PHavg.at( j );
      cout << setw( 4 ) << ( ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 )
	   <<"(" << setw( 3 ) << cnt << ")";
      if( row % 10 == 9 )
        cout << endl << "   ";
      Log.printf( " %i", ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 );
      if( cnt > 0 ) {
        ++nok;
        sum += ph;
        su2 += ph * ph;
        if( ph < phmin )
          phmin = ph;
        if( ph > phmax )
          phmax = ph;
        h11->Fill( ph );
      }
      ++j;
    } // row
    Log.printf( "\n" );
  } // col

  h11->Write();
  gStyle->SetOptStat( 111111 );
  gStyle->SetStatY( 0.60 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11" << endl;

  cout << endl;
  cout << "CtrlReg " << dacval[0][CtrlReg]
       << ", Vcal " << dacval[0][Vcal]
       << endl;

  cout << "Responding pixels " << nok << endl;
  if( nok > 0 ) {
    cout << "PH min " << phmin << ", max " << phmax << endl;
    double mid = sum / nok;
    double rms = sqrt( su2 / nok - mid * mid );
    cout << "PH mean " << mid << ", rms " << rms << endl;
  }
  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // calsmap

//------------------------------------------------------------------------------
CMD_PROC( bbtest ) // bump bond test
{
  if( ierror ) return false;

  int nTrig;
  PAR_INT( nTrig, 1, 65000 );

  Log.section( "BBTEST", false );
  Log.printf( "nTrig %i\n", nTrig );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  const int vctl = dacval[0][CtrlReg];
  tb.SetDAC( CtrlReg, 4 ); // large Vcal

  const int vcal = dacval[0][Vcal];
  tb.SetDAC( Vcal, 255 ); // max Vcal

  tb.Flush();

  // measure:

  vector < int16_t > nReadouts; // size 0
  vector < double > PHavg;
  vector < double > PHrms;
  nReadouts.reserve( 4160 ); // size 0, capacity 4160
  PHavg.reserve( 4160 );
  PHrms.reserve( 4160 );

  GetRocData( -nTrig, nReadouts, PHavg, PHrms ); // -nTRig = CALS flag

  tb.SetDAC( Vcal, vcal ); // restore
  tb.SetDAC( CtrlReg, vctl );
  tb.Flush();

  // analyze:

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "calsPH",
          "Cals PH distribution;Cals PH [ADC];pixels", 256, 0, 256 );
  h11->Sumw2();

  if( h21 )
    delete h21;
  h21 = new TH2D( "BBTest",
                  "Cals response map;col;row;responses / pixel",
                  52, -0.5, 51.5, 80, -0.5, 79.5 );

  if( h22 )
    delete h22;
  h22 = new TH2D( "CalsPHmap",
                  "Cals PH map;col;row;Cals PH [ADC]",
                  52, -0.5, 51.5, 80, -0.5, 79.5 );

  int nok = 0;
  int nActive = 0;
  int nPerfect = 0;

  double sum = 0;
  double su2 = 0;
  double phmin = 256;
  double phmax = -1;

  size_t j = 0; // index in data vectors

  for( int col = 0; col < 52; ++col ) {
    cout << endl << setw( 2 ) << col << " ";
    for( int row = 0; row < 80 && j < nReadouts.size(); ++row ) {
      int cnt = nReadouts.at( j );
      double ph = PHavg.at( j );
      cout << setw( 3 ) << cnt
	   << "(" << setw( 3 ) << ( ( ph > -0.1 ) ? int ( ph + 0.5 ) : -1 )
	   <<")";
      if( row % 10 == 9 )
        cout << endl << "   ";

      //Log.printf( " %i", (ph > -0.1 ) ? int(ph+0.5) : -1 );
      Log.printf( " %i", cnt );

      if( cnt > 0 ) {
        ++nok;
        if( cnt >= nTrig / 2 )
          ++nActive;
        if( cnt == nTrig )
          ++nPerfect;
        sum += ph;
        su2 += ph * ph;
        if( ph < phmin )
          phmin = ph;
        if( ph > phmax )
          phmax = ph;
        h11->Fill( ph );
        h21->Fill( col, row, cnt );
        h22->Fill( col, row, ph );
      }
      else {
        cout << "no response from " << setw( 2 ) << col
	     << setw( 3 ) << row << endl;
      }
      ++j;
    } // row
    Log.printf( "\n" );
  } // col
  Log.flush();

  h11->Write();
  h21->Write();
  h22->Write();
  cout << "  histos 11, 21, 22" << endl;

  h21->SetStats( 0 );
  h21->GetYaxis()->SetTitleOffset( 1.3 );
  h21->SetMinimum( 0 );
  h21->SetMaximum( nTrig );
  h21->Draw( "colz" );
  c1->Update();

  cout << endl;
  cout << "Cals PH test: " << endl;
  cout << " >0% pixels: " << nok << endl;
  cout << ">50% pixels: " << nActive << endl;
  cout << "100% pixels: " << nPerfect << endl;

  Log.printf( "[Bump Statistic] \n" );
  Log.printf( " >0 pixels %i \n", nok );
  Log.printf( " >50 pixels %i \n", nActive );
  Log.printf( " >100 pixels %i \n", nPerfect );

  if( nok > 0 ) {
    cout << "PH min " << phmin << ", max " << phmax << endl;
    double mid = sum / nok;
    double rms = sqrt( su2 / nok - mid * mid );
    cout << "PH mean " << mid << ", rms " << rms << endl;
  }

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // bbtest

//------------------------------------------------------------------------------
void CalDelRoc() // scan and set CalDel using all pixel: 17 s
{
  Log.section( "CALDELROC", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // scan caldel:

  int val = dacval[0][CalDel];

  const int pln = 256;
  int plx[pln] = { 0 };
  int pl1[pln] = { 0 };
  int pl5[pln] = { 0 };
  int pl9[pln] = { 0 };

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "caldel_act", "CalDel ROC scan;CalDel [DAC];active pixels", 128, -1,
          255 );
  h11->Sumw2();

  if( h12 )
    delete h12;
  h12 = new
    TH1D( "caldel_per", "CalDel ROC scan;CalDel [DAC];perfect pixels", 128,
          -1, 255 );
  h12->Sumw2();

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize ); // 2^24
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );

  // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 1 );
    for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim );
    }
  }
  tb.uDelay( 1000 );
  tb.Flush();

  int n0 = 0;
  int idel = 10;
  int step = 10;

  do {

    tb.SetDAC( CalDel, idel );
    tb.Daq_Start();
    tb.uDelay( 1000 );
    tb.Flush();

    int n01, n50, n99;

    GetEff( n01, n50, n99 );

    plx[idel] = idel;
    pl1[idel] = n01;
    pl5[idel] = n50;
    pl9[idel] = n99;

    h11->Fill( idel, n50 );
    h12->Fill( idel, n99 );

    cout << setw( 3 ) << idel
	 << setw( 6 ) << n01 << setw( 6 ) << n50 << setw( 6 ) << n99 << endl;
    Log.printf( "%i %i %i %i\n", idel, n01, n50, n99 );

    if( n50 > 0 ) {
      if( step == 10 ) {
        step = 4;
        idel -= 10; // go back and measure finer
        n0 = 0;
      }
      else if( step == 4 ) {
        step = 2;
        idel -= 4; // go back and measure finer
        n0 = 0;
      }
    }

    if( n01 < 1 && step == 2 )
      ++n0; // speed up: count empty responses after peak 

    idel += step;

  }
  while( n0 < 4 && idel < 256 );

  cout << endl;
  Log.flush();

  // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  // compress and sort:

  int j = 0;
  for( int idel = 0; idel < 256; ++idel ) {
    if( pl1[idel] == 0 )
      continue;
    plx[j] = idel;
    pl1[j] = pl1[idel];
    pl5[j] = pl5[idel];
    pl9[j] = pl9[idel];
    ++j;
  }

  int nstp = j;
  cout << j << " points" << endl;

  h11->Write();
  h12->Write();
  
  h11->SetStats( 0 );
  h12->SetStats( 0 );
  h12->Draw( "hist" );
  c1->Update();
  
  cout << "  histos 11, 12" << endl;

  // analyze:

  int nmax = 0;
  int i0 = 0;
  int i9 = 0;

  for( int j = 0; j < nstp; ++j ) {

    int idel = plx[j];
    //int nn = pl5[j]; // 50% active
    int nn = pl9[j]; // 100% perfect
    if( nn > nmax ) {
      nmax = nn;
      i0 = idel; // begin
    }
    if( nn == nmax )
      i9 = idel; // plateau

  } // dac

  cout << "eff plateau from " << i0 << " to " << i9 << endl;

  if( nmax > 55 ) {
    int i2 = i0 + ( i9 - i0 ) / 4;
    tb.SetDAC( CalDel, i2 );
    tb.Flush();
    dacval[0][CalDel] = i2;
    printf( "set CalDel to %i\n", i2 );
    Log.printf( "[SETDAC] %i  %i\n", CalDel, i2 );
  }
  else
    tb.SetDAC( CalDel, val ); // back to default

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

} // caldelroc

//------------------------------------------------------------------------------
CMD_PROC( caldelroc ) // scan and set CalDel using all pixel: 17 s
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  if( ierror ) return false;
  CalDelRoc();
  return true;
}

//------------------------------------------------------------------------------
// Jamie Cameron, 11.10.2014: Measure Thrmap RMS as a function of VthrComp

CMD_PROC( scanvthr )
{
  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  int vthrmin;
  PAR_INT( vthrmin, 0, 254 );
  int vthrmax;
  PAR_INT( vthrmax, 1, 255 );
  int vthrstp;
  if( !PAR_IS_INT( vthrstp, 1, 255 ) )
    vthrstp = 5;
    
  if(vthrmin > vthrmax) {
    cout << "vthrmin > vthrmax, switching parameters." << endl;
    int temp = vthrmax;
    vthrmax = vthrmin;
    vthrmin = temp;
  }
      
  int nstp = (vthrmax-vthrmin)/vthrstp + 1;
  cout << "Number of steps " << nstp << endl;

  double mid = 150 - 1.35*vthrmin; // empirical estimate of first guess. 
  
  const int roc = 0;
  const int nTrig = 20;
  const int step = 1;

  tb.roc_I2cAddr( roc );
  tb.SetDAC( CtrlReg, 0 ); // measure thresholds at ctl 0
  tb.Flush();
  
  Log.section( "SCANVTHR", true );
  
  if( h13 )
    delete h13;
  h13 = new
    TProfile( "vthr_scan_caldel",
	      "Vthr scan caldel;VthrComp [DAC];CalDelRoc [DAC]",
	      nstp, vthrmin - 0.5*vthrstp, vthrmax + 0.5*vthrstp );
  if( h14 )
    delete h14;
  h14 = new
    TProfile( "vthr_scan_thrmean",
	      "Vthr scan thr mean;VthrComp [DAC];Threshold Mean [DAC]",
	      nstp, vthrmin - 0.5*vthrstp, vthrmax + 0.5*vthrstp );
  if( h15 )
    delete h15;
  h15 = new
    TProfile( "vthr_scan_thrrms",
	      "Vthr scan thr RMS;VthrComp [DAC];Threshold RMS [DAC]",
	      nstp, vthrmin - 0.5*vthrstp, vthrmax + 0.5*vthrstp );
  
  for( int vthr = vthrmin; vthr <= vthrmax; vthr += vthrstp ) {

    cout << "VthrComp " << vthr << endl;

    tb.SetDAC( VthrComp, vthr );
   	
    // caldelroc:
    cout << "CalDelRoc..." << endl;
    CalDelRoc(); // fills histos 11, 12
    
    h13->Fill( vthr,  dacval[0][CalDel] );
  
    // take thrmap:

    RocThrMap( roc, step, nTrig );
  
    int nok = 0;
    int sum = 0;
    int su2 = 0;

    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
	int thr = modthr[roc][col][row];
        
	// update RMS parameters:  
	if( thr < 255 ) {
	  sum += thr;
	  su2 += thr * thr;
	  ++nok;
	}
      }  
      
    // calculate RMS:  
      
    cout << "nok " << nok << endl;
      
    if( nok > 0 ) {
      mid = ( double ) sum / ( double ) nok;
      double rms = sqrt( ( double ) su2 / ( double ) nok - mid * mid );
      cout << "  mean thr " << mid << ", rms " << rms << endl;
      Log.flush();
	
      Log.printf( "%i %f\n", vthr, mid );
      h14->Fill( vthr, mid ); 
           
      Log.printf( "%i %f\n", vthr, rms );
      h15->Fill( vthr, rms );
            
    }

  } // vthr loop

  tb.SetDAC( CtrlReg, dacval[roc][CtrlReg] ); // restore
  tb.SetDAC( VthrComp, dacval[roc][VthrComp] ); 
  tb.SetDAC( Vcal, dacval[roc][Vcal] ); 
  tb.Flush();
  
  // plot profile:
  h13->Write();
  
  h14->Write();
    
  h15->Write();
  h15->SetStats( 0 );
  h15->Draw( "hist" );
  c1->Update();
  
  cout << "  histos 13, 14, 15" << endl;

  Log.flush();
  
  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "scan duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // scanvthr

//------------------------------------------------------------------------------
CMD_PROC( vanaroc ) // scan and set Vana using all pixel: 17 s
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  if( ierror ) return false;

  Log.section( "VANAROC", true );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // scan vana:

  int val = dacval[0][Vana];

  const int pln = 256;
  int plx[pln] = { 0 };
  int pl1[pln] = { 0 };
  int pl5[pln] = { 0 };
  int pl9[pln] = { 0 };

  if( h10 )
    delete h10;
  h10 = new
    TProfile( "ia_vs_vana", "Ia vs Vana;Vana [DAC];Ia [mA]", 256, -0.5, 255.5,
              0, 999 );

  if( h11 )
    delete h11;
  h11 = new
    TProfile( "active_vs_vana", "Vana ROC scan;Vana [DAC];active pixels", 256,
              -0.5, 255.5, -1, 9999 );

  if( h12 )
    delete h12;
  h12 = new
    TProfile( "perfect_vs_vana", "Vana ROC scan;Vana [DAC];perfect pixels",
              256, -0.5, 255.5, -1, 9999 );

  if( h13 )
    delete h13;
  h13 = new
    TProfile( "perfect_vs_ia", "efficiency vs Ia;IA [mA];perfect pixels", 800,
              0, 80, -1, 9999 );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize ); // 2^24
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );

  // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 1 );
    for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim );
    }
  }
  tb.uDelay( 1000 );
  tb.Flush();

  int idac = 10;
  int step = 10;
  int nmax = 0;
  int kmax = 0;

  do {

    tb.SetDAC( Vana, idac );
    tb.Daq_Start();
    tb.uDelay( 1000 );
    tb.Flush();

    int n01, n50, n99;

    GetEff( n01, n50, n99 );

    plx[idac] = idac;
    pl1[idac] = n01;
    pl5[idac] = n50;
    pl9[idac] = n99;

    if( n99 > nmax ) {
      nmax = n99;
      kmax = 0;
    }
    if( n99 == nmax )
      ++kmax;

    double ia = tb.GetIA() * 1E3;

    h10->Fill( idac, ia );
    h11->Fill( idac, n50 );
    h12->Fill( idac, n99 );
    h13->Fill( ia, n99 );

    cout << setw( 3 ) << idac
	 << setw( 6 ) << ia
	 << setw( 6 ) << n01 << setw( 6 ) << n50 << setw( 6 ) << n99 << endl;
    Log.printf( "%i %f %i %i %i\n", idac, ia, n01, n50, n99 );

    if( kmax < 4 && n50 > 0 ) {
      if( step == 10 ) {
        step = 4;
        idac -= 10; // go back and measure finer
        kmax = 0;
      }
      else if( step == 4 ) {
        step = 2;
        idac -= 4; // go back and measure finer
        kmax = 0;
      }
      else if( step == 2 ) {
        step = 1;
        idac -= 2; // go back and measure finer
        kmax = 0;
      }
    }

    if( kmax == 4 )
      step = 2;
    if( kmax == 10 )
      step = 5;
    if( kmax == 14 )
      step = 10;

    idac += step;

  }
  while( idac < 256 );

  cout << endl;
  Log.flush();

  // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  // compress and sort:

  int j = 0;
  for( int idac = 0; idac < 256; ++idac ) {
    if( pl1[idac] == 0 )
      continue;
    plx[j] = idac;
    pl1[j] = pl1[idac];
    pl5[j] = pl5[idac];
    pl9[j] = pl9[idac];
    ++j;
  }

  int nstp = j;
  cout << j << " points" << endl;

  // plot:

  h11->Write();
  h12->Write();
  h13->Write();
  if( par.isInteractive() ) {
    h11->SetStats( 0 );
    h12->SetStats( 0 );
    h13->SetStats( 0 );
    h12->Draw( "hist" );
    c1->Update();
  }
  cout << "  histos 11, 12, 13" << endl;

  // analyze:

  nmax = 0;
  int i0 = 0;
  int i9 = 0;

  for( int j = 0; j < nstp; ++j ) {

    int idac = plx[j];
    //int nn = pl5[j]; // 50% active
    int nn = pl9[j]; // 100% perfect
    if( nn > nmax ) {
      nmax = nn;
      i0 = idac; // begin
    }
    if( nn == nmax )
      i9 = idac; // plateau

  } // dac

  cout << "eff plateau from " << i0 << " to " << i9 << endl;

  tb.SetDAC( Vana, val ); // back to default

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // vanaroc

//------------------------------------------------------------------------------
CMD_PROC( gaindac ) // calibrated PH vs Vcal: check gain
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  if( ierror ) return false;

  Log.section( "GAINDAC", false );
  Log.printf( "CtrlReg %i\n", dacval[0][CtrlReg] );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  if( h11 )
    delete h11;
  h11 = new
    TH1D( "vc_dac",
          dacval[0][CtrlReg] == 0 ?
          "mean Vcal PH vs Vcal;Vcal [DAC];calibrated PH [small Vcal DAC]" :
          "mean Vcal PH vs Vcal;Vcal [DAC];calibrated PH [large Vcal DAC]",
          256, -0.5, 255.5 );
  h11->Sumw2();

  if( h14 )
    delete h14;
  h14 = new
    TH1D( "rms_dac",
          dacval[0][CtrlReg] == 0 ?
          "PH Vcal RMS vs Vcal;Vcal [DAC];calibrated PH RMS [small Vcal DAC]"
          :
          "PH Vcal RMS vs Vcal;Vcal [DAC];calibrated PH RMS [large Vcal DAC]",
          256, -0.5, 255.5 );
  h14->Sumw2();

  TH1D hph[4160];
  size_t j = 0;
  for( int col = 0; col < 52; ++col )
    for( int row = 0; row < 80; ++row ) {
      hph[j] = TH1D( Form( "PH_Vcal_c%02i_r%02i", col, row ),
                     Form( "PH vs Vcal col %i row %i;Vcal [DAC];<PH> [ADC]",
                           col, row ), 256, -0.5, 255.5 );
      ++j;
    }

  // Vcal loop:

  int32_t dacstrt = 0;
  int32_t dacstep = 1;
  int32_t dacstop = 255;
  //int32_t nstp = (dacstop-dacstrt) / dacstep + 1;

  for( int32_t ical = dacstrt; ical <= dacstop; ical += dacstep ) {

    tb.SetDAC( Vcal, ical );
    tb.Daq_Start();
    tb.uDelay( 1000 );
    tb.Flush();

    // measure:

    uint16_t nTrig = 10;
    vector < int16_t > nReadouts; // size 0
    vector < double > PHavg;
    vector < double > PHrms;
    nReadouts.reserve( 4160 ); // size 0, capacity 4160
    PHavg.reserve( 4160 );
    PHrms.reserve( 4160 );

    GetRocData( nTrig, nReadouts, PHavg, PHrms );

    // analyze:

    int nok = 0;
    double sum = 0;
    double su2 = 0;
    double vcmin = 256;
    double vcmax = -1;

    size_t j = 0;

    for( int col = 0; col < 52; ++col ) {
      for( int row = 0; row < 80 && j < nReadouts.size(); ++row ) {
        int cnt = nReadouts.at( j );
        double ph = PHavg.at( j );
        hph[j].Fill( ical, ph );
        double vc = PHtoVcal( ph, 0, col, row );
        if( cnt > 0 && vc < 999 ) {
          ++nok;
          sum += vc;
          su2 += vc * vc;
          if( vc < vcmin )
            vcmin = vc;
          if( vc > vcmax )
            vcmax = vc;
        }
        ++j;
      } // row
    } // col

    cout << setw( 3 ) << ical << ": pixels " << setw( 4 ) << nok;
    double mid = 0;
    double rms = 0;
    if( nok > 0 ) {
      cout << ", vc min " << vcmin << ", max " << vcmax;
      mid = sum / nok;
      rms = sqrt( su2 / nok - mid * mid );
      cout << ", mean " << mid << ", rms " << rms;
    }
    cout << endl;
    h11->Fill( ical, mid );
    h14->Fill( ical, rms );

  } // ical

  tb.SetDAC( Vcal, dacval[0][Vcal] ); // restore
  tb.Flush();

  j = 0;
  for( int col = 0; col < 52; ++col )
    for( int row = 0; row < 80; ++row ) {
      hph[j].Write();
      ++j;
    }

  h14->Write();
  h11->Write();
  h11->SetStats( 0 );
  h11->Draw( "hist" );
  c1->Update();
  cout << "  histos 11, 14" << endl;

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // gaindac

//------------------------------------------------------------------------------
// utility function for ROC DAC scan, fills 2D histos 21 (N vs DAC) and 22 (PH vs DAC)
bool dacscanroc( int dac, int nTrig=10, int step=1, int stop=255 )
{
  cout << "  scan dac " << dacName[dac]
       << " step " << step
       << " at CtrlReg " << dacval[0][CtrlReg]
       << ", VthrComp " << dacval[0][VthrComp]
       << ", CalDel " << dacval[0][CalDel]
       << endl;

  Log.section( "DACSCANROC", false );
  Log.printf( "DAC %i, step %i, nTrig %i\n", dac, step, nTrig );

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );

  // all on:

  for( int col = 0; col < 52; ++col ) {
    tb.roc_Col_Enable( col, 1 );
    for( int row = 0; row < 80; ++row ) {
      int trim = modtrm[0][col][row];
      tb.roc_Pix_Trim( col, row, trim );
    }
  }
  tb.uDelay( 1000 );

  int ctl = dacval[0][CtrlReg];
  int cal = dacval[0][Vcal];

  if( nTrig < 0 ) {
    tb.SetDAC( CtrlReg, 4 ); // large Vcal
    ctl = 4;
  }

  tb.Flush();

  uint16_t flags = 0; // normal CAL
  if( nTrig < 0 ) {
    flags = 0x0002; // FLAG_USE_CALS
    cout << "  CALS used..." << endl;
  }
  // flags |= 0x0010; // FLAG_FORCE_MASK is FPGA default

  uint16_t mTrig = abs( nTrig );

  int16_t dacLower1 = dacStrt( dac );
  int16_t dacUpper1 = stop;
  int32_t nstp = (dacUpper1 - dacLower1) / step + 1;

  dacUpper1 = dacLower1 + (nstp-1)*step; // 255 or 254 (23.8.2014)

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( h21 )
    delete h21;
  if( nTrig < 0 ) // cals
    h21 = new TH2D( Form( "cals_N_DAC%02i_map", dac ),
		    Form( "cals N vs %s map;pixel;%s [DAC];responses",
			  dacName[dac].c_str(),
			  dacName[dac].c_str() ),
		    4160, -0.5, 4160-0.5,
		    nstp, dacLower1 - 0.5*step, dacUpper1 + 0.5*step );
  else
    h21 = new TH2D( Form( "N_DAC%02i_CR%i_Vcal%03i_map", dac, ctl, cal ),
		    Form( "N vs %s CR %i Vcal %i map;pixel;%s [DAC];responses",
			  dacName[dac].c_str(), ctl, cal,
			  dacName[dac].c_str() ),
		    4160, -0.5, 4160-0.5,
		    nstp, dacLower1 - 0.5*step, dacUpper1 + 0.5*step ); // 23.8.2014 * step

  if( h22 )
    delete h22;
  if( nTrig < 0 ) // cals
    h22 = new TH2D( Form( "cals_PH_DAC%02i_map", dac ),
		    Form( "cals PH vs %s map;pixel;%s [DAC];cals PH [ADC]",
			  dacName[dac].c_str(),
			  dacName[dac].c_str() ),
		    4160, -0.5, 4160-0.5,
		    nstp, dacLower1 - 0.5*step, dacUpper1 + 0.5*step );
  else
    h22 = new TH2D( Form( "PH_DAC%02i_CR%i_Vcal%03i_map", dac, ctl, cal ),
		    Form( "PH vs %s CR %i Vcal %i map;pixel;%s [DAC];PH [ADC]",
			  dacName[dac].c_str(), ctl, cal,
			  dacName[dac].c_str() ),
		    4160, -0.5, 4160-0.5,
		    nstp, dacLower1 - 0.5*step, dacUpper1 + 0.5*step ); // 23.8.2014 * step

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // measure:

  cout << "  pulsing 4160 pixels with " << mTrig << " triggers for "
       << nstp << " DAC steps up to " << dacUpper1
       << " may take " << int ( 4160 * mTrig * nstp * 6.6e-6 ) + 1
       << " s..." << endl;

  //tb.SetTimeout( 4 * mTrig * nstp * 6 * 2 ); // [ms]

  // header = 1 word
  // pixel = +2 words
  // size = 256 dacs * 4160 pix * mTrig * 3 words = 3.2 MW * mTrig
  // 4 GB RAM limit: mTrig 670

  // nTrig 1600, step 1, stop 100, thr 78:
  // up to col 38 row 18 DAC 100: "done"
  // test duration 3246.99 s

  // nTrig 1100, step 1, stop 100, thr 49:
  // up to col 42 row 31 DAC 91
  // test duration 2337.42 s

  vector < uint16_t > data;
  data.reserve( min( nstp * mTrig * 4160 * 3, (int) tbState.GetDaqSize() ) );

  bool done = 0;
  double tloop = 0;
  double tread = 0;

  int ipx = -1;
  int idc = dacLower1;
  int jdc = 0; // DAC steps, per pixel

  while( done == 0 ) {

    timeval tv;
    gettimeofday( &tv, NULL );
    long s1 = tv.tv_sec; // seconds since 1.1.1970
    long u1 = tv.tv_usec; // microseconds

    tb.Daq_Start();
    tb.uDelay( 100 );

    try {
      done =
	tb.LoopSingleRocAllPixelsDacScan( 0, mTrig, flags, dac, step, dacLower1, dacUpper1 );
    }
    catch( CRpcError & e ) {
      e.What();
      return 0;
    }

    tb.Daq_Stop(); // avoid extra (noise) data

    gettimeofday( &tv, NULL );
    long s2 = tv.tv_sec; // seconds since 1.1.1970
    long u2 = tv.tv_usec; // microseconds
    double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;
    tloop += dt;
    cout << "  LoopSingleRocAllPixelsDacScan takes " << dt << " s" << endl;
    cout << ( done ? "  done" : "  not done" ) << endl;

    int dSize = tb.Daq_GetSize();
    cout << "  DAQ size " << dSize << " words" << endl;

    vector < uint16_t > dataB;
    dataB.reserve( Blocksize );

    try {
      uint32_t rest;
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(), dataB.begin(), dataB.end() );
      cout << "    data size " << data.size()
	   << ", remaining " << rest << endl;
      while( rest > 0 ) {
	dataB.clear();
	tb.uDelay( 5000 );
	tb.Daq_Read( dataB, Blocksize, rest );
	data.insert( data.end(), dataB.begin(), dataB.end() );
	cout << "    data size " << data.size()
	     << ", remaining " << rest << endl;
      }
    }
    catch( CRpcError & e ) {
      e.What();
      return 0;
    }

    // unpack data:
    // ~/psi/dtb/pixel-dtb-firmware/software/dtb_expert/trigger_loops.cpp
    // loop cols
    //   loop rows
    //     loop dacs
    //       loop trig

    int pos = 0;
    int err = 0;
    int iTrig = 0; // data block contains full trigger block
    int cnt = 0;
    int phsum = 0;

    while( pos < (int) data.size() ) {

      if( iTrig == 0 && jdc == 0 )
	++ipx;
      int col = ipx/80;
      int row = ipx%80;

      PixelReadoutData pix;
      err = DecodePixel( data, pos, pix ); // analyzer

      if( err ) {
	cout << "error " << err << " at trig " << iTrig
	     << ", stp " << jdc
	     << ", pix " << col << " " << row << ", pos " << pos << endl;
	break;
      }

      if( pix.n > 0 ) {
	if( pix.x != col ) {
	  cout << "error: col " << setw(2) << pix.x
	       << " at ipx " << setw(4) << ipx
	       << " should be " << setw(2) << col
	       << endl;
	  break;
	}
	if( pix.y != row ) {
	  cout << "error: row " << setw(2) << pix.y
	       << " at ipx " << setw(4) << ipx
	       << " should be " << setw(2) << row
	       << endl;
	  break;
	}
	++cnt;
	phsum += pix.p;
      }

      ++iTrig;

      if( iTrig == mTrig ) { // complete trigger block

	double ph = -1;
	if( cnt > 0 )
	  ph = ( double ) phsum / cnt;

	idc = dacLower1 + step*jdc;

	h22->Fill( ipx, idc, ph );
	h21->Fill( ipx, idc, cnt );

	iTrig = 0; // for next trigger block
	cnt = 0;
	phsum = 0;

	++jdc;

	if( jdc == nstp )
	  jdc = 0; // new pixel should come next

      } // reached mTrig

    } // while data

    data.clear();

    gettimeofday( &tv, NULL );
    long s3 = tv.tv_sec; // seconds since 1.1.1970
    long u3 = tv.tv_usec; // microseconds
    double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
    tread += dtr;
    cout << "  Daq_Read takes " << dtr << " s"
	 << " = " << 2 * dSize / dtr / 1024 / 1024 << " MiB/s"
	 << endl;
    cout << "  up to col " << ipx/80 << " row " << ipx%80
	 << " DAC " << idc
	 << endl;

    if( err ) break;

    if( !done )
      cout << "  loop more..." << endl << flush;

  } // while not done

  cout << "  tb.Loop takes " << tloop
       << " s = " << tloop / 4160 / mTrig / nstp * 1e6 << " us / pix"
       << endl;
  cout << "  Daq_Read takes " << tread << " s" << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  tb.SetDAC( dac, dacval[0][dac] ); // restore
  tb.SetDAC( CtrlReg, dacval[0][CtrlReg] ); // restore

  // all off:

  for( int col = 0; col < 52; col += 2 ) // DC
    tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.Flush();

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  return true;

} // dacscanroc

//------------------------------------------------------------------------------
CMD_PROC( dacscanroc ) // LoopSingleRocAllPixelsDacScan: 72 s with nTrig 10
// dacscanroc dac [[-]ntrig] [step] [stop]
// dacscanroc 25: PH vs Vcal = gain calibration
// dacscanroc 25 16 1 60: N  vs Vcal = Scurves, threshold
// dacscanroc 12 -16: cals N vs VthrComp = bump bond test
{
  if( ierror ) return false;

  if( tb.TBM_Present() ) {
    cout << "use dacscanmod for modules" << endl;
    return false;
  }

  int dac;
  PAR_INT( dac, 1, 32 ); // only DACs, not registers

  if( dacval[0][dac] == -1 ) {
    cout << "DAC " << dac << " not active" << endl;
    return false;
  }

  int nTrig;
  if( !PAR_IS_INT( nTrig, -65000, 65000 ) )
    nTrig = 16;

  int step;
  if( !PAR_IS_INT( step, 1, 127 ) )
    step = 1;

  int stop;
  if( !PAR_IS_INT( stop, 1, 255 ) )
    stop = dacStop( dac );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  dacscanroc( dac, nTrig, step, stop );

  int mTrig = abs( nTrig );

  if( h10 )
    delete h10;
  h10 = new TH1D( "Responses",
		  "Responses;max responses;pixels",
		  mTrig + 1, -0.5, mTrig + 0.5 );
  h10->Sumw2();

  if( h20 )
    delete h20;
  h20 = new TH2D( "ResponseMap",
		  "Response map;col;row;max responses",
		  52, -0.5, 51.5, 80, -0.5, 79.5 );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // BB test analysis:

  if( nTrig < 0 && dac == 12 ) {

    if( h14 )
      delete h14;
    h14 = new TH1D( "CalsVthrPlateauWidth",
		    "Width of VthrComp plateau for cals;width of VthrComp plateau for cals [DAC];pixels",
		    160/step, 0, 160 );

    if( h24 )
      delete h24;
    h24 = new TH2D( "BBtestMap",
		    "BBtest map;col;row;cals VthrComp response plateau width",
		    52, -0.5, 51.5, 80, -0.5, 79.5 );

    if( h25 )
      delete h25;
    h25 = new TH2D( "BBqualityMap",
		    "BBtest quality map;col;row;dead missing good",
		    52, -0.5, 51.5, 80, -0.5, 79.5 );

    // Localize the missing Bump from h21

    int nbinx = h21->GetNbinsX(); // pix
    int nbiny = h21->GetNbinsY(); // dac

    int nGood = 0;
    int nMissing = 0;
    int nWeak = 0;
    int nDead = 0;

    // pixels:

    for( int ibin = 1; ibin <= nbinx; ++ibin ) {

      int ipx = h21->GetXaxis()->GetBinCenter( ibin );

      // max response:

      int imax = 0;
      for( int j = 1; j <= nbiny; ++j ) {
	int cnt = h21->GetBinContent( ibin, j );
	if( cnt > imax )
	  imax = cnt;
      }
      h10->Fill( imax );
      h20->Fill( ipx / 80, ipx % 80, imax );

      if( imax == 0 ) {
	++nDead;
	cout << "    Dead pixel at col row " << ipx / 80
	     << " " << ipx % 80 << endl;
      }
      else if( imax < mTrig / 2 ) {
	++nWeak;
	cout << "    Weak pixel at col row " << ipx / 80
	     << " " << ipx % 80 << endl;
      }
      else {

	// Search for the Plateau

	int iEnd = 0;
	int iBegin = 0;

	// search for DAC response plateau:

	for( int jbin = 0; jbin <= nbiny; ++jbin ) {

	  int idac = h21->GetYaxis()->GetBinCenter( jbin );

	  int cnt = h21->GetBinContent( ibin, jbin );

	  if( cnt >= imax / 2 ) {
	    iEnd = idac; // end of plateau
	    if( iBegin == 0 )
	      iBegin = idac; // begin of plateau
	  }

	} // y-bins = DAC

	//cout << "Bin: " << ibin << " Plateau " << iBegin << " - " << iEnd << endl;

	// narrow plateau is from noise

	h14->Fill( iEnd - iBegin );
	h24->Fill( ipx / 80, ipx % 80, iEnd - iBegin );

	if( iEnd - iBegin < 35 ) {

	  ++nMissing;
	  cout << "    Missing Bump at col row " << ipx / 80
	       << " " << ipx % 80 << endl;
	  Log.printf( "[Missing Bump at col row] %i %i\n", ibin / 80, ibin % 80 );
	  h25->Fill( ipx / 80, ipx % 80, 2 ); // red
	}
	else {
	  ++nGood;
	  h25->Fill( ipx / 80, ipx % 80, 1 ); // green
	} // plateau width

      } // active imax

    } // x-bins = pixels

    // save the map in the log file

    for( int ibin = 1; ibin <= h24->GetNbinsX(); ++ibin ) {
      for( int jbin = 1; jbin <= h24->GetNbinsY(); ++jbin ) {
	int c_val = h24->GetBinContent( ibin, jbin );
	Log.printf( " %i", c_val );
	//cout << ibin << " " << jbin << " " << c_val << endl;
      } //row
      Log.printf( "\n" );
    } // col
    Log.flush();

    Log.printf( "Number of Good bump bonds [above trig/2]: %i\n", nGood );
    Log.printf( "Number of Missing bump bonds: %i\n", nMissing );
    Log.printf( "Number of Weak pixel: %i\n", nWeak );
    Log.printf( "Number of Dead pixel: %i\n", nDead );

    cout << "  Number of Good bump bonds: " << nGood << endl;
    cout << "  Number of Missing bump bonds: " << nMissing << endl;
    cout << "  Number of Weak pixel: " << nWeak << endl;
    cout << "  Number of Dead pixel: " << nDead << endl;

    h14->Write();
    h24->Write();
    h25->Write();
    cout << "  histos 10, 14, 20, 21, 22, 24, 25" << endl;

    h25->SetStats( 0 );
    h25->SetMinimum(0);
    h25->SetMaximum(2);
    h25->Draw( "colz" );
    c1->Update();

  } // BB test

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // thr analysis:

  else if( dacval[0][CtrlReg] == 0 && dac == 25 ) { // small Vcal

    if( h14 )
      delete h14;
    h14 = new TH1D( "thresholds",
		    "threshold distribution;threshold [small Vcal DACs];pixels",
		    151, -0.5, 150.5 );
    h14->Sumw2();

    if( h24 )
      delete h24;
    h24 = new TH2D( "thresholdMap",
		    "threshold map;col;row;threshold [small Vcal DACs]",
		    52, -0.5, 51.5, 80, -0.5, 79.5 );

    // analyze 2-D map h21:

    int nbinx = h21->GetNbinsX(); // pix
    int nbiny = h21->GetNbinsY(); // dac

    int nGood = 0;
    int nWeak = 0;
    int nDead = 0;

    // pixels:

    for( int ibin = 1; ibin <= nbinx; ++ibin ) {

      int ipx = h21->GetXaxis()->GetBinCenter( ibin );

      // max response:

      int imax = 0;
      for( int j = 1; j <= nbiny; ++j ) {
	int cnt = h21->GetBinContent( ibin, j );
	if( cnt > imax )
	  imax = cnt;
      }
      h10->Fill( imax );
      h20->Fill( ipx / 80, ipx % 80, imax );

      if( imax == 0 ) {
	++nDead;
	cout << "    Dead pixel at col row " << ipx / 80
	     << " " << ipx % 80 << endl;
      }
      else if( imax < mTrig / 2 ) {
	++nWeak;
	cout << "    Weak pixel at col row " << ipx / 80
	     << " " << ipx % 80 << endl;
      }
      else {

	// Search for threshold from zero:

	int ithr = 0;

	for( int jbin = 0; jbin <= nbiny; ++jbin ) {

	  int idac = h21->GetYaxis()->GetBinCenter( jbin );

	  int cnt = h21->GetBinContent( ibin, jbin );

	  if( cnt < imax / 2 ) // below 50%
	    continue;

	  ithr = idac; // first bin above 50% = thr
	  break;

	} // y-bins = DACs

	h14->Fill( ithr );
	h24->Fill( ipx / 80, ipx % 80, ithr );

	if( ithr > 0 )
	  ++nGood;

      } // good imax

    } // x-bins = pixels

    h14->Write();
    h24->Write();
    cout << "  histos 10, 14, 20, 21, 22, 24" << endl;

    h14->Draw();
    c1->Update();

    Log.printf( "Number of Good pixels: %i\n", nGood );
    Log.printf( "Number of Weak pixels: %i\n", nWeak );
    Log.printf( "Number of Dead pixels: %i\n", nDead );

    cout << "  Number of Good pixels: " << nGood << endl;
    cout << "  Number of Weak pixels: " << nWeak << endl;
    cout << "  Number of Dead pixels: " << nDead << endl;

    cout << "  mean threshold " << h14->GetMean() << endl;
    cout << "  threshold RMS  " << h14->GetRMS() << endl;

  } // thr map

  else {

    // analyze 2-D map h21:

    int nbinx = h21->GetNbinsX(); // pix
    int nbiny = h21->GetNbinsY(); // dac

    int nGood = 0;
    int nWeak = 0;
    int nDead = 0;

    // pixels:

    for( int ibin = 1; ibin <= nbinx; ++ibin ) {

      int ipx = h21->GetXaxis()->GetBinCenter( ibin );

      // max response:

      int imax = 0;
      for( int j = 1; j <= nbiny; ++j ) {
	int cnt = h21->GetBinContent( ibin, j );
	if( cnt > imax )
	  imax = cnt;
      }
      h10->Fill( imax );
      h20->Fill( ipx / 80, ipx % 80, imax );

      if( imax == 0 ) {
	++nDead;
	cout << "    Dead pixel at col row " << ipx / 80
	     << " " << ipx % 80 << endl;
      }
      else if( imax < mTrig / 2 ) {
	++nWeak;
	cout << "    Weak pixel at col row " << ipx / 80
	     << " " << ipx % 80 << endl;
      }
      else {
	++nGood;
      }

    } // x-bins = pixels

    cout << "  Number of Good pixels: " << nGood << endl;
    cout << "  Number of Weak pixels: " << nWeak << endl;
    cout << "  Number of Dead pixels: " << nDead << endl;

    h21->SetStats( 0 );
    h21->Draw( "colz" );
    c1->Update();
    cout << "  histos 10, 20, 21, 22" << endl;

  } // default

  h10->Write();
  h20->Write();

  h21->Write();
  h22->Write();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  Log.flush();

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;
}

//------------------------------------------------------------------------------
bool dacdac( int col, int row, int dac1, int dac2, int cals=1, uint16_t nTrig=10 )
{
  Log.section( "DACDAC", false );
  Log.printf( "pixel %i %i DAC %i vs %i\n", col, row, dac1, dac2 );

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize ); // 2^24
#endif
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );
  tb.uDelay( 100 );
  tb.Daq_Start();
  tb.uDelay( 100 );

  // pixel on:

  tb.roc_Col_Enable( col, 1 );
  int trim = modtrm[0][col][row];
  tb.roc_Pix_Trim( col, row, trim );
  tb.Flush();

  uint16_t flags = 0; // normal CAL

  if( cals )
    flags = 0x0002; // FLAG_USE_CALS;
  if( flags > 0 )
    cout << "CALS used..." << endl;

  if( dac1 == 11 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac1 == 12 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac2 == 11 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy
  if( dac2 == 12 )
    flags |= 0x0010; // FLAG_FORCE_MASK else noisy

  //flags |= 0x0020; // don't use DAC correction table

  cout << "flags hex " << hex << flags << dec << endl;

  int16_t dacLower1 = dacStrt( dac1 );
  int16_t dacUpper1 = dacStop( dac1 );
  int32_t nstp1 = dacUpper1 - dacLower1 + 1;

  int16_t dacLower2 = dacStrt( dac2 );
  int16_t dacUpper2 = dacStop( dac2 );
  int32_t nstp2 = dacUpper2 - dacLower2 + 1;

  gettimeofday( &tv, NULL );
  long s1 = tv.tv_sec; // seconds since 1.1.1970
  long u1 = tv.tv_usec; // microseconds

  // measure:

  cout << "pulsing " << nTrig << " triggers for "
       << nstp1 << " x " << nstp2 << " DAC steps may take "
       << int ( nTrig * nstp1 * nstp2 * 6e-6 ) + 1 << " s..." << endl;

  try {
    tb.LoopSingleRocOnePixelDacDacScan( 0, col, row, nTrig, flags,
                                        dac1, dacLower1, dacUpper1,
                                        dac2, dacLower2, dacUpper2 );
    // ~/psi/dtb/pixel-dtb-firmware/software/dtb_expert/trigger_loops.cpp
    // loop dac1
    //   loop dac2
    //     loop trig
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  tb.Daq_Stop(); // avoid extra (noise) data

  int dSize = tb.Daq_GetSize();

  gettimeofday( &tv, NULL );
  long s2 = tv.tv_sec; // seconds since 1.1.1970
  long u2 = tv.tv_usec; // microseconds
  double dt = s2 - s1 + ( u2 - u1 ) * 1e-6;

  cout << "LoopSingleRocOnePixelDacDacScan takes " << dt << " s"
       << " = " << dt / nTrig / nstp1 / nstp2 * 1e6 << " us / pix" << endl;

  tb.SetDAC( dac1, dacval[0][dac1] ); // restore
  tb.SetDAC( dac2, dacval[0][dac2] ); // restore

  tb.roc_Col_Enable( col, 0 );
  tb.roc_Chip_Mask();
  tb.roc_ClrCal();
  tb.Flush();

  // header = 1 word
  // pixel = +2 words
  // size = 256 dacs * 4160 pix * nTrig * 3 words = 32 MW

  cout << "DAQ size " << dSize << endl;

  vector < uint16_t > data;
  data.reserve( tb.Daq_GetSize() );

  try {
    uint32_t rest;
    tb.Daq_Read( data, Blocksize, rest );
    cout << "data size " << data.size()
	 << ", remaining " << rest << endl;
    while( rest > 0 ) {
      vector < uint16_t > dataB;
      dataB.reserve( Blocksize );
      tb.Daq_Read( dataB, Blocksize, rest );
      data.insert( data.end(), dataB.begin(), dataB.end() );
      cout << "data size " << data.size()
	   << ", remaining " << rest << endl;
      dataB.clear();
    }
  }
  catch( CRpcError & e ) {
    e.What();
    return 0;
  }

  gettimeofday( &tv, NULL );
  long s3 = tv.tv_sec; // seconds since 1.1.1970
  long u3 = tv.tv_usec; // microseconds
  double dtr = s3 - s2 + ( u3 - u2 ) * 1e-6;
  cout << "Daq_Read takes " << dtr << " s"
       << " = " << 2 * dSize / dtr / 1024 / 1024 << " MiB/s" << endl;

#ifdef DAQOPENCLOSE
  tb.Daq_Close();
  //tb.Daq_DeselectAll();
  tb.Flush();
#endif

  if( h21 )
    delete h21;
  h21 = new
    TH2D( Form( "PH_DAC%02i_DAC%02i_col%02i_row%02i", dac1, dac2, col, row ),
          Form( "PH vs %s vs %s col %i row %i;%s [DAC];%s [DAC];PH [ADC]",
                dacName[dac1].c_str(), dacName[dac2].c_str(), col, row,
                dacName[dac1].c_str(), dacName[dac2].c_str() ),
          nstp1, -0.5, nstp1 - 0.5, nstp2, -0.5, nstp2 - 0.5 );

  if( h22 )
    delete h22;
  h22 = new
    TH2D( Form( "N_DAC%02i_DAC%02i_col%02i_row%02i", dac1, dac2, col, row ),
          Form( "N vs %s vs %s col %i row %i;%s [DAC];%s [DAC];responses",
                dacName[dac1].c_str(), dacName[dac2].c_str(), col, row,
                dacName[dac1].c_str(), dacName[dac2].c_str() ),
          nstp1, -0.5, nstp1 - 0.5, nstp2, -0.5, nstp2 - 0.5 );

  if( h23 )
    delete h23;
  h23 = new
    TH2D( Form
          ( "N_Trig2DAC%02i_DAC%02i_col%02i_row%02i", dac1, dac2, col, row ),
          Form
          ( "Responses vs %s vs %s col %i row %i;%s [DAC];%s [DAC];responses",
            dacName[dac1].c_str(), dacName[dac2].c_str(), col, row,
            dacName[dac1].c_str(), dacName[dac2].c_str() ), nstp1, -0.5,
          nstp1 - 0.5, nstp2, -0.5, nstp2 - 0.5 );

  TH1D *h_one =
    new TH1D( Form( "h_optimal_DAC%02i_col%02i_row%02i", dac2, col, row ),
              Form( "optimal DAC %i col %i row %i", dac2, col, row ),
              nstp1, -0.5, nstp1 - 0.5 );
  h_one->Sumw2();

  // unpack data:

  PixelReadoutData pix;

  int pos = 0;
  int err = 0;

  for( int32_t i = 0; i < nstp1; ++i ) { // DAC steps

    for( int32_t j = 0; j < nstp2; ++j ) { // DAC steps

      int cnt = 0;
      int phsum = 0;

      for( int k = 0; k < nTrig; ++k ) {

        err = DecodePixel( data, pos, pix ); // analyzer

        if( err )
          cout << "error " << err << " at trig " << k
	       << ", stp " << j << " " << i << ", pos " << pos << endl;
        if( err )
          break;

        if( pix.n > 0 ) {
          if( pix.x == col && pix.y == row ) {
            ++cnt;
            phsum += pix.p;
          }
        }

      } // trig

      if( err )
        break;

      double ph = -1;
      if( cnt > 0 ) {
        ph = ( double ) phsum / cnt;
      }
      uint32_t idac = dacLower1 + i;
      uint32_t jdac = dacLower2 + j;
      h21->Fill( idac, jdac, ph );
      h22->Fill( idac, jdac, cnt );

      if( cnt > nTrig / 2 ) {
        h23->Fill( idac, jdac, cnt );
      }

    } // dac

    if( err )
      break;

  } // dac

  Log.flush();

  h21->Write();
  h22->Write();
  h23->Write();

  h23->SetStats( 0 );
  h23->Draw( "colz" );
  c1->Update();
  cout << "  histos 21, 22, 23" << endl;

  if( cals && dac1 == 26 && dac2 == 12 ) { // Tornado plot: VthrComp vs CalDel

    int dac1Mean = int ( h23->GetMean( 1 ) );

    int i1 = h23->GetXaxis()->FindBin( dac1Mean );

    cout << "dac " << dac1 << " mean " << dac1Mean
	 << " at bin " << i1 << endl;

    // find efficiency plateau of dac2 at dac1Mean:

    int nm = 0;
    int i0 = 0;
    int i9 = 0;

    for( int j = 0; j < nstp1; ++j ) {

      int cnt = h23->GetBinContent( i1, j+1 ); // ROOT bins start at 1

      h_one->Fill( j, cnt );

      //cout << " " << cnt;
      //Log.printf( "%i %i\n", j, cnt );

      if( cnt > nm ) {
	nm = cnt;
	i0 = j; // begin of plateau
      }
      if( cnt >= nm ) {
	i9 = j; // end of plateau
      }
    }

    cout << "dac " << dac2 << " plateau from " << i0 << " to " << i9 << endl;

    int dac2Best = i0 + ( i9 - i0 ) / 4;

    cout << "set dac " << dac1 << " " << dacName[dac1]
	 << " to mean " << dac1Mean << endl;

    cout << "set dac " << dac2 << " " << dacName[dac2]
	 << " to best " << dac2Best << endl;

    tb.SetDAC( dac1, dac1Mean );
    tb.SetDAC( dac2, dac2Best );
    dacval[0][dac1] = dac1Mean;
    dacval[0][dac2] = dac2Best;

    Log.printf( "[SETDAC] %i  %i\n", dac1, dac1Mean );
    Log.printf( "[SETDAC] %i  %i\n", dac2, dac2Best );

    Log.printf( "dac2 %i Plateau begin %i end %i\n", dac2, i0, i9 );

    h_one->Write();
    delete h_one;

  } // Tornado

  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "  test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;

  return true;

} // dacdac

//------------------------------------------------------------------------------
CMD_PROC( dacdac ) // LoopSingleRocOnePixelDacDacScan: 
// dacdac 22 33 26 12 0 = N vs CalDel and VthrComp = tornado
// dacdac 22 33 26 25 0 = N vs CalDel and Vcal = timewalk
// dacdac 22 33 26 12 = cals N vs CalDel and VthrComp = tornado, set VthrComp
{
  if( ierror ) return false;

  if( tb.TBM_Present() ) {
    cout << "cannot be used for modules" << endl;
    return false;
  }

  int col, row;
  PAR_INT( col, 0, 51 );
  PAR_INT( row, 0, 79 );
  int dac1;
  PAR_INT( dac1, 1, 32 ); // only DACs, not registers
  int dac2;
  PAR_INT( dac2, 1, 32 ); // only DACs, not registers
  int cals;
  if( !PAR_IS_INT( cals, 0, 1 ) )
    cals = 1;
  if( ! HV ) cals = 0;
  int nTrig;
  if( !PAR_IS_INT( nTrig, 1, 1000 ) )
    nTrig = 10;

  cout << "cals " << cals << endl;

  if( dacval[0][dac1] == -1 ) {
    cout << "DAC " << dac1 << " not active" << endl;
    return false;
  }
  if( dacval[0][dac2] == -1 ) {
    cout << "DAC " << dac2 << " not active" << endl;
    return false;
  }

  dacdac( col, row, dac1, dac2, cals, nTrig );

  return true;

} // dacdac

//------------------------------------------------------------------------------
/* read back reg 255:
   0000 I2C data
   0001 I2C address
   0010 I2C pixel column
   0011 I2C pixel row

   1000 VD unreg
   1001 VA unreg
   1010 VA reg
   1100 IA
*/
CMD_PROC( readback )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  int i;

#ifdef DAQOPENCLOSE
  tb.Daq_Open( Blocksize );
#endif
  tb.Pg_Stop();
  tb.Pg_SetCmd( 0, PG_TOK );
  tb.Daq_Select_Deser160( tbState.GetDeserPhase() );

  // take data:

  tb.Daq_Start();
  for( i = 0; i < 36; ++i ) {
    tb.Pg_Single();
    tb.uDelay( 20 );
  }
  tb.Daq_Stop();

  // read out data
  vector < uint16_t > data;
  tb.Daq_Read( data, 40 );
#ifdef DAQOPENCLOSE
  tb.Daq_Close();
#endif

  //analyze data
  int pos = 0;
  PixelReadoutData pix;
  int err = 0;

  unsigned int value = 0;

  // find start bit
  do {
    err = DecodePixel( data, pos, pix );
    if( err )
      break;
  } while( ( pix.hdr & 2 ) == 0 );

  // read data
  for( i = 0; i < 16; ++i ) {
    err = DecodePixel( data, pos, pix );
    if( err )
      break;
    value = ( value << 1 ) + ( pix.hdr & 1 );
  }

  int roc = ( value >> 12 ) & 0xf;
  int cmd = ( value >> 8 ) & 0xf;
  int x = value & 0xff;

  printf( "%04X: roc[%i].", value, roc );
  switch ( cmd ) {
  case 0:
    printf( "I2C_data = %02X\n", x );
    break;
  case 1:
    printf( "I2C_addr = %02X\n", x );
    break;
  case 2:
    printf( "col = %02X\n", x );
    break;
  case 3:
    printf( "row = %02X\n", x );
    break;
  case 8:
    printf( "VD_unreg = %02X\n", x );
    break;
  case 9:
    printf( "VA_unreg = %02X\n", x );
    break;
  case 10:
    printf( "VA_reg = %02X\n", x );
    break;
  case 12:
    printf( "IA = %02X\n", x );
    break;
  default:
    printf( "? = %04X\n", value );
    break;
  }

  return true;

} // readback

//------------------------------------------------------------------------------
CMD_PROC( oneroc ) // single ROC test
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  int roc = 0;

  int target = 30; // [IA [mA]
  if( !setia( target ) ) return 0;

  int ctl = dacval[roc][CtrlReg];

  // caldel at large Vcal:

  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  dacval[roc][CtrlReg] = 4;

  int nTrig = 100;
  bool ok = 0;
  unsigned int col = 26;
  unsigned int row = 40;
  unsigned int ntry = 0;
  while( !ok && ntry < 20 ) {
    ok = setcaldel( roc, col, row, nTrig );
    col = (col+1) % 50 + 1; // 28, 30, 32..50, 2, 4, 
    row = (row+1) % 78 + 1; // 42, 44, 46..78, 2, 4, 
    ++ntry;
  }
  if( !ok ) return 0;

  // response map at large Vcal:

  geteffmap( nTrig );

  // threshold at small Vcal:

  tb.SetDAC( CtrlReg, 0 ); // small Vcal
  dacval[roc][CtrlReg] = 0;
  int stp = 1;
  int dac = Vcal;
  ntry = 0;
  int vthr = dacval[roc][VthrComp];
  int thr = effvsdac( col, row, dac, stp, nTrig, roc );
  target = 50; // Vcal threshold [DAC]

  while( abs(thr-target) > 5 && ntry < 11 && vthr > 5 && vthr < 250 ) {

    if( thr > target )
      vthr += 5;
    else
      vthr -= 5;

    tb.SetDAC( VthrComp, vthr );

    dacval[roc][VthrComp] = vthr;

    ok = setcaldel( roc, col, row, nTrig ); // update CalDel

    thr = effvsdac( col, row, dac, stp, nTrig, roc );

    ++ntry;

  } // vthr

  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  dacval[roc][CtrlReg] = 4;

  tunePH( col, row, roc ); // opt PHoffset, PHscale into ADC range

  // S-curves, noise, gain, cal:

  tb.SetDAC( CtrlReg, 0 ); // small Vcal
  dacval[roc][CtrlReg] = 0;
  dacscanroc( 25, 16, 1, 127 ); // dac nTrig step stop

  tb.SetDAC( CtrlReg, ctl ); // restore
  dacval[roc][CtrlReg] = ctl;
  tb.Flush();

  return ok;

} // oneroc

//------------------------------------------------------------------------------
CMD_PROC( bare ) // single ROC bare module test, without or with Hansen bump height test
{
  int Hansen;
  if( !PAR_IS_INT( Hansen, 0, 1 ) )
    Hansen = 1;

  int roc = 0;

  int target = 30; // IA [mA]
  if( !setia( target ) ) return 0;

  int ctl = dacval[roc][CtrlReg];

  // caldel at large Vcal:

  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  dacval[roc][CtrlReg] = 4;

  int nTrig = 100;
  bool ok = 0;
  unsigned int col = 26;
  unsigned int row = 40;
  unsigned int ntry = 0;

  while( !ok && ntry < 20 ) {

    ok = setcaldel( roc, col, row, nTrig );

    // % has higher precedence than +
    col = (col+1) % 50 + 1; // 28, 30, 32..50, 2, 4, 
    row = (row+1) % 78 + 1; // 42, 44, 46..78, 2, 4, 
    ++ntry;
  }

  if( !ok ) return 0;

  // response map at large Vcal:

  geteffmap( nTrig );

  // threshold at small Vcal:

  tb.SetDAC( CtrlReg, 0 ); // small Vcal
  dacval[roc][CtrlReg] = 0;
  int stp = 1;
  int dac = Vcal;
  ntry = 0;
  int vthr = dacval[roc][VthrComp];
  int thr = effvsdac( col, row, dac, stp, nTrig, roc );
  target = 50; // Vcal threshold [DAC]

  while( abs(thr-target) > 5 && ntry < 11 && vthr > 5 && vthr < 250 ) {

    if( thr > target )
      vthr += 5;
    else
      vthr -= 5;

    tb.SetDAC( VthrComp, vthr );

    dacval[roc][VthrComp] = vthr;

    ok = setcaldel( roc, col, row, nTrig ); // update CalDel

    thr = effvsdac( col, row, dac, stp, nTrig, roc );

    ++ntry;

  } // vthr

  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  dacval[roc][CtrlReg] = 4;

  tunePH( col, row, roc ); // opt PHoffset, PHscale into ADC range

  // S-curves, noise, gain, cal:
  if( Hansen ) {
    tb.SetDAC( CtrlReg, 0 ); // small Vcal
    dacval[roc][CtrlReg] = 0;
    cout << "[S-curves]" << endl;
    dacscanroc( 25, 25, 1, 127 ); // dac nTrig step stop
  }

  // cals:

  tb.SetDAC( CtrlReg, 4 ); // large Vcal
  dacval[roc][CtrlReg] = 4;

  dacdac( col, row, 26, 12 ); // sets CalDel and VthrComp for cals

  if( Hansen ) {
    cout << "[Hansen Test]" << endl;
    dacscanroc( 25, -25, 2, 255 ); // for bump height
  }
  cout << "[Bump Bond Test]" << endl;
  dacscanroc( 12, -10, 1, 255 ); // cals, scan VthrComp, bump bond test

  tb.SetDAC( CtrlReg, ctl ); // restore
  dacval[roc][CtrlReg] = ctl;
  tb.Flush();

  return ok;

} // bare

//------------------------------------------------------------------------------
CMD_PROC( mt ) // module test
{
  int hub;
  if( !PAR_IS_INT( hub, 0, 31 ) )
    hub = 31; // default

  int layer;
  if( !PAR_IS_INT( layer, 1, 4 ) )
    layer = 1; // default

  int BB; // flag for bump bond test (90s)
  if( !PAR_IS_INT( BB, 0, 1 ) )
    BB = 0; // default off

  Log.section( "MODULE_TEST", false );
  Log.printf( "Hub ID %i layer %i\n", hub, layer );
  cout << "Module test for hub " << hub << " in layer " << layer << endl;

  timeval tv;
  gettimeofday( &tv, NULL );
  long s0 = tv.tv_sec; // seconds since 1.1.1970
  long u0 = tv.tv_usec; // microseconds

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // initialize:

  // DTB:

  int vd = 2900; // digital supply from DTB [mV]
  tb._SetVD( vd );
  Log.printf( "[SETVD] %i mV\n", vd );
  dacval[0][VDx] = vd;

  int va = 1900; // analog supply from DTB [mV]
  tb._SetVA( va );
  Log.printf( "[SETVA] %i mV\n", va );
  dacval[0][VAx] = va;

  int idmax = 900; // [mA]
  tb._SetID( idmax * 10 ); // internal unit is 0.1 mA

  int iamax = 600; // [mA]
  tb._SetIA( iamax * 10 ); // internal unit is 0.1 mA

  tb.Flush();

  int ns = 4; // [ns]
  int duty = 0;
  tb.Sig_SetDelay( SIG_CLK, ns +  0, duty );
  tb.Sig_SetDelay( SIG_CTR, ns +  0, duty );
  tb.Sig_SetDelay( SIG_SDA, ns + 15, duty );
  tbState.SetClockPhase( ns );

  int lvl = 13;
  tb.Sig_SetLevel( SIG_CLK, lvl );
  tb.Sig_SetLevel( SIG_SDA, lvl );
  tb.Sig_SetLevel( SIG_CTR, lvl );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // LV power on:
  
  tb.Pon(); // VA, VD power on
  tb.ResetOff();
  tb.mDelay( 500 ); // [ms]

  tb.Flush();

  double vd0 = tb.GetVD();
  printf( "VD %1.3f V\n", vd0 );
  Log.printf( "VD %1.3f V\n", vd0 );

  double va0 = tb.GetVA();
  printf( "VA %1.3f V\n", va0 );
  Log.printf( "VA %1.3f V\n", va0 );

  double id0 = tb.GetID() * 1E3; // [mA]
  printf( "ID %1.1f mA\n", id0 );
  Log.printf( "ID %1.1f mA\n", id0 );

  double ia0 = tb.GetIA() * 1E3; // [mA]
  printf( "IA %1.1f mA\n", ia0 );
  Log.printf( "IA %1.1f mA\n", ia0 );

  if( ia0 < 10 ) {
    tb.Poff();
    tb.Flush();
    Log.printf( "[POFF]\n" );
    cout << "low voltage power off" << endl;
    cout << "exit or quit to close the ROOT file and the DTB" << endl;
    return 0;
  }

  cout << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // TBM:

  tb.tbm_Enable( true );
  if( layer == 1 )
    tb.mod_Addr( hub, hub-1 ); // sets FPGA L1 flag
  else
    tb.mod_Addr( hub );

  if( layer == 1 )
    tb.roc_I2cAddr( 0 ); // TBM 1 talks to ROCs 12-15 and 0-3

  tb.tbm_Set( 0xE4, 0b11110000 ); // init TBM core A
  tb.tbm_Set( 0xF4, 0b11110000 ); // init TBM core B

  tb.tbm_Set( 0xE0, 0b10000001 ); // Disable Auto Reset, Disable PKAM Counter
  tb.tbm_Set( 0xF0, 0b10000001 ); // 

  tb.tbm_Set( 0xE2, 0b11000000 ); // Mode: Calibration
  tb.tbm_Set( 0xF2, 0b11000000 ); // Mode: Calibration

  tb.tbm_Set( 0xE8, 9 ); // PKAM (x+1)*6.4 us
  tb.tbm_Set( 0xF8, 9 ); // PKAM (x+1)*6.4 us

  tb.tbm_Set( 0xEC, 99 ); // Auto reset rate (x+1)*256
  tb.tbm_Set( 0xFC, 99 ); // 

  if( layer == 1 ) {
    tb.tbm_Set( 0xEA, 0b11100100 ); // 1+1+3+3 = del_tin  del_Hdr/Trl  del_port1  del_port0  12-15
    tb.tbm_Set( 0xFA, 0b11100100 ); // 1+1+3+3 = del_tin  del_Hdr/Trl  del_port3  del_port2   0- 3
  }
  else {
    tb.tbm_Set( 0xEA, 0b01101101 ); // 1+1+3+3 = del_tin  del_Hdr/Trl  del_port1  del_port0   0- 7
    tb.tbm_Set( 0xFA, 0b01101101 ); // 1+1+3+3 = del_tin  del_Hdr/Trl  del_port3  del_port2   8-15
  }

  if( layer == 1 )
    tb.tbm_Set( 0xEE, 0b00001000 ); // 00+3+3 = 160 + 400 MHz phase
  else
    tb.tbm_Set( 0xEE, 0b00110100 ); // 00+3+3 = 160 + 400 MHz phase
  tb.tbm_Set( 0xFE, 0 ); // Temp measurement control

  if( layer == 1 ) {
    tb.roc_I2cAddr( 4 ); // TBM 0 talks to ROCs 4-7 and 8-11

    tb.tbm_Set( 0xE4, 0b11110000 ); // init TBM core A
    tb.tbm_Set( 0xF4, 0b11110000 ); // init TBM core B

    tb.tbm_Set( 0xE0, 0b10000001 ); // Disable Auto Reset, Disable PKAM Counter
    tb.tbm_Set( 0xF0, 0b10000001 ); // 

    tb.tbm_Set( 0xE2, 0b11000000 ); // Mode: Calibration
    tb.tbm_Set( 0xF2, 0b11000000 ); // Mode: Calibration

    tb.tbm_Set( 0xE8, 9 ); // PKAM (x+1)*6.4 us
    tb.tbm_Set( 0xF8, 9 ); // PKAM (x+1)*6.4 us

    tb.tbm_Set( 0xEC, 99 ); // Auto reset rate (x+1)*256
    tb.tbm_Set( 0xFC, 99 ); // 

    tb.tbm_Set( 0xEA, 0b11100100 ); // 1+1+3+3 = del_tin  del_Hdr/Trl  del_port1  del_port0   4- 7
    tb.tbm_Set( 0xFA, 0b11100100 ); // 1+1+3+3 = del_tin  del_Hdr/Trl  del_port3  del_port2   8-11

    tb.tbm_Set( 0xEE, 0b00001000 ); // 00+3+3 = 160 + 400 MHz phase

    tb.tbm_Set( 0xFE, 0 ); // Temp measurement control
  }
  
  tb.Daq_Deser400_OldFormat( false ); // DESY has TBM08b/a

  tb.invertAddress = 0;

  tb.Flush();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // module:

  L1 = 0;
  L2 = 0;
  L4 = 0;

  if( layer == 1 ) {
    L1 = 1;
    nChannels = 8;
    Chip = 700; // PROC
    tb.linearAddress = 1;
  }
  else if( layer == 2 ) {
    L2 = 1;
    nChannels = 4;
    Chip = 500; // respin
    tb.linearAddress = 0;
  }
  else {
    L4 = 1;
    nChannels = 2;
    Chip = 500; // respin
    tb.linearAddress = 0;
  }

  int buffersize = 40/nChannels*1000*1000; // words = 2 bytes
  tbState.SetDaqSize( buffersize );
  for( int ch = 0; ch < nChannels; ++ch ) {
    int allocated = tb.Daq_Open( buffersize, ch );
    cout << "DAQ channel " << ch << " allocated " << allocated/1E6 << "M words in RAM" << endl;
    tb.Daq_Stop( ch );
  }
  tbState.SetDaqOpen( 1 );

  haveGain = 0;

  cout << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // ROCs:

  for( int roc = 0; roc < 16; ++roc )
    roclist[roc] = 1;

  dacName[1] = "Vdig";
  dacName[2] = "Vana";
  if( Chip < 400 )
    dacName[3] = "Vsf ";
  else if( Chip < 700 )
    dacName[3] = "Vsh "; // digV2.1
  else
    dacName[3] = "Iph "; // PROC
  dacName[4] = "Vcomp";

  if( Chip < 200 )
    dacName[5] = "Vleak_comp"; // only analog
  if( Chip < 200 )
    dacName[6] = "VrgPr"; // removed on dig
  dacName[7] = "VwllPr";
  if( Chip < 200 )
    dacName[8] = "VrgSh"; // removed on dig
  dacName[9] = "VwllSh";

  if( Chip < 700 )
    dacName[10] = "VhldDel";
  dacName[11] = "Vtrim";
  dacName[12] = "VthrComp";

  if( Chip < 700 )
    dacName[13] = "VIBias_Bus";
  else
    dacName[13] = "VColOr";
  if( Chip < 700 )
    dacName[14] = "Vbias_sf";

  if( Chip < 400 )
    dacName[15] = "VoffsetOp";
  if( Chip < 200 )
    dacName[16] = "VIbiasOp"; // analog
  if( Chip < 400 )
    dacName[17] = "VoffsetRO"; //
  else
    dacName[17] = "PHOffset"; // digV2.1
  if( Chip < 400 )
    dacName[18] = "VIon";

  dacName[19] = "Vcomp_ADC"; // dig
  if( Chip < 400 )
    dacName[20] = "VIref_ADC"; // dig
  else
    dacName[20] = "PHScale"; // digV2.1
  if( Chip < 200 )
    dacName[21] = "VIbias_roc"; // analog
  if( Chip < 700 )
    dacName[22] = "VIColOr";

  dacName[25] = "VCal";
  dacName[26] = "CalDel";

  dacName[31] = "VD  "; // space for nice tabulator print out
  dacName[32] = "VA  ";

  dacName[253] = "CtrlReg";
  dacName[254] = "WBC ";
  dacName[255] = "RBReg";

  // for L2 and L1:

  if( Chip < 700 )
    SetDACallROCs(   1,   8 ); // Vdig respin
  else
    SetDACallROCs(   1,  12 ); // Vdig PROC
  SetDACallROCs(   2, 111 ); // Vana for 30 mA
  if( Chip < 700 )
    SetDACallROCs(   3,  30 ); // Vsh respin
  else
    SetDACallROCs(   3,   8 ); // Iph 4 bit PROC
  SetDACallROCs(   4,  12 ); // Vcomp

  SetDACallROCs(   7, 160 ); // VwllPr
  SetDACallROCs(   9, 160 ); // VwllSh

  SetDACallROCs(  11,   1 ); // Vtrim
  SetDACallROCs(  12,  90 ); // VthrComp

  SetDACallROCs(  13,  30 ); // VIBias_Bus / VColOr
  if( Chip < 700 )
    SetDACallROCs(  22,  99 ); // VIColOr

  SetDACallROCs(  17, 166 ); // PHOffset
  SetDACallROCs(  20,  99 ); // PHScale
  SetDACallROCs(  19,  10 ); // VcompADC

  SetDACallROCs(  25, 255 ); // Vcal, max for BB test
  SetDACallROCs(  26, 122 ); // CalDel

  if( Chip < 700 )
    SetDACallROCs( 253,   4 ); // CtrlReg, 4 for large Vcal
  else
    SetDACallROCs( 253,   5 ); // PROC CtrlReg, 5 for large Vcal, disable trg out
  SetDACallROCs( 254, 100 ); // WBC
  SetDACallROCs( 255,  12 ); // RBreg

  tb.Flush();

  for( int roc = 0; roc < 16; ++roc )
    if( roclist[roc] ) {
      tb.roc_I2cAddr( roc );
      tb.roc_ClrCal();
      tb.roc_Chip_Mask();
    }

  tb.Flush();

  cout << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // HV:

  tb.HVon(); // close HV relais on DTB
  Log.printf( "[HVON]\n" );
  cout << "HV on" << endl;
  HV = 1;
  tb.mDelay( 500 ); // [ms]
  tb.Flush();

  double vd1 = tb.GetVD();
  printf( "VD %1.3f V\n", vd1 );
  Log.printf( "VD %1.3f V\n", vd1 );

  double va1 = tb.GetVA();
  printf( "VA %1.3f V\n", va1 );
  Log.printf( "VA %1.3f V\n", va1 );

  double id1 = tb.GetID() * 1E3; // [mA]
  printf( "ID %1.1f mA\n", id1 );
  Log.printf( "ID %1.1f mA\n", id1 );

  double ia1 = tb.GetIA() * 1E3; // [mA]
  printf( "IA %1.1f mA\n", ia1 );
  Log.printf( "IA %1.1f mA\n", ia1 );

  if( ia1 < 200 ) {
    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Close( ch );
    tb.HVoff(); // open HV relais on DTB
    tb.Flush();
    Log.printf( "[HVOFF]\n" );
    cout << "HV off" << endl;
    HV = 0;
    tb.Poff();
    tb.Flush();
    Log.printf( "[POFF]\n" );
    cout << "low voltage power off" << endl;
    cout << "exit or quit to close the ROOT file and the DTB" << endl;
    return 0;
  }

  cout << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // test pulse pattern generator:

  tb.Pg_Stop();
  int ipos = 0; // address in PG
  int bits = 0b011000; // reset TBM, reset ROC
  int delay = 16; // [BC]
  tb.Pg_SetCmd( ipos++, ( bits << 8 ) + delay );

  bits = 0b000100; // cal
  delay = 106; // WBC + 6 for respin
  tb.Pg_SetCmd( ipos++, ( bits << 8 ) + delay );

  bits = 0b000010; // trigger
  delay = 0; // last
  tb.Pg_SetCmd( ipos++, ( bits << 8 ) + delay );

  cout << "filled " << ipos << " PG lines" << endl;

  tb.SetLoopTriggerDelay( 400 ); // [BC], 400 = 10 us

  tb.Pg_Single();
  tb.Flush();

  cout << endl;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // adjust Vana for IA:

  if( BB ) {

    int target = 30; // IA [mA] for BB test
    if( !setiamod( target ) ) {
      for( int ch = 0; ch < nChannels; ++ch )
	tb.Daq_Close( ch );
      tb.HVoff(); // open HV relais on DTB
      tb.Flush();
      Log.printf( "[HVOFF]\n" );
      cout << "HV off" << endl;
      HV = 0;
      tb.Poff();
      tb.Flush();
      Log.printf( "[POFF]\n" );
      cout << "low voltage power off" << endl;
      cout << "exit or quit to close the ROOT file and the DTB" << endl;
      return 0;
    }

    cout << endl;

    double va2 = tb.GetVA();
    printf( "VA %1.3f V\n", va2 );
    Log.printf( "VA %1.3f V\n", va2 );

    double ia2 = tb.GetIA() * 1E3; // [mA]
    printf( "IA %1.1f mA\n", ia2 );
    Log.printf( "IA %1.1f mA\n", ia2 );

    if( ia2 < 300 ) {
      for( int ch = 0; ch < nChannels; ++ch )
	tb.Daq_Close( ch );
      tb.HVoff(); // open HV relais on DTB
      tb.Flush();
      Log.printf( "[HVOFF]\n" );
      cout << "HV off" << endl;
      HV = 0;
      tb.Poff();
      tb.Flush();
      Log.printf( "[POFF]\n" );
      cout << "low voltage power off" << endl;
      cout << "exit or quit to close the ROOT file and the DTB" << endl;
      return 0;
    }

  } // BB wanted

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // caldel at large Vcal:

  int nTrig = 16;
  unsigned int col = 22;
  unsigned int row = 33;

  int mm[16]; // responses per ROC

  for( size_t roc = 0; roc < 16; ++roc ) {

    if( roclist[roc] == 0 )
      continue;

    //select the ROC:
    tb.roc_I2cAddr( roc );

    int ctl = dacval[roc][CtrlReg];

    tb.SetDAC( CtrlReg, 4 ); // large Vcal
    dacval[roc][CtrlReg] = 4;

    // scan caldel:

    bool ok = setcaldel( roc, col, row, nTrig );
    if( ok )
      mm[roc] = h11->GetMaximum();
    else
      mm[roc] = 0;

    tb.SetDAC( CtrlReg, ctl ); // restore
    tb.Flush();
    dacval[roc][CtrlReg] = ctl;

  } // rocs

  cout << endl;
  bool ok = 1;
  for( int roc = 0; roc < 16; ++roc ) {
    cout << setw( 2 ) << roc << " CalDel " << setw( 3 ) << dacval[roc][CalDel]
	 << " plateau height " << mm[roc]
	 << endl;
    if( mm[roc] < nTrig/2 )
      ok = 0;
  }

  if( !ok ) {
    for( int ch = 0; ch < nChannels; ++ch )
      tb.Daq_Close( ch );
    tb.HVoff(); // open HV relais on DTB
    tb.Flush();
    Log.printf( "[HVOFF]\n" );
    cout << "HV off" << endl;
    HV = 0;
    tb.Poff();
    tb.Flush();
    Log.printf( "[POFF]\n" );
    cout << "low voltage power off" << endl;
    cout << "exit or quit to close the ROOT file and the DTB" << endl;
    return 0;
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // response map at large Vcal

  ok = ModMap( 20 );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // BB test:

  if( BB ) {
  
    cout << endl << "Bump bond test:" << endl;

    //ok = DacScanMod( 12, -10, 2, 255, 80 ); // scan VthrComp with Cals, 5 loops, 143s
  //              dac nTrg step stop strt
    ok = DacScanMod( 12, -10, 4, 255, 80 ); // scan VthrComp with Cals, 5 loops, 74s
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // done:

  tb.HVoff(); // open HV relais on DTB
  tb.Flush();
  Log.printf( "[HVOFF]\n" );
  cout << "HV off" << endl;
  HV = 0;

  for( int ch = 0; ch < nChannels; ++ch )
    tb.Daq_Close( ch );
  tb.Poff();
  tb.Flush();
  Log.printf( "[POFF]\n" );
  cout << "low voltage power off" << endl;
  
  gettimeofday( &tv, NULL );
  long s9 = tv.tv_sec; // seconds since 1.1.1970
  long u9 = tv.tv_usec; // microseconds
  cout << "total test duration " << s9 - s0 + ( u9 - u0 ) * 1e-6 << " s" << endl;
  cout << "exit or quit to close the ROOT file and the DTB" << endl;

  return ok;

} // mt

//------------------------------------------------------------------------------
void cmdHelp()
{
  fputs( "+-cmd commands --------------+\n"
         "| help    list of commands   |\n"
         "| exit    exit commander     |\n"
         "| quit    quit commander     |\n"
         "+----------------------------+\n"
	 , stdout );
}

//------------------------------------------------------------------------------
CMD_PROC( h )
{
  int dummy; if( !PAR_IS_INT( dummy, 0, 1 ) ) dummy = 80;
  cmdHelp();
  return true;
}

//------------------------------------------------------------------------------
void cmd()                    // called once from psi46test
{
  CMD_REG( showtb,    "showtb                        print DTB state" );
  CMD_REG( showhv,    "show hv                       status of iseg HV supply" );
  CMD_REG( scan,      "scan                          enumerate USB devices" );
  CMD_REG( open,      "open <name>                   open connection to testboard" );
  CMD_REG( close,     "close                         close connection to testboard" );
  CMD_REG( welcome,   "welcome                       blinks with LEDs" );
  CMD_REG( setled,    "setled <bits>                 set atb LEDs" );
  CMD_REG( log,       "log <text>                    writes text to log file" );
  CMD_REG( upgrade,   "upgrade <filename>            upgrade DTB" );
  CMD_REG( rpcinfo,   "rpcinfo                       lists DTB functions" );
  CMD_REG( version,   "version                       shows DTB software version" );
  CMD_REG( info,      "info                          shows detailed DTB info" );
  CMD_REG( boardid,   "boardid                       get board id" );
  CMD_REG( init,      "init                          inits the testboard" );
  CMD_REG( flush,     "flush                         flushes usb buffer" );
  CMD_REG( clear,     "clear                         clears usb data buffer" );

  CMD_REG( udelay,    "udelay <us>                   waits <us> microseconds" );
  CMD_REG( mdelay,    "mdelay <ms>                   waits <ms> milliseconds" );

  CMD_REG( d1,        "d1 <signal>                   assign signal to D1 output" );
  CMD_REG( d2,        "d2 <signal>                   assign signal to D2 outout" );
  CMD_REG( a1,        "a1 <signal>                   assign analog signal to A1 output" );
  CMD_REG( a2,        "a2 <signal>                   assign analog signal to A2 outout" );
  CMD_REG( probeadc,  "probeadc <signal>             assign analog signal to ADC" );

  CMD_REG( clksrc,    "clksrc <source>               Select clock source" );
  CMD_REG( clkok,     "clkok                         Check if ext clock is present" );
  CMD_REG( fsel,      "fsel <freqdiv>                clock frequency select" );
  CMD_REG( stretch,   "stretch <src> <delay> <width> stretch clock" );

  CMD_REG( clk,       "clk <delay>                   clk delay" );
  CMD_REG( ctr,       "ctr <delay>                   ctr delay" );
  CMD_REG( sda,       "sda <delay>                   sda delay" );
  CMD_REG( tin,       "tin <delay>                   tin delay" );
  CMD_REG( rda,       "rda <delay>                   tin delay" );
  CMD_REG( clklvl,    "clklvl <level>                clk signal level" );
  CMD_REG( ctrlvl,    "ctrlvl <level>                ctr signel level" );
  CMD_REG( sdalvl,    "sdalvl <level>                sda signel level" );
  CMD_REG( tinlvl,    "tinlvl <level>                tin signel level" );
  CMD_REG( clkmode,   "clkmode <mode>                clk mode" );
  CMD_REG( ctrmode,   "ctrmode <mode>                ctr mode" );
  CMD_REG( sdamode,   "sdamode <mode>                sda mode" );
  CMD_REG( tinmode,   "tinmode <mode>                tin mode" );

  CMD_REG( sigoffset, "sigoffset <offset>            output signal offset" );
  CMD_REG( lvds,      "lvds                          LVDS inputs" );
  CMD_REG( lcds,      "lcds                          LCDS inputs" );

  CMD_REG( pon,       "pon                           switch ROC power on" );
  CMD_REG( poff,      "poff                          switch ROC power off" );
  CMD_REG( va,        "va <mV>                       set VA supply in mV" );
  CMD_REG( vd,        "vd <mV>                       set VD supply in mV" );
  CMD_REG( ia,        "ia <mA>                       set IA limit in mA" );
  CMD_REG( id,        "id <mA>                       set ID limit in mA" );

  CMD_REG( getva,     "getva                         set VA in V" );
  CMD_REG( getvd,     "getvd                         set VD in V" );
  CMD_REG( getia,     "getia                         get IA in mA" );
  CMD_REG( getid,     "getid                         get ID in mA" );
  CMD_REG( optia,     "optia <ia>                    set Vana to desired ROC Ia [mA]" );
  CMD_REG( optiamod,  "optiamod <ia>                 set Vana to desired ROC Ia [mA] for module" );

  CMD_REG( hvon,      "hvon                          switch HV on" );
  CMD_REG( hvoff,     "hvoff                         switch HV off" );
  CMD_REG( vb,        "vb <V>                        set -Vbias in V" );
  CMD_REG( getvb,     "getvb                         measure bias voltage" );
  CMD_REG( getib,     "getib                         measure bias current" );
  CMD_REG( scanvb,    "scanvb vmax [vstp]            bias voltage scan" );
  CMD_REG( ibvst,     "ibvst                         bias current vs time, any key to stop" );

  CMD_REG( reson,     "reson                         activate reset" );
  CMD_REG( resoff,    "resoff                        deactivate reset" );
  CMD_REG( rocaddr,   "rocaddr                       set ROC address" );
  CMD_REG( rowinvert, "rowinvert                     invert row address psi46dig" );
  CMD_REG( chip,      "chip num                      set chip number" );
  CMD_REG( module,    "module num                    set module number" );

  CMD_REG( pgset,     "pgset <addr> <bits> <delay>   set pattern generator entry" );
  CMD_REG( pgstop,    "pgstop                        stops pattern generator" );
  CMD_REG( pgsingle,  "pgsingle                      send single pattern" );
  CMD_REG( pgtrig,    "pgtrig                        enable external pattern trigger" );
  CMD_REG( pgloop,    "pgloop <period>               start patterngenerator in loop mode" );
  CMD_REG( trigdel,   "trigdel <delay>               delay in trigger loop [BC]" );

  CMD_REG( trgsel,    "trgsel bitmask                trigger mode, 16=PG" );
  CMD_REG( trgdel,    "trgdelay delay                trigger delay" );
  CMD_REG( trgtimeout,"trgtimeout                    trigger token timeout" );
  CMD_REG( trgper,    "trgper period                 trigger period" );
  CMD_REG( trgrand,   "trgrand rate                  random trigger rate" );
  CMD_REG( trgsend,   "trgsend bitmask               send trigger" );

  CMD_REG( upd,       "upd <histo>                  re-draw ROOT histo in canvas" );

  CMD_REG( dopen,     "dopen <buffer size> [<ch>]    Open DAQ and allocate memory" );
  CMD_REG( dsel,      "dsel <MHz>                    select deserializer 160 or 400 MHz" );
  CMD_REG( dreset,    "dreset <reset>                DESER400 reset 1, 2, or 3" );
  CMD_REG( dclose,    "dclose [<channel>]            Close DAQ" );
  CMD_REG( dstart,    "dstart [<channel>]            Enable DAQ" );
  CMD_REG( dstop,     "dstop [<channel>]             Disable DAQ" );
  CMD_REG( dsize,     "dsize [<channel>]             Show DAQ buffer fill state" );
  CMD_REG( dread,     "dread                         Read Daq buffer and show as ROC data" );
  CMD_REG( dreadm,    "dreadm [channel]              Read Daq buffer and show as module data" );

  CMD_REG( showclk,   "showclk                       show CLK signal" );
  CMD_REG( showctr,   "showctr                       show CTR signal" );
  CMD_REG( showsda,   "showsda                       show SDA signal" );

  CMD_REG( tbmdis,    "tbmdis                        disable TBM" );
  CMD_REG( tbmsel,    "tbmsel <hub> <port>           set hub and port address, port 6=all" );
  CMD_REG( modsel,    "modsel <hub>                  set hub address for module" );
  CMD_REG( modsel2,   "modsel2 <hub>                 set 2 hub address for L1 module" );
  CMD_REG( tbmset,    "tbmset <reg> <value>          set TBM register" );
  CMD_REG( tbmget,    "tbmget <reg>                  read TBM register" );
  CMD_REG( tbmgetraw, "tbmgetraw <reg>               read TBM register" );
  CMD_REG( tbmscan,   "tbmscan                       scan TBM delays against RO err" );

  CMD_REG( readback,  "readback                      read out ROC data" );

  CMD_REG( dselmod,   "dselmod                       select deser400 for DAQ channel 0" );
  CMD_REG( dmodres,   "dmodres                       reset all deser400" );
  CMD_REG( dselroca,  "dselroca value                select adc for channel 0" );
  CMD_REG( dseloff,   "dseloff                       deselect all" );
  CMD_REG( deser160,  "deser160                      align deser160" );
  CMD_REG( deserext,  "deserext                      scan deser160 ext trig" );
  CMD_REG( deser,     "deser value                   controls deser160" );

  CMD_REG( select,    "select addr:range             set i2c address" );
  CMD_REG( seltbm,    "seltbm 0/1                    select next TBM" );
  CMD_REG( dac,       "dac address value [roc]       set DAC" );
  CMD_REG( vdig,      "vdig value                    set Vdig" );
  CMD_REG( vana,      "vana value                    set Vana" );
  CMD_REG( vtrim,     "vtrim value                   set Vtrim" );
  CMD_REG( vthr,      "vthr value                    set VthrComp" );
  CMD_REG( subvthr,   "subvthr value                 sutract from VthrComp" );
  CMD_REG( addvtrim,  "addvtrim value                add to Vtrim" );
  CMD_REG( vcal,      "vcal value                    set Vcal" );
  CMD_REG( ctl,       "ctl value                     set control register" );
  CMD_REG( wbc,       "wbc value                     set WBC" );
  CMD_REG( show,      "show [roc]                    print dacs" );
  CMD_REG( show1,     "show1 dac                     print one dac for all ROCs" );
  CMD_REG( wdac,      "wdac [description]            write dacParameters_chip_desc.dat" );
  CMD_REG( rddac,     "rddac [description]           read dacParameters_chip_desc.dat" );
  CMD_REG( wtrim,     "wtrim [description]           write trimParameters_chip_desc.dat" );
  CMD_REG( rdtrim,    "rdtrim [description]          read trimParameters_chip_desc.dat" );
  CMD_REG( setbits,   "setbits bit                   set trim bits" );

  CMD_REG( cole,      "cole range                    enable column" );
  CMD_REG( cold,      "cold roc col                  disable column" );
  CMD_REG( pixi,      "pixi roc col row              show trim bits" );
  CMD_REG( pixt,      "pixt roc col row trim         set trim bits" );
  CMD_REG( pixe,      "pixe range range              enable pixel" );
  CMD_REG( pixd,      "pixd roc cols rows            mask pixel" );
  CMD_REG( pixm,      "pixm roc col row              mask pixel" );
  CMD_REG( cal,       "cal range range               enable calibrate pixel" );
  CMD_REG( arm,       "arm range range [ROC]         activate pixel" );
  CMD_REG( cals,      "cals range range              sensor calibrate pixel" );
  CMD_REG( cald,      "cald                          clear calibrate" );
  CMD_REG( mask,      "mask                          mask all pixel and cols" );
  CMD_REG( fire,      "fire col row [nTrig]          single pulse and read" );
  CMD_REG( fire2,     "fire col row [-][nTrig]       correlation" );
  CMD_REG( takedata,  "takedata period               readout 40 MHz/period (stop: s enter)" );
  CMD_REG( tdscan,    "tdscan vmin vmax              take data vs VthrComp" );
  CMD_REG( onevst,    "onevst col row period         <PH> vs time (stop: s enter)" );

  CMD_REG( daci,      "daci dac                      current vs dac" );
  CMD_REG( vanaroc,   "vanaroc                       ROC efficiency scan vs Vana" );
  CMD_REG( vthrcompi, "vthrcompi roc                 Id vs VthrComp for one ROC" );
  CMD_REG( caldel,    "caldel col row                CalDel efficiency scan" );
  CMD_REG( caldelmap, "caldelmap                     map of CalDel range" );
  CMD_REG( caldelroc, "caldelroc                     ROC CalDel efficiency scan" );

  CMD_REG( vthrcomp5, "vthrcomp5 target              set VthrComp to target Vcal from 5" );
  CMD_REG( vthrcomp,  "vthrcomp target               set VthrComp to target Vcal from all" );
  CMD_REG( trim,      "trim target                   set Vtrim and trim bits" );
  CMD_REG( trimbits,  "trimbits                      set trim bits for efficiency" );
  CMD_REG( thrdac,    "thrdac col row dac            Threshold vs DAC one pixel" );
  CMD_REG( thrmap,    "thrmap                        threshold map trimmed" );
  CMD_REG( thrmapsc,  "thrmapsc stop (4=cals)        threshold map" );
  CMD_REG( scanvthr,  "scanvthr vthrmin vthrmax [vthrstp]    threshold RMS vs VthrComp" );

  CMD_REG( tune,      "tune col row                  tune gain and offset" );
  CMD_REG( phmap,     "phmap nTrig                   ROC PH map" );
  CMD_REG( calsmap,   "calsmap nTrig                 CALS map = bump bond test" );
  CMD_REG( effmap,    "effmap nTrig                  pixel alive map" );
  CMD_REG( bbtest,    "bbtest nTrig                  CALS map = bump bond test" );
  CMD_REG( oneroc,    "oneroc                        single ROC test" );
  CMD_REG( bare,      "bare Hansen                   bare module ROC test" );
  CMD_REG( mt,        "mt hub layer                  module test" );

  CMD_REG( effdac,    "effdac col row dac [stp] [nTrig] [roc]  Efficiency vs DAC one pixel" );
  CMD_REG( phdac,     "phdac col row dac [stp] [nTrig] [roc]  PH vs DAC one pixel" );
  CMD_REG( gaindac,   "gaindac                       calibrated PH vs Vcal" );
  CMD_REG( calsdac,   "calsdac col row dac [ntrig] [roc]  cals vs DAC one pixel" );
  CMD_REG( dacdac,    "dacdac col row dacx dacy [cals] [nTrig] DAC DAC scan" );

  CMD_REG( dacscanroc,"dacscanroc dac [-][nTrig] [step] [stop]  PH vs DAC, all pixels" );

  CMD_REG( modcaldel, "modcaldel col row             CalDel efficiency scan for module" );
  CMD_REG( modpixsc,  "modpixsc col row ntrig        module pixel S-curve" );
  CMD_REG( dacscanmod,"dacscanmod dac [-]ntrig [step] [stop] [strt] module dac scan" );
  CMD_REG( modthrdac, "modthrdac col row dac         Threshold vs DAC one pixel" );
  CMD_REG( modvthrcomp, "modvthrcomp target            set VthrComp on each ROC" );
  CMD_REG( modvtrim,  "modvtrim target               set Vtrim" );
  CMD_REG( modtrimscale, "modtrimscale target          scale trim bits" );
  CMD_REG( modtrimstep, "modtrimstep target          iterate trim bits" );
  CMD_REG( modtrimbits, "modtrimbits cut               adjust noisy trim bits" );
  CMD_REG( subtb,     "subtb                         subtract one trim bit" );
  CMD_REG( modnoise,  "modnoise                      trim bits noise limit" );
  CMD_REG( modtune,   "modtune col row               tune PH gain and offset" );
  CMD_REG( modmap,    "modmap nTrig                  module map" );
  CMD_REG( modthrmap, "modthrmap stop step           module threshold map" );
  CMD_REG( modtd,     "modtd period                  module take data 40MHz/period (stop: s enter)" );

  cmdHelp();

  cmd_intp.SetScriptPath( settings.path );

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  for( size_t iroc = 0; iroc < 16; ++iroc )
    for( size_t idac = 0; idac < 256; ++idac )
      dacval[iroc][idac] = -1;

  // init globals (to something other than the default zero):

  for( int roc = 0; roc < 16; ++roc )
    for( int col = 0; col < 52; ++col )
      for( int row = 0; row < 80; ++row ) {
        modthr[roc][col][row] = 255;
        modtrm[roc][col][row] = 15;
	modcnt[roc][col][row] = 0;
	modamp[roc][col][row] = 0;
      }

  ierror = 0;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // set ROOT styles:

  gStyle->SetTextFont( 62 ); // 62 = Helvetica bold
  gStyle->SetTextAlign( 11 );

  gStyle->SetTickLength( -0.02, "x" ); // tick marks outside
  gStyle->SetTickLength( -0.02, "y" );
  gStyle->SetTickLength( -0.01, "z" );

  gStyle->SetLabelOffset( 0.022, "x" );
  gStyle->SetLabelOffset( 0.022, "y" );
  gStyle->SetLabelOffset( 0.022, "z" );

  gStyle->SetTitleOffset( 1.3, "x" );
  gStyle->SetTitleOffset( 1.6, "y" );
  gStyle->SetTitleOffset( 1.7, "z" );

  gStyle->SetLabelFont( 62, "X" );
  gStyle->SetLabelFont( 62, "Y" );
  gStyle->SetLabelFont( 62, "z" );

  gStyle->SetTitleFont( 62, "X" );
  gStyle->SetTitleFont( 62, "Y" );
  gStyle->SetTitleFont( 62, "z" );

  gStyle->SetTitleBorderSize( 0 ); // no frame around global title
  gStyle->SetTitleAlign( 13 ); // 13 = left top align
  gStyle->SetTitleX( 0.12 ); // global title
  gStyle->SetTitleY( 0.98 ); // global title

  gStyle->SetLineWidth( 1 ); // frames
  gStyle->SetHistLineColor( 4 ); // 4=blau
  gStyle->SetHistLineWidth( 3 );
  gStyle->SetHistFillColor( 5 ); // 5=gelb
  //  gStyle->SetHistFillStyle(4050); // 4050 = half transparent
  gStyle->SetHistFillStyle( 1001 ); // 1001 = solid

  gStyle->SetFrameLineWidth( 2 );

  // statistics box:

  gStyle->SetOptStat( 111111 );
  gStyle->SetStatFormat( "8.6g" ); // more digits, default is 6.4g
  gStyle->SetStatFont( 42 ); // 42 = Helvetica normal
  //  gStyle->SetStatFont(62); // 62 = Helvetica bold
  gStyle->SetStatBorderSize( 1 ); // no 'shadow'

  gStyle->SetStatX( 0.99 );
  gStyle->SetStatY( 0.60 );

  //gStyle->SetPalette( 1 ); // rainbow colors, red on top

  // ROOT View colors Color Wheel:

  int pal[20];

  pal[ 0] = 1;
  pal[ 1] = 922; // gray
  pal[ 2] = 803; // brown
  pal[ 3] = 634; // dark red
  pal[ 4] = 632; // red
  pal[ 5] = 810; // orange
  pal[ 6] = 625;
  pal[ 7] = 895; // dark pink
  pal[ 8] = 614;
  pal[ 9] = 609;
  pal[10] = 616; // magenta
  pal[11] = 593;
  pal[12] = 600; // blue
  pal[13] = 428;
  pal[14] = 423;
  pal[15] = 400; // yellow
  pal[16] = 391;
  pal[17] = 830;
  pal[18] = 416; // green
  pal[19] = 418; // dark green on top

  gStyle->SetPalette( 20, pal );

  gStyle->SetHistMinimumZero(); // no zero suppression

  gStyle->SetOptDate();

  cout << "open ROOT window..." << endl;
  MyMainFrame *myMF = new MyMainFrame( gClient->GetRoot(), 800, 600 );

  myMF->SetWMPosition( 99, 0 );

  cout << "open Canvas..." << endl;
  c1 = myMF->GetCanvas(); // global c1

  c1->SetBottomMargin( 0.15 );
  c1->SetLeftMargin( 0.15 );
  c1->SetRightMargin( 0.18 );

  gPad->Update(); // required

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // command loop:

  while( true ) {

    try {

      CMD_RUN( stdin );

      delete myMF;
      return;
    }
    catch( CRpcError e ) {
      cout << "cmd gets CRpcError" << endl;
      //e.What(); crashes
      /*
        in __strlen_sse42 () from /lib64/libc.so.6
        std::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string(char const*, std::allocator<char> const&) () from /usr/lib64/libstdc++.so.6
        in CRpcError::What (this=0x7fff6c8dfc30) at rpc_error.cpp:36
        in cmd () at cmd.cpp:7714
      */
    }
  }

} // cmd
