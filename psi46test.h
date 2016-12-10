
#ifndef PSI46TEST_H
#define PSI46TEST_H

#define withROOT

#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <TCanvas.h>

#include "version.h"
#include "pixel_dtb.h"
#include "settings.h"
//DP#include "prober.h"
#include "protocol.h"
//DP#include "pixelmap.h"
//DP#include "test.h"
//DP#include "chipdatabase.h"

#define VERSIONINFO TITLE " " VERSION " (" TIMESTAMP ")" // from version.h

// global variables:
extern int nEntry;              // counts the entries in the log file

extern CTestboard tb;           // in pixel_dtb.h
extern CSettings settings;      // global settings
//DP extern CProber prober; // prober
extern CProtocol Log;           // log file

//DP extern CChip g_chipdata;

//DP extern int delayAdjust;
//DP extern int deserAdjust;

void cmd(  );

#ifdef withROOT

class MyMainFrame:public TGMainFrame
{

private:
  TGMainFrame * fMain;
  TRootEmbeddedCanvas *fEcanvas;
public:
  MyMainFrame( const TGWindow * p, UInt_t w, UInt_t h );
  virtual ~ MyMainFrame(  );
  TCanvas *GetCanvas(  );
};

#endif

#endif
