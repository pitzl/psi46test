
--- digital ROCs with DTB on the PA300, Apr 2014

log === set DTB parameters ===

resoff

- supply voltages and current limits:

vd 2500 mV
va 1800 mV
id  610 mA # for mod.roc 10.07
ia  200 mA

#fsel 1 # 0 = 40 MHz, 1 = 20 MHz

- timing phase [ns]
clk 22  (FEC 30 cm cable)
ctr 22  (CLK +  0)
sda 33  (CLK + 11)
tin 23  (CLK +  1)

deser 5

- levels:
clklvl 15 # at FEC, 30 cm cable
ctrlvl 15
sdalvl 15
tinlvl 15

--- pattern generator: set redout timing --------------------
pgstop
pgset 0 b101000  16  pg_resr pg_sync
pgset 1 b000100  66  pg_cal, set WBC = pg_cal - 6
pgset 2 b000010  20  pg_trg  
pgset 3 b000001   0  pg_tok, end of pgset

trigdel 200  # delay in trigger loop [BC], 200 = 5 us

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

--- set divgV2 chip DACs:

chip 350

dac   1   15  Vdig  FEC 30 cm cable
dac   2  160  Vana  for 30 mA
dac   3  130  Vsf   linearity
dac   4   12  Vcomp

dac   7   11  VwllPr  for fast Cal at 10 nTrig
dac   9   11  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12   88  VthrComp for cals

dac  13    1  VIBias_Bus
dac  14   14  Vbias_sf
dac  22   20  VIColOr

dac  15  137  VoffsetOp
dac  17  140  VoffsetRO
dac  18   55  VIon

dac  19   40  Vcomp_ADC
dac  20   61  VIref_ADC

dac  25  222  Vcal
dac  26  134  CalDel

dac 253    4  CtrlReg
dac 254   60  WBC

flush

show

mask

getvd
getva
getid
getia

mdelay 500

hvon

mdelay 500

-- set bias manually, once ROC is powered properly
-- vb 150
-- mdelay 2500
-- getib
