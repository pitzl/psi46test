
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
pgset 0 b101000  16  pg_resr pg_sync
pgset 1 b000100  55  pg_cal, set WBC = pg_cal - 6
pgset 2 b000010  16  pg_trg  
pgset 3 b000001   0  pg_tok, end of pgset

trigdel 400  # delay in trigger loop [BC], 240 = 6 us

# d1 1 (40 MHz clk on D1)
# d1 4 (token on D1, see pixel_dtb.h)
  d1 5 (trigg on D1, see pixel_dtb.h)
  d2 6 (cal   on D2, see pixel_dtb.h)
# d1 7 (reset on D1, see pixel_dtb.h)
# d1 9 (sync  on D1, see pixel_dtb.h)

select 0   [set roclist, I2C]
rocaddr 0  [set ROC]

dopen 40100100   [daq_open]

--- power on --------------------------------
pon

mdelay 500

--- set divgV2 chip 311 DACs  185 Mrad

chip 311

dac   1    7  Vdig  
dac   2  213  Vana  
dac   3  130  Vsf   linearity
dac   4   12  Vcomp

dac   7    0  VwllPr  for fast Cal at 10 nTrig
dac   9    0  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim   
dac  12  105  VthrComp

dac  13    1  VIBias_Bus
dac  14   14  Vbias_sf
dac  22   20  VIColOr

dac  15  173  VoffsetOp
dac  17  120  VoffsetRO
dac  18   50  VIon

dac  19   50  Vcomp_ADC
dac  20   20  VIref_ADC

dac  25  222  Vcal
dac  26  153  CalDel

dac 253    4  CtrlReg
dac 254   49  WBC (159 to get 79 pixel/DC, but not 80 = erase)

flush

show

mask

getvd
getva
getid
getia

fire 2 2
