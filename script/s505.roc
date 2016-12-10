
# psi46digV2p1respin, DESY bump bonded Feb 2015

log === set DTB parameters ===

resoff

- supply voltages and current limits:

vd 2500 mV
va 1800 mV
id  100 mA
ia  100 mA

- timing & levels:

deser 4

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
pgset 0 b101000  15  pg_resr pg_sync
pgset 1 b000100 106  pg_cal = WBC + 6
pgset 2 b000010  16  pg_trg  [50 does not work: phmap errors]
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

mdelay 500

--- set divgV2.1 DACs:

chip 505

dac   1    9  Vdig  
dac   2   96  Vana  27 mA
dac   3   33  Vsf   linearity
dac   4   12  Vcomp 

dac   7  160  VwllPr
dac   9  160  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12  122  VthrComp

dac  13   30  VIBias_Bus
dac  22   99  VIColOr

dac  17  170  PHOffset

dac  19   10  Vcomp_ADC
dac  20  103  PHScale

dac  25  222  Vcal
dac  26  140  CalDel

dac 253    4  CtrlReg
dac 254  100  WBC = tct - 6
dac 255   12  RBreg

flush

show

mask

hvon

getvd
getva
getid
getia

fire 2 2
