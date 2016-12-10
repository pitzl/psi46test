
#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdio.h>
#include "version.h"

#define NUMSETTING 3

class CSettings
{
  FILE *f;
  bool read_int( int &value, int min, int max );
  bool read_string( char string[], int size );
public:
    bool read( const char filename[] );

 // --- data --------------------------------------------------------------
  char port_tb[20];             // default USB serial number testboard
  char path[256];               // command path
};

#endif
