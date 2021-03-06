/*
*
* Author: Teunis van Beelen
* Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014
* last revision: January 31, 2014
* teuniz@gmail.com
* http://www.teuniz.net/RS-232/
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2 of the License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* This version of GPL is at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*
*/


#ifndef rs232_INCLUDED
#define rs232_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>

#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

  int RS232_OpenComport( int, int );
  int RS232_PollComport( int, char *, int );
  int RS232_SendByte( int, unsigned char );
  int RS232_SendBuf( int, unsigned char *, int );
  int RS232_SendBufString( int, char *, int );
  void RS232_CloseComport( int );
  void RS232_cputs( int, const char * );
  int RS232_IsDCDEnabled( int );
  int RS232_IsCTSEnabled( int );
  int RS232_IsDSREnabled( int );
  void RS232_enableDTR( int );
  void RS232_disableDTR( int );
  void RS232_enableRTS( int );
  void RS232_disableRTS( int );


  int openComPort( const int comPortNumber, const int baud );
  void closeComPort(  );

  int writeCommand( const char *command );
  int writeCommandAndReadAnswer( const char *command, char *answer );

  int writeCommandString( const char *command );
  int writeCommandStringAndReadAnswer( const char *command, char *answer,
                                       int delay = 2 );


#ifdef __cplusplus
}                               /* extern "C" */
#endif

#endif
