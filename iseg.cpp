
// Daniel Pitzl, DESY, 16.6.2014
// iseg SHQ 122M via USB and RS232

#include <iostream> // cout
#include <cstdlib> // atoi
#include <cmath> // pow

#include "iseg.h"

#include "rs232.h"

using namespace std;

//------------------------------------------------------------------------------
double outToDouble( char *word )
{
  int sign = 1;
  int orderSign = 1;
  int order = 0;

  if( word[0] == '-' ) {
    sign = -1;
    if( word[6] == '-' )
      orderSign = -1;
  }
  else if( word[5] == '-' )
    orderSign = -1;

  char *istr;
  istr = strtok( &word[0], " +-" );
  double value = atof( istr );
  while( istr != NULL ) {
    order = atoi( istr );
    istr = strtok( NULL, " +-" );
  }

  return ( sign * value * pow( 10, orderSign * order ) );
}

//------------------------------------------------------------------------------
// Constructor: Connects to the device, initializes communication
Iseg::Iseg(  )
{
  // "S1": Read status
  // "D1": New set-voltage
  // "G1": Apply new set-voltage

  ldb = 1;

  iseg_connected = 0;
  hv_on = false;
  supply_tripped = false;
  voltage_set = 0;

  const int comPortNumber = 16; // 16 = /dev/ttyUSB0
  //const int comPortNumber = 17; // 17 = /dev/ttyUSB1, see rs232.cpp
  // sudo chmod 666 /dev/ttyUSB0
  // sudo chmod 666 /dev/ttyUSB1
  // sudo chmod o+w /dev/ttyUSB0
  // sudo chmod o+r /dev/ttyUSB0

  cout << "instatiating an iseg HV supply " << comPortNumber << endl;

  if( !openComPort( comPortNumber, 9600 ) ) {
    cout << "  cannot open RS232 port" << endl;
    return;
  }
  cout << "  Opened RS232 COM port to iseg device" << endl;

  iseg_connected = 1;

  char answer[256] = { 0 };
  writeCommandAndReadAnswer( "S1", answer );
  if( strcmp( answer, "S1=ON" ) != 0 ) {
   // Try once more:
    writeCommandAndReadAnswer( "S1", answer );
  }
  handleAnswers( answer );
  if( strcmp( answer, "S1=ON" ) != 0 ) {
    cout << "  iseg device did not return proper status code!" << endl;
    return;
  }
  cout << "  iseg communication initialized" << endl;

  hv_on = true;

}

//------------------------------------------------------------------------------
// Destructor: Will turn off the HV and terminate connection to the HV Power Supply device
Iseg::~Iseg(  )
{
  if( iseg_connected == 0 )
    return;
  setVoltage( 0 );
  closeComPort(  ); // Close the COM port
  iseg_connected = 0;
}

//------------------------------------------------------------------------------
void Iseg::handleAnswers( char *answer )
{
  if( strcmp( answer, "S1=ON" ) == 0 ) {
    if( ldb )
      cout << "  voltage is set" << endl;
  }
  else if( strcmp( answer, "S1=OFF" ) == 0 ) {
    cout << "HV Power Supply set to OFF!" << endl;
    iseg_connected = 0;
  }
  else if( strcmp( answer, "S1=TRP" ) == 0 ) {
    cout << "Power supply tripped!" << endl;
    supply_tripped = true;
  }
  else if( strcmp( answer, "S1=ERR" ) == 0 ) {
    cout << "Power supply tripped!" << endl;
    supply_tripped = true;
  }
  else if( strcmp( answer, "S1=MAN" ) == 0 ) {
    cout << "HV Power Supply set to MANUAL mode!" << endl;
    iseg_connected = 0;
  }
  else if( strcmp( answer, "S1=L2H" ) == 0 ) {
    if( ldb )
      cout << "Voltage is rising" << endl;
  }
  else if( strcmp( answer, "S1=H2L" ) == 0 ) {
    if( ldb )
      cout << "Voltage is falling" << endl;
  }
  else {
    if( ldb )
      cout << "Unknown device return value";
    supply_tripped = false;
  }
}

//------------------------------------------------------------------------------
void Iseg::status(  )
{
  if( iseg_connected == 0 ) {
    cout << "iseg not connected" << endl;
    return;
  }

  char answer[256] = { 0 };

  writeCommandAndReadAnswer( "#", answer );
  cout << "iseg module   " << answer << endl;

  writeCommandAndReadAnswer( "S1", answer );
  cout << "status        " << answer << endl;

  writeCommandAndReadAnswer( "W", answer );
  cout << "break time    " << answer << " ms" << endl;

  writeCommandAndReadAnswer( "A1", answer );
  cout << "auto start    " << answer << endl;

  writeCommandAndReadAnswer( "M1", answer );
  cout << "voltage limit " << answer << " % of 2000 V" << endl;

  writeCommandAndReadAnswer( "V1", answer );
  cout << "ramp speed    " << answer << " V/s" << endl;

  writeCommandAndReadAnswer( "D1", answer );
  cout << "voltage set   " << answer << " = " << outToDouble( answer ) << " V"
    << endl;

  writeCommandAndReadAnswer( "U1", answer );
  cout << "voltage meas  " << answer << " = " << outToDouble( answer ) << " V"
    << endl;

  writeCommandAndReadAnswer( "N1", answer );
  cout << "current limit " << answer << " % of range" << endl;

  writeCommandAndReadAnswer( "L1", answer );
  cout << "current trip  " << answer << endl;

  writeCommandAndReadAnswer( "LS1", answer );
  cout << "current trip  " << answer << " = " << outToDouble( answer ) <<
    endl;

  writeCommandAndReadAnswer( "I1", answer );
  cout << "current meas  " << answer << " = " << outToDouble( answer ) << " A"
    << endl;

}

//------------------------------------------------------------------------------
// Sets the desired voltage
bool Iseg::setVoltage( double volts )
{
  if( iseg_connected == 0 ) {
   //cout << "iseg not connected" << endl;
    return 1; // error
  }

 // Internally store voltage:
  voltage_set = double ( volts );

  if( hv_on ) {
    if( ldb )
      cout << "Setting HV to " << voltage_set << " V" << endl;
    char command[256] = { 0 };
    char answer[256] = { 0 };
    sprintf( &command[0], "D1=%.f", voltage_set );
    writeCommandAndReadAnswer( command, answer );
    writeCommandAndReadAnswer( "G1", answer );
    handleAnswers( answer );
  }
  else {
    cout << "HV not activated" << endl;
  }
 // FIXME not checking for return codes yet!
  return true;
}

//------------------------------------------------------------------------------
// Reads back the configured voltage
double Iseg::getVoltage(  )
{
  if( iseg_connected == 0 ) {
    cout << "iseg not connected" << endl;
    return 0;
  }
  char answer[256] = { 0 };
  writeCommandAndReadAnswer( "U1", answer );
  return outToDouble( answer );
}

//------------------------------------------------------------------------------
// Reads back the current drawn
double Iseg::getCurrent(  )
{
  if( iseg_connected == 0 ) {
    cout << "iseg not connected" << endl;
    return 0;
  }
  char answer[256] = { 0 };
  writeCommandAndReadAnswer( "I1", answer );
  return outToDouble( answer );
}

//------------------------------------------------------------------------------
// current limit, in % of range
bool Iseg::setCurrentLimit( int limit )
{
  if( iseg_connected == 0 ) {
    cout << "iseg not connected" << endl;
    return 0;
  }
  if( limit > 100 )
    limit = 100;

  cout << "Setting current limit to " << limit << " uA" << endl;

  char command[256] = { 0 };
  char answer[256] = { 0 };
 // Factor 100 is required since this value is the sensitive region,
 // not milliamps!
  sprintf( &command[0], "LS1=%i", limit );
  writeCommandAndReadAnswer( command, answer );
  writeCommandAndReadAnswer( "G1", answer );
  handleAnswers( answer );
  return true;
}

//------------------------------------------------------------------------------
// Read the current limit in compliance mode [uA]
double Iseg::getCurrentLimit(  )
{
  if( iseg_connected == 0 ) {
    cout << "iseg not connected" << endl;
    return 0;
  }
  char answer[256] = { 0 };
  writeCommandAndReadAnswer( "LS1", answer );

  return outToDouble( answer ) * 1E6; // A to uA
}

//------------------------------------------------------------------------------
bool Iseg::tripped(  )
{
  if( supply_tripped )
    return true;
  return false;
}
