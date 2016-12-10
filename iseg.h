
#pragma once

class Iseg
{
  bool ldb;
  bool iseg_connected;
  bool hv_on;
  bool supply_tripped;
  double voltage_set;

public:

    Iseg(  );
   ~Iseg(  );
  void status(  );
  void handleAnswers( char *answer );
  bool setVoltage( double volts );
  double getVoltage(  );
  double getCurrent(  );
  bool setCurrentLimit( int microampere );
  double getCurrentLimit(  );
  bool tripped(  );

};
