
# psi46digV2p1respin, DESY bump bonded Feb 2015
# test beam, 1 ft cable

log === set DTB parameters ===

resoff

- supply voltages and current limits:

vd 2500 mV
va 1800 mV
id  100 mA
ia  100 mA

- timing & levels:

deser 4

clk  1
ctr  1  (CLK +  0)
sda 16  (CLK + 15)
tin  6  (CLK +  5)

clklvl 10
ctrlvl 10
sdalvl 10
tinlvl 10

--- pattern generator: set redout timing --------------------
pgstop
pgset 0 b101000  15  pg_resr pg_sync
pgset 1 b000100 106  pg_cal = WBC + 6
pgset 2 b000010  16  pg_trg  [50 does not work: phmap errors]
pgset 3 b000001   0  pg_tok, end of pgset

trigdel 200  # delay in trigger loop [BC], 200 = 5 us

# d1  1 (40 MHz clk on D1)
# d1  4 (pg_token on D1, see pixel_dtb.h)
  d1 11 (token to ROC, see pixel_dtb.h)
# d1  5 (pg_trigg on D1, see pixel_dtb.h)
# d2  6 (cal   on D2, see pixel_dtb.h)
 d2 10 (ctr   on D2, see pixel_dtb.h)
# d1  7 (reset on D1, see pixel_dtb.h)
# d1  9 (sync  on D1, see pixel_dtb.h)
# d1 12 (token out from ROC, see pixel_dtb.h)

select 0	  [set roclist, I2C]
rocaddr 0	  [set ROC]

dopen 40100100 0 [daq_open]

--- power on --------------------------------
pon

mdelay 500

--- set divgV2.1 DACs:

chip 506

dac   1    6  Vdig
dac   2   77  Vana  24 mA
dac   3   33  Vsf   linearity
dac   4   12  Vcomp

dac   7  160  VwllPr
dac   9  160  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12   99  VthrComp

dac  13   90  VIBias_Bus
dac  22  255  VIColOr

dac  17  153  PHOffset

dac  19   10  Vcomp_ADC
dac  20  112  PHScale

dac  25  252  Vcal
dac  26  154  CalDel

dac 253    4  CtrlReg
dac 254  100  WBC = tct - 6
dac 255   12  RBreg

rddac trim36-tb  # TB22
rdtrim trim36-tb

wbc 150 # for tdt for edge-on

flush

show

mask

hvon

getvd
getva
getid
getia

fire 2 2
