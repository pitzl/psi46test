
# psi46digV2.1

log === set DTB parameters ===

resoff

- supply voltages and current limits:

vd 2500 mV
va 1800 mV
id  100 mA
ia  100 mA

- timing & levels:
clk  4
ctr  4  (CLK +  0)
sda 19  (CLK + 15)
tin  9  (CLK +  5)

clklvl 10
ctrlvl 10
sdalvl 10
tinlvl 10

--- pattern generator: set redout timing --------------------
pgstop
pgset 0 b101000  15  pg_resr pg_sync
pgset 1 b000100  76  pg_cal, set WBC = pg_cal - 7
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
hvon

mdelay 500

--- set divgV2.1 chip 401 DACs beam test

chip 401

dac   1    9  Vdig  
dac   2   85  Vana  25 mA
dac   3   33  Vsf   linearity
dac   4   12  Vcomp 

dac   7  160  VwllPr
dac   9  160  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12  109  VthrComp

dac  13   30  VIBias_Bus
dac  22   99  VIColOr

dac  17  207  PHOffset

dac  19   10  Vcomp_ADC
dac  20   88  PHScale

dac  25  222  Vcal
dac  26  116  CalDel  

dac 253    4  CtrlReg
dac 254   69  WBC     # 49 too short with clock stretch: errors
dac 255   12  RBreg

flush

show

mask

getvd
getva
getid
getia

fire 2 2
