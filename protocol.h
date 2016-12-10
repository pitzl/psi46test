
//  log file for PSI46V1 Wafer tester
//  Beat Meier, PSI
//  24.1.2004

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdio.h>
#include <string>

class CProtocol
{
  FILE *f;
public:
    CProtocol(  )
  {
    f = NULL;
  }
   ~CProtocol(  )
  {
    close(  );
  }
  bool open( const char filename[] );
  bool append( const char filename[] );
  void close(  );
  void timestamp( const char s[] );
  void section( const char s[], bool crlf = true );
  void section( const char s[], const char par[] );
  void puts( const char s[] );
  void puts( const std::string s );
  void printf( const char *fmt, ... );
  void flush(  );
  FILE *File(  )
  {
    return f;
  }
};

#endif
