
# psi46digV2.1

log === set DTB parameters ===

resoff

- supply voltages and current limits:

vd 2500 mV
va 1800 mV
id  100 mA
ia  100 mA

- timing & levels:
clk  2
ctr  2  (CLK +  0)
sda 17  (CLK + 15)
tin  7  (CLK +  5)

clklvl 10
ctrlvl 10
sdalvl 10
tinlvl 10

--- pattern generator: set redout timing --------------------
pgstop
pgset 0 b101000  25  pg_resr pg_sync
pgset 1 b000100 106  pg_cal, set WBC = pg_cal - 7
pgset 2 b000010  46  pg_trg  
pgset 3 b000001   0  pg_tok, end of pgset

trigdel 200  # delay in trigger loop [BC], 200 = 5 us

# d1 1 (40 MHz clk on D1)
# d1 4 (token on D1, see pixel_dtb.h)
  d1 5 (trigg on D1, see pixel_dtb.h)
  d2 6 (cal   on D2, see pixel_dtb.h)
# d1 7 (reset on D1, see pixel_dtb.h)
# d1 9 (sync  on D1, see pixel_dtb.h)

select 0	  [set roclist, I2C]
rocaddr 0	  [set ROC]

dopen 40100100 0 [daq_open]

--- power on --------------------------------
pon
hvon

mdelay 500

--- set divgV2.1 chip 404 DACs run 12526 trim 32

chip 404

dac   1    9  Vdig  
dac   2   84  Vana  
dac   3   33  Vsf   linearity
dac   4    4  Vcomp better at low trim 13.5.2014

dac   7  160  VwllPr (for effmap 98)
dac   9  160  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12  100  VthrComp

dac  13   30  VIBias_Bus
dac  22   99  VIColOr

dac  17  210  PHOffset

dac  19   50  Vcomp_ADC
dac  20   80  PHScale

dac  25  222  Vcal
dac  26  136  CalDel

dac 253    4  CtrlReg
dac 254   99  WBC (159 to get 79 pixel/DC, but not 80 = erase)
dac 255   12  RBreg

flush

show

mask

getvd
getva
getid
getia

fire 2 2
