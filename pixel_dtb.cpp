
#include "pixel_dtb.h"
#include <stdio.h>

#ifndef _WIN32
#include <unistd.h>
#include <iostream>
#endif

bool CTestboard::EnumNext( string & name )
{
  char s[64];
  if( !usb.EnumNext( s ) )
    return false;
  name = s;
  return true;
}


bool CTestboard::Enum( unsigned int pos, string & name )
{
  char s[64];
  if( !usb.Enum( s, pos ) )
    return false;
  name = s;
  return true;
}


bool CTestboard::FindDTB( string & usbId )
{
  string name;
  vector < string > devList;
  unsigned int nDev;
  unsigned int nr;

  try {
    if( !EnumFirst( nDev ) )
      throw int ( 1 );
    for( nr = 0; nr < nDev; nr++ ) {
      if( !EnumNext( name ) )
        continue;
      if( name.size(  ) < 4 )
        continue;
      if( name.compare( 0, 4, "DTB_" ) == 0 )
        devList.push_back( name );
    }
  }
  catch( int e )
  {
    switch ( e ) {
    case 1:
      printf( "Cannot access the USB driver\n" );
      return false;
    default:
      return false;
    }
  }

  if( devList.size(  ) == 0 ) {
    printf( "No DTB connected.\n" );
    return false;
  }

  if( devList.size(  ) == 1 ) {
    usbId = devList[0];
    return true;
  }

 // If more than 1 connected device list them
  printf( "\nConnected DTBs:\n" );
  for( nr = 0; nr < devList.size(  ); nr++ ) {
    printf( "%2u: %s", nr, devList[nr].c_str(  ) );
    if( Open( devList[nr], false ) ) {
      try {
        unsigned int bid = GetBoardId(  );
        printf( "  BID=%2u\n", bid );
      }
      catch( ... ) {
        printf( "  Not identifiable\n" );
      }
      Close(  );
    }
    else
      printf( " - in use\n" );
  }

  printf( "Please choose DTB (0-%u): ", ( nDev - 1 ) );
  char choice[8];
  fgets( choice, 8, stdin );
  sscanf( choice, "%d", &nr );
  if( nr >= devList.size(  ) ) {
    nr = 0;
    printf( "No DTB opened\n" );
    return false;
  }

  usbId = devList[nr];
  return true;
}


bool CTestboard::Open( string & usbId, bool init )
{
  rpc_Clear(  );
  if( !usb.Open( &( usbId[0] ) ) )
    return false;
  printf( "USB opened %s\n", usbId.c_str(  ) );
  if( init )
    Init(  );
  return true;
}


void CTestboard::Close(  )
{
 // if( usb.Connected()) Daq_Close();
  usb.Close(  );
  rpc_Clear(  );
}


void CTestboard::mDelay( uint16_t ms )
{
  Flush(  );
#ifdef _WIN32
  Sleep( ms ); // Windows
#else
  usleep( ms * 1000 ); // Linux
#endif
}

//------------------------------------------------------------------------------
// DP: set DAC with corrected DAC table
void CTestboard::SetDAC( uint8_t reg, uint16_t value )
{
  if( reg == VDx ) {
    if( value < 3000 )
      _SetVD( value );
    else
      _SetVD( 3000 );
    return;
  }

  if( reg == VAx ) {
    if( value < 3000 )
      _SetVA( value );
    else
      _SetVA( 3000 );
    return;
  }
 // PSI re-ordering:
  static const int vdac[256] = {
    0, 1, 2, 3, 4, 5, 6, 8, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
      21, 22, 24, 23, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
      40, 39, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 56, 55,
      57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 72, 71, 73, 74,
      75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 88, 87, 89, 90, 91, 92,
      93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 104, 103, 105, 106, 107, 108,
      109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 120, 119, 121, 122,
      123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 136, 135,
      137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150,
      152, 151, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
      165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178,
      179, 180, 181, 182, 184, 183, 185, 186, 187, 188, 189, 190, 191, 192,
      193, 194, 195, 196, 197, 198, 200, 199, 201, 202, 203, 204, 205, 206,
      207, 208, 209, 210, 211, 212, 213, 214, 216, 215, 217, 218, 219, 220,
      221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 232, 231, 233, 234,
      235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 248, 247,
      249, 250, 251, 252, 253, 254, 255
  };

  int val = value;
  if( value > 255 )
    val = 255;
  else
    if( reg == Vcal
        || reg == VthrComp
        || reg == VoffsetOp
        || reg == VoffsetRO || reg == VIon || reg == VIref_ADC )
    val = vdac[value];
  roc_SetDAC( reg, val );
 //cout << setw(3) << (int) reg << setw(5) << value << setw(5) << val << endl;
}
