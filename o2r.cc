
// Daniel Pitzl (DESY) Aug 2014, Nov 2016
// .out to .root for modules

// o2r 14

#include <stdlib.h> // atoi
#include <iostream> // cout
#include <iomanip> // setw
#include <string> // strings
#include <sstream> // stringstream
#include <fstream> // files
#include <vector>
#include <cmath>

#include <TFile.h>
#include <TH1I.h>
#include <TH2I.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TRandom1.h>

using namespace std;

struct pixel {
  int col;
  int row;
  int adc;
  double cal;
  int roc;
  int colROC;
  int rowROC;
  int raw[6];
  //DP double xy[2];
};

struct cluster {
  vector <pixel> vpix;
  int size;
  int sumA;//DP
  double charge;
  double col,row;
  int layer;
  //DP double xy[2]; // local coordinates
  //DP double xyz[3];
};

// globals:

bool haveGain = 0;

double p0[16][52][80];
double p1[16][52][80];
double p2[16][52][80];
double p3[16][52][80];
double p4[16][52][80];
double p5[16][52][80];

pixel pb[66560]; // global declaration: vector of pixels with hit
int fNHit; // global

//------------------------------------------------------------------------------
// inverse decorrelated Weibull PH -> large Vcal DAC
double PHtoVcal( double ph, int roc, int col, int row )
{
  if( !haveGain )
    return ph;

  if( roc < 0 )
    return ph;
  if( roc > 15 )
    return ph;

  if( col < 0 )
    return ph;
  if( col > 51 )
    return ph;

  if( row < 0 )
    return ph;
  if( row > 79 )
    return ph;

  // phroc2ps decorrelated: ph = p4 - p3*exp(-t^p2), t = p0 + q/p1

  double Ared = ph - p4[roc][col][row]; // p4 is asymptotic maximum

  if( Ared >= 0 ) {
    Ared = -0.1; // avoid overflow
  }

  double a3 = p3[roc][col][row]; // positive

  // large Vcal = ( (-ln(-(A-p4)/p3))^1/p2 - p0 )*p1

  double vc =
    p1[roc][col][row] * ( pow( -log( -Ared / a3 ), 1 / p2[roc][col][row] ) - p0[roc][col][row] );

  if( vc > 999 )
    cout << "overflow " << vc << " at"
	 << setw( 3 ) << col
	 << setw( 3 ) << row << ", Ared " << Ared << ", a3 " << a3 << endl;

  //return vc * p5[roc][col][row]; // small Vcal
  return vc; // large Vcal
}

// ----------------------------------------------------------------------
vector<cluster> getClus()
{
  // returns clusters with local coordinates
  // decodePixels should have been called before to fill pixel buffer pb 
  // simple clusterization
  // cluster search radius fCluCut ( allows fCluCut-1 empty pixels)

  const int fCluCut = 1; // clustering: 1 = no gap (15.7.2012)
  //const int fCluCut = 2;

  vector<cluster> v;
  if( fNHit == 0 ) return v;

  int* gone = new int[fNHit];

  for( int i = 0; i < fNHit; ++i )
    gone[i] = 0;

  int seed = 0;

  while( seed < fNHit ) {

    // start a new cluster

    cluster c;
    c.vpix.push_back( pb[seed] );
    gone[seed] = 1;

    // let it grow as much as possible:

    int growing;
    do{
      growing = 0;
      for( int i = 0; i < fNHit; ++i ) {
        if( !gone[i] ){ // unused pixel
          for( unsigned int p = 0; p < c.vpix.size(); ++p ) { // vpix in cluster so far
            int dr = c.vpix.at(p).row - pb[i].row;
            int dc = c.vpix.at(p).col - pb[i].col;
            if( (   dr>=-fCluCut) && (dr<=fCluCut) 
		&& (dc>=-fCluCut) && (dc<=fCluCut) ) {
              c.vpix.push_back(pb[i]);
	      gone[i] = 1;
              growing = 1;
              break; // important!
            }
          } // loop over vpix
        } // not gone
      } // loop over all pix
    }
    while( growing );

    // added all I could. determine position and append it to the list of clusters:

    c.sumA = 0;
    c.charge = 0;
    c.size = 0;
    c.col = 0;
    c.row = 0;
    //c.xy[0] = 0;
    //c.xy[1] = 0;
    double sumQ = 0;

    for( vector<pixel>::iterator p = c.vpix.begin();  p != c.vpix.end();  ++p ) {
      c.sumA += p->adc; // Aout
      double Qpix = p->cal; // calibrated [Vcal]
      if( Qpix < 0 ) Qpix = 1; // DP 1.7.2012
      c.charge += Qpix;
      //if( Qpix > 20 ) Qpix = 20; // DP 25.8.2013, cut tail. tiny improv
      sumQ += Qpix;
      c.col += (*p).col*Qpix;
      c.row += (*p).row*Qpix;
      //c.xy[0] += (*p).xy[0]*Qpix;
      //c.xy[1] += (*p).xy[1]*Qpix;
    }

    c.size = c.vpix.size();

    //cout << "(cluster with " << c.vpix.size() << " pixels)" << endl;

    if( ! c.charge == 0 ) {
      c.col /= sumQ;
      c.row /= sumQ;
      //c.col /= c.charge;
      //c.row /= c.charge;
      //c.xy[0] /= c.charge;
      //c.xy[1] /= c.charge;
    }
    else {
      c.col = (*c.vpix.begin()).col;
      c.row = (*c.vpix.begin()).row;
      //c.xy[0] = (*c.vpix.begin()).xy[0];
      //c.xy[1] = (*c.vpix.begin()).xy[1];
      cout << "GetClus: cluster with zero charge" << endl;
    }

    v.push_back(c); // add cluster to vector

    // look for a new seed = used pixel:

    while( (++seed < fNHit) && gone[seed] );

  } // while over seeds

  // nothing left,  return clusters

  delete gone;
  return v;
}

//------------------------------------------------------------------------------
int main( int argc, char* argv[] )
{
  cout << "main " << argv[0] << " called with " << argc << " arguments" << endl;

  if( argc == 1 ) {
    cout << "give run number" << endl;
    return 1;
  }

  // run number = last argument:

  string runnum( argv[argc-1] );
  int run = atoi( argv[argc-1] );

  cout << "run " << run << endl;

  ostringstream evFileName; // output string stream
  evFileName << "run" << run << ".out";

  cout << "try to open " << evFileName.str();

  ifstream evFile( evFileName.str() );

  if( !evFile ) {
    cout << " : failed " << endl;
    return 2;
  }

  cout << " : succeed " << endl;

  // gain:

  string gainFileName;
  gainFileName = "D1020-gaincal.dat"; // L1 at PSI
  
  if( run >= 1778 )
    gainFileName = "D4294-tb24-trim32-gaincal.dat";

  if( run >= 1786 )
    gainFileName = "D4339-tb24-gaincal.dat";

  if( run >= 1794 )
    gainFileName = "D4294-tb24-trim32-gaincal.dat";

  //double ke = 0.350; // large Vcal -> ke

  if( gainFileName.length(  ) > 0 ) {

    ifstream gainFile( gainFileName.c_str() );

    if( gainFile ) {

      haveGain = 1;
      cout << "gain: " << gainFileName << endl;

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

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // (re-)create root file:

  ostringstream fname; // output string stream
  fname << "run" << run << ".root";
  TFile * histoFile = new TFile( fname.str(  ).c_str(  ), "RECREATE" );

  // book histos:

  TH1I hcol0( "col0", "column ROCs 0-7;column ROCs 0-7;pixels", 416, 0, 416 );
  TH1I hcol1( "col1", "column ROCs 15-8;column ROCs 15-8;pixels", 416, 0, 416 );
  TH1I hrow( "row", "row;row;pixels", 160, 0, 160 );

  TH1I hcolroc( "colroc", "ROC column;ROC column;hits", 52, 0, 52 );
  TH1I hroc( "roc", "ROCs;ROC;hits", 16, 0, 16 );

  TH2I hmap( "pxmap", "pixel map;column;row;pixels",
	     8*52, 0, 8*52, 2*80, 0, 160 );

  TH1I hadc( "adc", "pixel ADC;pixel PH [ADC];pixels", 256, 0, 256 );
  TH1I hpxv( "pxv", "pixel charge;pixel charge [Vcal];pixels",
	     200, 0, 200 ); // for X-rays
  TH1I hpxv1( "pxv1", "1st pixel charge;pixel charge [Vcal];1st pixels",
	     200, 0, 200 ); // for X-rays
  TH1I hpxv2( "pxv2", "2nd pixel charge;pixel charge [Vcal];2nd pixels",
	     200, 0, 200 ); // for X-rays
  TH1I hpxv3( "pxv3", "3rd pixel charge;pixel charge [Vcal];3rd pixels",
	     200, 0, 200 ); // for X-rays
  TH1I hpxv4( "pxv4", "4th pixel charge;pixel charge [Vcal];4th pixels",
	     200, 0, 200 ); // for X-rays
  TH1I hpxq( "pxq", "pixel charge;pixel charge [ke];pixels",
	     200, 0, 40 ); // for Sr90
  TProfile
    qvsx0( "qvsx0",
	      "pixel charge untere ROCs;pixel col;<pixel charge> [Vcal]",
	      8*52, 0, 8*52,  0, 999 );
  TProfile
    qvsx1( "qvsx1",
	      "pixel charge obere ROCs;pixel col;<pixel charge> [Vcal]",
	      8*52, 0, 8*52,  0, 999 );

  TH1I hnpx( "npx",
	     "pixels per non-empty event per TBM channel;pixels per non-empty event per TBM channel;events",
	     100, 0, 100 );
  TH1I hlognpx( "lognpx",
		"pixels per non-empty event per TBM channel;log_{10}(number of pixels);events",
		60, 1, 4 );

  TProfile npx0vst6( "npx0vst6",
		     "lower pixels vs time;event number;<pixels/lower half module>",
		     1000, 0, 1e6, 0, 3E4 );
  TProfile npx1vst6( "npx1vst6",
		     "upper pixels vs time;event number;<pixels/upper half module>",
		     1000, 0, 1e6, 0, 3E4 );

  TH1I hpxrate2( "pxrate2",
	    "pixel hit rate;hits per pixel;pixels",
	    100, 0, 1E2 );
  TH1I hpxrate3( "pxrate3",
	    "pixel hit rate;hits per pixel;pixels",
	    100, 0, 1E3 );
  TH1I hpxrate4( "pxrate4",
	    "pixel hit rate;hits per pixel;pixels",
	    100, 0, 1E4 );
  TH1I hpxrate5( "pxrate5",
	    "pixel hit rate;hits per pixel;pixels",
	    100, 0, 1E5 );

  TH1I hdc( "dc",
	    "double column rate;hits per DC;DCs",
	    80, 0, 80 );
  TH1I hdcn( "dcn",
	     "double column rate;hits per DC;hits",
	     80, 0, 80 );

  TH1I hbig( "big",
	     "normal vs big pixel rate;normal or big pixel;hits",
	     2, 0, 2 );

  TH1I hncl( "ncl",
	     "cluster per non-empty event;clusters;non-empty events",
	     80, 0, 80 );

  TH1I hdx( "dx", "dx;dx [col];cluster pairs", 210, -420, 420 );
  TH1I hdy( "dy", "dy;dy [row];cluster pairs", 160, -80, 80 );

  TH1I hsiz( "clsz", "cluster size;pixels/cluster;cluster",
	     50, 0, 50 );
  TH1I hclv( "clv", "cluster charge;cluster charge [Vcal];clusters",
	     200, 0, 200 );
  TH1I hclq( "clq", "cluster charge;cluster charge [ke];clusters",
	     200, 0, 100 );
  TH1I hclq5( "clq5", "cluster charge;cluster charge [ke];clusters",
	     100, 0, 500 );
  TH1I hclv1( "clv1", "1-px cluster charge;cluster charge [Vcal];1-px clusters",
	      200, 0, 100 );
  TH1I hclv2( "clv2", "2-px cluster charge;cluster charge [Vcal];2-px clusters",
	      200, 0, 100 );
  TH1I hclv3( "clv3", "3-px cluster charge;cluster charge [Vcal];3-px clusters",
	      200, 0, 100 );

  TProfile2D * szvsxy;
  szvsxy = new
    TProfile2D( "clszmap",
		"pixels/cluster;cluster col;cluster row;<pixels/cluster>",
		8*52, 0, 8*52, 2*80, 0, 160, 0, 99 );

  TProfile2D * qvsxy;
  qvsxy = new
    TProfile2D( "clqmap",
		"cluster charge;cluster col;cluster row;<cluster charge> [Vcal]",
		8*52, 0, 8*52, 2*80, 0, 160, 0, 1E4 );

  TProfile
    qvssz( "clqvssz",
		"cluster charge vs size;cluster size [pixels];<cluster charge> [ke]",
		25, 0, 25, 0, 500 );
  TProfile
    szvsq( "clszvsq",
		"cluster size vs charge;cluster charge [ke];<cluster size> [pixels]",
		100, 0, 200, 0, 99 );

  TProfile
    qvst6( "qvst6",
		 "cluster charge;event number;<cluster charge> [Vcal]",
		 1000, 0, 1e6, 0, 1E4 );
  TProfile
    qvst7( "qvst7",
		 "cluster charge;event number;<cluster charge> [Vcal]",
		 1000, 0, 1e7, 0, 1E4 );
  TProfile
    qvst8( "qvst8",
		 "cluster charge;event number;<cluster charge> [Vcal]",
		 1000, 0, 1e8, 0, 1E4 );

  TH1I * hchrgroc[16];
  for(int roc = 0; roc<16; ++roc ) {
    hchrgroc[roc] = new
      TH1I ( Form( "cl_charge_ROC%02i", roc ),
	     Form( "cluster charge ROC %i ;cluster charge [Vcal];clusters", roc ),
	     200, 0, 200 );
  }

  double log10 = log(10);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // event loop:

  bool ldb = 0;
  int nev = 0; // lines
  int iev = 0; // trigger number
  int kpx = 0;
  int mpx = 0;
  int NN[8*52][2*80];

  // Read file by lines = TBM channels = half modules

  while( evFile.good() && ! evFile.eof() ) {

    string sl;
    getline( evFile, sl ); // read one line into string

    // one line = one trigger from one TBM channel = half module

    istringstream is( sl ); // tokenize string

    // 1234 358 81 47 = ev col row adc

    is >> iev; // trigger number

    ++nev; // non-empty lines

    //if( nev >=  893735 ) ldb = 1; // 1070 x02
    //if( nev > 2893734 ) ldb = 1; // 1070
    if( ldb ) cout << "line " << nev << " ev " << iev << " char " << sl.size() << endl;

    if( !(nev%100000) )
      cout << "line " << nev
	   << " ev " << iev
	   << " pix " << mpx
	   << endl;
    
    if( is.eof() ) continue; // next event

    int npx = 0;
    bool ierr = 0;
    int ncol[8*52] = {0};
    int ch = 0;
    int prevroc = -1;
    int npxroc = 0;

    do {

      int xm;
      is >> xm; // 0..415
      if( is.eof() ) {
	ierr = 1;
	cout << "truncated at line " << nev << " ev " << iev << endl;
	break;
      }
      if( ldb ) cout << npx << " " << xm << flush;

      int ym;
      is >> ym; // 0..159
      if( is.eof() ) {
	ierr = 1;
	cout << "truncated at line " << nev << " ev " << iev << endl;
	break;
      }
      if( ldb ) cout << " " << ym << flush;

      int roc = xm / 52; // 0..7
      int col = xm % 52; // 0..51
      int row = ym;

      if( ym > 79 ) { // module
	roc = 15 - roc; // 15..8
	col = 51 - col; // 51..0
	row = 159 - ym; // 79..0
	ch = 1;
      }

      int adc;
      is >> adc;
      if( ldb ) cout << " " << adc << flush;
      if( ldb ) cout << " (" << is.tellg() << ") " << flush;

      if( xm < 0 || xm > 415 || ym < 0 || ym > 159 || adc < 0 || adc > 255 ) {
	cout << "pixel error at line " << nev << " ev " << iev
	     << " x " << xm << " y " << ym << " a " << adc
	     << endl;
	ierr = 1;
	break;
      }

      if( roc != prevroc ) npxroc = 0;
      ++npxroc;

      double q = PHtoVcal( adc, roc, col, row ); // [Vcal]

      if( ym < 80 )
	hcol0.Fill( xm + 0.1 );
      else
	hcol1.Fill( xm + 0.1 );
      hcolroc.Fill( col + 0.1 );
      hrow.Fill( ym + 0.1 );
      hroc.Fill( roc + 0.1 );
      hadc.Fill( adc + 0.1 );
      hmap.Fill( xm + 0.1, ym + 0.1 );
      hpxv.Fill( q ); // [Vcal]
      if(      npxroc == 1 )
	hpxv1.Fill( q ); // [Vcal]
      else if( npxroc == 2 )
	hpxv2.Fill( q ); // [Vcal]
      else if( npxroc == 3 )
	hpxv3.Fill( q ); // [Vcal]
      else if( npxroc == 4 )
	hpxv4.Fill( q ); // [Vcal]
      hpxq.Fill( q*50E-3 );
      ++ncol[xm];
      ++NN[xm][ym];

      hchrgroc[roc]->Fill( q );
      qvst6.Fill( nev, q );
      qvst7.Fill( nev, q );
      qvst8.Fill( nev, q );
      if( ym < 80 )
	qvsx0.Fill( xm, q );
      else
	qvsx1.Fill( xm, q );

      // fill pixel block for clustering:
      pb[npx].col = xm; // module clustering
      pb[npx].row = ym;
      pb[npx].adc = adc;
      pb[npx].cal = q;
      ++npx;
      ++mpx;

      prevroc = roc;

    }
    while( ! ierr && is.good() && ! is.eof() &&
	   is.tellg() > 0 && is.tellg() < (int) sl.size() ); // one TBM channel data

    //cout << sl.size() << "  " << is.tellg() << endl; // identical

    if( ldb ) cout << endl;

    if( ierr ) {
      cout << "error in line " << nev << " ev " << iev << ". skipped" << endl;
      continue; // skip event
    }

    hnpx.Fill( npx + 0.1 );
    hlognpx.Fill( log(npx)/log10 );
    if( ch )
      npx1vst6.Fill( iev, npx );
    else
      npx0vst6.Fill( iev, npx );

    if( npx > kpx ) {
      kpx = npx;
      cout << "max pix " << kpx << " at line " << nev << " ev " << iev << endl;
    }

    // DC rate:

    for( int dc = 0; dc < 8*26; ++dc ) {
      int ndc = ncol[2*dc+0] + ncol[2*dc+1];
      if( ndc == 0 ) continue;
      hdc.Fill( ndc + 0.1 ); // un-weighted
      hdcn.Fill( ndc + 0.1, double(ndc) ); // weighted
    }

    // big pix:

    for( int col = 0; col < 8*52; ++col ) {
      int n = ncol[col];
      if( n == 0 ) continue;
      int c = col%52;
      if( c == 0 || c == 51 )
	hbig.Fill( 1.1, n );
      else
	hbig.Fill( 0.1, n );
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // clustering:

    fNHit = npx; // for cluster search

    vector<cluster> clust = getClus();	    

    hncl.Fill( clust.size() + 0.1 );
    if( ldb ) cout << "  clusters " << clust.size() << endl;

    // 2-cluster correlation:

    for( vector<cluster>::iterator c1 = clust.begin(); c1 != clust.end(); ++c1 )
      for( vector<cluster>::iterator c2 = clust.begin(); c2 != clust.end(); ++c2 ) {
	if( c1 == c2 ) continue;
	double dx = c2->col - c1->col;
	double dy = c2->row - c1->row;
	hdx.Fill( dx ); // triangle
	hdy.Fill( dy );
      }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // look at clusters:

    for( vector<cluster>::iterator c = clust.begin(); c != clust.end(); ++c ) {

      hsiz.Fill( c->size + 0.1 );
      hclv.Fill( c->charge ); // [Vcal]
      hclq.Fill( c->charge*50e-3 ); // [ke]
      hclq5.Fill( c->charge*50e-3 ); // [ke]
      szvsxy->Fill( c->col, c->row, c->size );
      qvsxy->Fill( c->col, c->row, c->charge );

      qvssz.Fill( c->size + 0.1, c->charge*50E-3 );
      szvsq.Fill( c->charge*50e-3, c->size );

      if(      c->size == 1 )
	hclv1.Fill( c->charge );
      else if( c->size == 2 )
	hclv2.Fill( c->charge );
      else
	hclv3.Fill( c->charge );

      // pix in clus:

    } // clusters

  } // while events

    // pix rate:

  for( int col = 0; col < 8*52; ++col )
    for( int row = 0; row < 160; ++row ) {
      hpxrate2.Fill( NN[col][row] );
      hpxrate3.Fill( NN[col][row] );
      hpxrate4.Fill( NN[col][row] );
      hpxrate5.Fill( NN[col][row] );
    }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  cout << "eof" << endl;
  cout << "lines " << nev
       << ", triggers " << iev
       << ", pixel hits " << mpx
       << " = " << double(mpx)/iev << " per trigger"
       << endl;

  histoFile->Write();
  histoFile->Close();
  cout << histoFile->GetName() << endl;

  return 0;
}
