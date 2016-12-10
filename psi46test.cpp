/* -------------------------------------------------------------
 *
 *  main program for psi46test
 *
 *  author:      Beat Meier, PSI, 2006
 *  modified:    Daniel Pitzl, DESY, 2014
 *
 * -------------------------------------------------------------
 */

#include <string>
#include <vector>
#include <iostream> // cout

#include <TApplication.h>
#include <TGClient.h>
#include <TGFrame.h>
#include <TFile.h>

#include "psi46test.h"

#include "profiler.h"

using namespace std;

// globals:

int nEntry;                     // counts the chip tests

CTestboard tb;
CSettings settings;             // global settings
//DP CProber prober;
CProtocol Log;

//char filename[512];             // log file

//------------------------------------------------------------------------------
void help(  )
{
  printf( "usage: psi46test a.log\n" );
}

#ifdef withROOT
//------------------------------------------------------------------------------
MyMainFrame::MyMainFrame( const TGWindow * p, UInt_t w, UInt_t h )
:TGMainFrame( p, w, h )
{
  cout << "MyMainFrame..." << endl;
 // Create a main frame:
  fMain = new TGMainFrame( p, w, h );

  fMain->SetWMPosition( 99, 0 ); // no effect

 // Create canvas widget:
  fEcanvas = new TRootEmbeddedCanvas( "Ecanvas", fMain, w, h );

  fMain->AddFrame( fEcanvas,
                   new TGLayoutHints( kLHintsExpandX | kLHintsExpandY, 1, 1, 1, 1 ) );

 // Set a name to the main frame:
  fMain->SetWindowName( "psi46test" );

 // Map all subwindows of main frame:
  fMain->MapSubwindows(  );

 // Initialize the layout algorithm:
  fMain->Resize( fMain->GetDefaultSize(  ) );

 // Map main frame:
  fMain->MapWindow(  );
}

//------------------------------------------------------------------------------
MyMainFrame::~MyMainFrame(  )
{
 // Clean up used widgets: frames, buttons, layouthints
  cout << "~MyMainFrame: Cleanup..." << endl;
  fMain->Cleanup(  );
  cout << "~MyMainFrame: delete canvas..." << endl;
  //delete fEcanvas;
  cout << "~MyMainFrame: delete Main" << flush << endl;
  //delete fMain;
}

//------------------------------------------------------------------------------
TCanvas *MyMainFrame::GetCanvas(  )
{
//TCanvas *c1 = fEcanvas->GetCanvas();
  return ( fEcanvas->GetCanvas(  ) );
}
#endif

//------------------------------------------------------------------------------
uint32_t GetHashForString( const char *s )
{
 // Using some primes
  uint32_t h = 31;
  while( *s ) {
    h = ( h * 54059 ) ^ ( s[0] * 76963 );
    s++;
  }
  return h % 86969;
}

//------------------------------------------------------------------------------
uint32_t GetHashForStringVector( const std::vector < std::string > &v )
{
  uint32_t ret = 0;
  for( size_t i = 0; i < v.size(  ); i++ )
    ret += ( i + 1 ) * GetHashForString( v[i].c_str(  ) );
  return ret;
}

//------------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
  printf( VERSIONINFO "\n" );

  char filename[512]; // log file

  if( argc == 2 )
    strncpy( filename, argv[1], sizeof( filename ) );
  else
    strncpy( filename, "test.log", sizeof( filename ) );

  string filestring = filename;

  // search for period in filename:
  string::size_type idx = filestring.find(".");
  if( idx == string::npos ) {
    // filename does not contain any period
    filestring += ".log";
    strcat( filename, ".log" ); // concatenate (append)
  }

 // load settings:

  cout << "reading psi46test.ini..." << endl;
  if( !settings.read( "psi46test.ini" ) ) {
    printf( "error reading \"psi46test.ini\"\n" );
    return 2;
  }

  cout << "logging to " << filename << endl;
  if( !Log.open( filename ) ) {
    printf( "log: error creating file\n" );
    return 3;
  }

 // open test board on USB:

  Log.section( "DTB" );
  string usbId;

  try {
    if( !tb.FindDTB( usbId ) ) {
      printf( "found DTB %s\n", usbId.c_str(  ) );
    }
    else if( tb.Open( usbId ) ) {
      printf( "\nDTB %s opened\n", usbId.c_str(  ) );
      string info;
      try {
        tb.GetInfo( info );
        printf( "--- DTB info-------------------------------------\n"
                "%s"
                "-------------------------------------------------\n",
                info.c_str(  ) );
        Log.puts( info.c_str(  ) );
        tb.Welcome(  );
        tb.Flush(  );

       // check DTB SW hash code:

        string dtb_hashcmd;
        tb.GetRpcCallName( 5, dtb_hashcmd );
        if( dtb_hashcmd.compare( "GetRpcCallHash$I" ) != 0 )
          cout << "Your DTB flash file is outdated:"
            <<
            " it does not provide an RPC hash value for compatibility checks."
            << endl <<
            "please upgrade to the flash file corresponding to this version of psi46test"
            << endl;
        else {

         // Get hash for the PC RPC command list:

          uint32_t pcCmdHash =
            GetHashForStringVector( tb.GetHostRpcCallNames(  ) );
          cout << "PC  hash " << pcCmdHash << endl;

          uint32_t dtbCmdHash = tb.GetRpcCallHash(  );
          cout << "DTB hash " << dtbCmdHash << endl;

         // If they don't match check RPC calls one by one and print offenders:

          if( dtbCmdHash != pcCmdHash ) {
            cout << "RPC call hashes of DTB and PC do not match!" << endl;
            cout << "please: upgrade .flash" << endl;

            if( !tb.RpcLink(  ) )
              cout << "Please upgrade your DTB with the correct flash file."
                << endl;
            else
              cout << "but all required functions are present: OK to proceed"
                << endl;
          }
          else
            cout << "RPC call hashes of PC and DTB match: " << pcCmdHash <<
              endl;
        }

      }
      catch( CRpcError & e ) {
        e.What(  );
        printf
          ( "ERROR: DTB software version could not be identified, please upgrade the flash file\n" );
        tb.Close(  );
        printf( "Connection to Board %s has been cancelled\n",
                usbId.c_str(  ) );
      }
    }
    else {
      printf( "USB error: %s\n", tb.ConnectionError(  ) );
      printf( "DTB: could not open port to device %s\n", settings.port_tb );
      printf
        ( "Connect testboard and try command 'scan' to find connected devices.\n" );
      printf( "Make sure you have permission to access USB devices.\n" );
    }

    Log.flush(  );

#ifdef withROOT

    string::size_type dot = filestring.find(".");
    string rootfile = filestring.substr( 0, dot ) + ".root";
    TFile *histoFile = new TFile( rootfile.c_str(), "RECREATE" );

    cout << "ROOT application..." << endl;

    TApplication theApp( "psi46test", &argc, argv );
#endif

   // call command interpreter:

    nEntry = 0;

    cmd(  );

    cout << "Daq close..." << endl;
    tb.Daq_Close(  );
    tb.Flush(  );

    cout << "ROOT close ..." << endl;
   //histoFile->Write();
    histoFile->Close(  );
    cout << "                closed " << histoFile->GetName(  ) << endl;
    tb.Close(  );
  }
  catch( CRpcError & e ) {
    e.What(  );
  }

  return 0;
}
