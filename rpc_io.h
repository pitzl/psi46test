
#pragma once

#include "rpc_error.h"
#include <iostream> // cout

class CRpcIo
{
 protected:
  void Dump( const char *msg, const void *buffer, unsigned int size );
 public:
  virtual ~CRpcIo(  )
    {
    }
  virtual void Write( const void *buffer, unsigned int size ) = 0;
  virtual void Flush(  ) = 0;
  virtual void Clear(  ) = 0;
  virtual void Read( void *buffer, unsigned int size ) = 0;
  virtual void Close(  ) = 0;
};

// usb.h implements CRpcIo as CUSB

// Null ? This should not be called

class CRpcIoNull:public CRpcIo
{
 public:
  void Write( const void *buffer, unsigned int size )
  {
    std::cout << "rpc_io Null called for Write " << size
	      << " at " << buffer << std::endl;
    throw CRpcError( CRpcError::WRITE_ERROR );
  }
  void Flush(  )
  {
  }
  void Clear(  )
  {
  }
  void Read( void *buffer, unsigned int size )
  {
    std::cout << "rpc_io Null called for Read " << size
	      << " at " << buffer << std::endl;
    throw CRpcError( CRpcError::READ_ERROR );
  } // real Read is in usb.h
  void Close(  )
  {
  }

};
