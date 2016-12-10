
#include <libusb-1.0/libusb.h>
#include <cstring>
#include "profiler.h"
#include "rpc_error.h"
#include "usb.h"
#include <iostream> // cout
#include <unistd.h> // usleep

using namespace std;

//------------------------------------------------------------------------------
const char *CUSB::GetErrorMsg( int error )
{
  switch ( error ) {
  case FT_OK:
    return "ok";
  case FT_INVALID_HANDLE:
    return "invalid handle";
  case FT_DEVICE_NOT_FOUND:
    return "device not found";
  case FT_DEVICE_NOT_OPENED:
    return "device not opened";
  case FT_IO_ERROR:
    return "io error";
  case FT_INSUFFICIENT_RESOURCES:
    return "insufficient resource";
  case FT_INVALID_PARAMETER:
    return "invalid parameter";
  case FT_INVALID_BAUD_RATE:
    return "invalid baud rate";
  case FT_DEVICE_NOT_OPENED_FOR_ERASE:
    return "device not opened for erase";
  case FT_DEVICE_NOT_OPENED_FOR_WRITE:
    return "device not opened for write";
  case FT_FAILED_TO_WRITE_DEVICE:
    return "failed to write device";
  case FT_EEPROM_READ_FAILED:
    return "eeprom read failed";
  case FT_EEPROM_WRITE_FAILED:
    return "eeprom write failed";
  case FT_EEPROM_ERASE_FAILED:
    return "eeprom erase failed";
  case FT_EEPROM_NOT_PRESENT:
    return "eeprom not present";
  case FT_EEPROM_NOT_PROGRAMMED:
    return "eeprom not programmed";
  case FT_INVALID_ARGS:
    return "invalid args";
  case FT_NOT_SUPPORTED:
    return "not supported";
  case FT_OTHER_ERROR:
    return "other error";
  }
  return "unknown error";
}

//------------------------------------------------------------------------------
bool CUSB::EnumFirst( unsigned int &nDevices )
{
  ftStatus = FT_ListDevices( &enumCount, NULL, FT_LIST_NUMBER_ONLY ); // DP
 //DP ftStatus = FT_ListDevices( &enumCount, NULL, FT_LIST_NUMBER_ONLY|FT_OPEN_BY_SERIAL_NUMBER );
  if( ftStatus != FT_OK ) {
    cout << "[CUSB::EnumFirst]: " << GetErrorMsg( ftStatus ) << endl;
    nDevices = enumCount = enumPos = 0;
    return false;
  }

  nDevices = enumCount;
  enumPos = 0;
  return true;
}

//------------------------------------------------------------------------------
bool CUSB::EnumNext( char name[] )
{
  if( enumPos >= enumCount )
    return false;
  ftStatus = FT_ListDevices( ( PVOID ) enumPos, name, FT_LIST_BY_INDEX );
  if( ftStatus != FT_OK ) {
    enumCount = enumPos = 0;
    return false;
  }

  enumPos++;
  return true;
}

//------------------------------------------------------------------------------
bool CUSB::Enum( char name[], unsigned int pos )
{
  enumPos = pos;
  if( enumPos >= enumCount )
    return false;
  ftStatus = FT_ListDevices( ( PVOID ) enumPos, name, FT_LIST_BY_INDEX );
  if( ftStatus != FT_OK ) {
    enumCount = enumPos = 0;
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool CUSB::Open( char serialNumber[] )
{
  if( isUSB_open ) {
    ftStatus = FT_DEVICE_NOT_OPENED;
    return false;
  }

  m_posR = m_sizeR = m_posW = 0;

  ftStatus = FT_OpenEx( serialNumber, FT_OPEN_BY_SERIAL_NUMBER, &ftHandle );

  if( ftStatus != FT_OK ) {
   // maybe the ftdi_sio and usbserial kernel modules are attached to the device
   // try to detach them using the libusb library directly

   // prepare libusb structures
    libusb_device **list;
    libusb_device_handle *handle;
    struct libusb_device_descriptor descriptor;

   // initialise libusb and get device list
    libusb_init( NULL );
    ssize_t ndevices = libusb_get_device_list( NULL, &list );
    if( ndevices < 0 )
      return false;

    char serial[20];

    bool found = false;

   // loop over all USB devices
    for( int dev = 0; dev < ndevices; dev++ ) {
     // get the device descriptor
      int ok = libusb_get_device_descriptor( list[dev], &descriptor );
      if( ok != 0 )
        continue;
     // we're only interested in devices with one vendor and two possible product ID
      if( descriptor.idVendor != 0x0403 &&
          ( descriptor.idProduct != 0x6001 ||
            descriptor.idProduct != 0x6014 ) )
        continue;

     // open the device:
      ok = libusb_open( list[dev], &handle );
      if( ok != 0 )
        continue;
     // Read the serial number from the device
      ok = libusb_get_string_descriptor_ascii( handle,
                                               descriptor.iSerialNumber,
                                               ( unsigned char * ) serial,
                                               20 );
      if( ok < 0 )
        continue;

     // Check the device serial number
      if( strcmp( serialNumber, serial ) == 0 ) {
       // that's our device
        found = true;

       // Detach the kernel module from the device
        ok = libusb_detach_kernel_driver( handle, 0 );
        if( ok == 0 )
          printf( "Detached kernel driver from selected testboard.\n" );
        else
          printf
            ( "Unable to detach kernel driver from selected testboard.\n" );
        break;
      }

      libusb_close( handle );
    }

    libusb_free_device_list( list, 1 );

   // if the device was not found in the previous loop, don't try again
    if( !found )
      return false;

   // try to re-open with the detached device
    ftStatus = FT_OpenEx( serialNumber, FT_OPEN_BY_SERIAL_NUMBER, &ftHandle );
    if( ftStatus != FT_OK )
      return false;
  }

  ftStatus = FT_SetBitMode( ftHandle, 0xFF, 0x40 );
  if( ftStatus != FT_OK ) {
    cout << "FTD2xx usb error setting bit mode" << endl;
    return false;
  }

  ftStatus = FT_SetBaudRate( ftHandle, 9600 ); // HP 8.5.2014
  if( ftStatus != FT_OK ) {
    cout << "FTD2xx usb error setting baud rate" << endl;
    return false;
  }

 // set usb transfer size parameters
 // http://www.ftdichip.com/Support/Knowledgebase/ft_setusbparameters.htm

  ftStatus = FT_SetUSBParameters( ftHandle, 8192, 8192 ); // default: 4096, must be multiple of 64
  if( ftStatus != FT_OK ) {
    cout << "FTD2xx usb error setting USB parameters" << endl;
    return false;
  }

 //FT_SetTimeouts( ftHandle, 1000, 1000 ); // for X-rays
 //FT_SetTimeouts( ftHandle, 5000, 5000 ); // for X-rays, effmap
 //FT_SetTimeouts( ftHandle, 100000, 100000 ); // [ms] for maps
  FT_SetTimeouts( ftHandle, 200000, 200000 ); // [ms] for dacscanroc (gaincal)

  isUSB_open = true;

  return true;
}

//------------------------------------------------------------------------------
void CUSB::Close(  )
{
  if( !isUSB_open )
    return;
  FT_Close( ftHandle );
  isUSB_open = 0;
}

//------------------------------------------------------------------------------
void CUSB::SetTimeout( int ms )
{
  if( !isUSB_open )
    return;
  if( ms < 1000 )
    ms = 1000;
  FT_SetTimeouts( ftHandle, ms, ms );
}

//------------------------------------------------------------------------------
void CUSB::Write( const void *buffer, unsigned int bytesToWrite )
{
  PROFILING;

  if( !isUSB_open )
    throw CRpcError( CRpcError::WRITE_ERROR );

  DWORD k = 0;
  for( k = 0; k < bytesToWrite; k++ ) {
    if( m_posW >= USBWRITEBUFFERSIZE )
      Flush(  );
    m_bufferW[m_posW++] = ( ( unsigned char * ) buffer )[k];
  }
}

//------------------------------------------------------------------------------
void CUSB::Flush(  )
{
  PROFILING;
  DWORD bytesWritten;
  DWORD bytesToWrite = m_posW;
  m_posW = 0;

  if( !isUSB_open )
    throw CRpcError( CRpcError::WRITE_ERROR );

  if( !bytesToWrite )
    return;

  ftStatus = FT_Write( ftHandle, m_bufferW, bytesToWrite, &bytesWritten );

  if( ftStatus != FT_OK )
    throw CRpcError( CRpcError::WRITE_ERROR );
  if( bytesWritten != bytesToWrite ) {
    ftStatus = FT_IO_ERROR;
    throw CRpcError( CRpcError::WRITE_ERROR );
  }
}

//------------------------------------------------------------------------------
int CUSB::GetQueue(  )
{
  if( !isUSB_open )
    return -1;

  uint32_t bytesAvailable;
  ftStatus = FT_GetQueueStatus( ftHandle, &bytesAvailable );
  if( ftStatus != FT_OK ) {
    cout << " CUSB::GetQeue(): USB connection not OK\n";
    return -1;
  }
  return bytesAvailable;
}

//------------------------------------------------------------------------------
// Waits in 10ms steps until queue is filled with pSize bytes;
// pSize should be calculated dependent of the data type to be read:
// e.g. if you want to read 100 shorts using Read_SHORTS() then pSize = 100*sizeof(short).
// Maximum time to wait set by pMaxWait (in ms); if exceeded, the method returns false,
// otherwise it returns true (=ready to read queue)
// If pMaxWait is negative the routine will wait forever until buffer is filled.
// Experience so far (using ATB):
// - The routine works well in all tested cases and can replace extended MDelay calls
// - After a usb_write, a short delay (~ 10ms ?) is needed before calling this routine; 
//     in case of memreadout the first buffer seems to be truncated otherwise 

bool CUSB::WaitForFilledQueue( int32_t pSize, int32_t pMaxWait )
{
  if( !isUSB_open )
    return false;

  int32_t waitCounter = 0;
  int32_t bytesWaiting = GetQueue(  );
  while( ( ( waitCounter * 10 < pMaxWait ) || pMaxWait < 0 ) &&
         bytesWaiting < pSize ) {
    if( ( waitCounter + 1 ) % 100 == 0 )
      cout << "USB waiting for " << pSize << " data, t = " << waitCounter *
        10 << " ms, bytes to fetch so far = " << bytesWaiting << endl;
    usleep( 10000 ); // wait 10 ms
    waitCounter++;
    bytesWaiting = GetQueue(  );
  }
 // check if the timeout has been reached:
  if( waitCounter * 10 >= pMaxWait ) {
    cout << "USB ... timeout reached! ";
   // check if we have data
    if( bytesWaiting > 0 )
      cout << "USB buffer filled with " << bytesWaiting <<
        " bytes, check if given expectation of " << pSize << " is correct! "
        << endl;
    else
      cout << "USB NO DATA in buffer! " << endl;
    return false;
  }
 //cout << "USB ready to read data after = " << waitCounter*10 << " ms, bytes to fetch = " << bytesWaiting << endl;
  return true;
}

//------------------------------------------------------------------------------
bool CUSB::FillBuffer( DWORD minBytesToRead )
{
  PROFILING;

  if( !isUSB_open )
    return false;

  DWORD bytesAvailable, bytesToRead;

  ftStatus = FT_GetQueueStatus( ftHandle, &bytesAvailable );
  if( ftStatus != FT_OK ) {
    cout << "[CUSB::FillBuffer] GetQueueStatus error " << ftStatus << endl;
    return false;
  }
  if( m_posR < m_sizeR ) {
    cout << "[CUSB::FillBuffer] error: m_posR " << m_posR
      << " < m_sizeR " << m_sizeR << endl;
    return false;
  }

  bytesToRead =
    ( bytesAvailable > minBytesToRead ) ? bytesAvailable : minBytesToRead;
  if( bytesToRead > USBREADBUFFERSIZE )
    bytesToRead = USBREADBUFFERSIZE;

 //WaitForFilledQueue( bytesToRead ); // does not help with modtd READ_ERROR

  ftStatus = FT_Read( ftHandle, m_bufferR, bytesToRead, &m_sizeR );
  m_posR = 0;
  if( ftStatus != FT_OK ) {
    cout << "[CUSB::FillBuffer] Read error " << ftStatus
      << ", bytesAvailable " << bytesAvailable << endl;
    m_sizeR = 0;
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
void CUSB::Read( void *buffer, unsigned int bytesToRead )
{
  PROFILING;

  if( !isUSB_open )
    throw CRpcError( CRpcError::READ_ERROR );

  DWORD i;

 //cout << "[CUSB::Read] for " << bytesToRead << " bytes" << endl;

  bool done = 0;

  for( i = 0; i < bytesToRead; i++ ) { // copy byte-by-byte

    if( m_posR < m_sizeR ) // have read
      ( ( unsigned char * ) buffer )[i] = m_bufferR[m_posR++]; // just copy

    else if( done ) // skip some missing bytes
      break; // DP

    else { // need to read

      DWORD n = bytesToRead - i;
      if( n > USBREADBUFFERSIZE )
        n = USBREADBUFFERSIZE;

     //cout << "[CUSB::Read] call FillBuffer(" << n << ")" << endl;

      if( !FillBuffer( n ) ) {
        cout << "[CUSB::Read] error from FillBuffer(" << n << ")"
          << " at byte " << i << " of " << bytesToRead << endl;
        throw CRpcError( CRpcError::READ_ERROR );
      }

      if( m_posR < m_sizeR )
        ( ( unsigned char * ) buffer )[i] = m_bufferR[m_posR++];
      else {
        cout << "[CUSB::Read] time out at m_sizeR " << m_sizeR
          << ", m_posR " << m_posR
          << " at byte " << i << " of " << bytesToRead << endl;
        throw CRpcError( CRpcError::READ_TIMEOUT ); // effmap 111
      }

      if( m_sizeR < n ) {
        cout << "[CUSB::Read] error m_sizeR " << m_sizeR
          << " < n " << n
          << " at byte " << i << " of " << bytesToRead << endl;
       //DP throw CRpcError(CRpcError::READ_ERROR);
        done = 1; // these bytes will never come...
      }

    } // read

  } // bytes i

}

//------------------------------------------------------------------------------
void CUSB::Clear(  )
{
  PROFILING;
  if( !isUSB_open )
    return;

  ftStatus = FT_Purge( ftHandle, FT_PURGE_RX | FT_PURGE_TX );
  m_posR = m_sizeR = 0;
  m_posW = 0;
}
