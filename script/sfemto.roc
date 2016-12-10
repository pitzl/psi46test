
--- respin ROCs with DTB on the Femto, Feb 2015

log === set DTB parameters ===

resoff

- supply voltages and current limits:

vd 2500 mV
va 1800 mV
id  610 mA # for mod.roc 10.07
ia  200 mA

#fsel 1 # 0 = 40 MHz, 1 = 20 MHz

- timing phase [ns]
clk  2  (FEC 30 cm cable)
ctr  2  (CLK +  0)
sda 17  (CLK + 15)
tin  7  (CLK +  5)

deser 4

- levels:
clklvl 15 # at FEC, 30 cm cable
ctrlvl 15
sdalvl 15
tinlvl 15

--- pattern generator: set readout timing --------------------
pgstop
pgset 0 b101000  16  pg_resr pg_sync
pgset 1 b000100  26  pg_cal, set WBC = pg_cal - 6
pgset 2 b000010  16  pg_trg  
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

chip 504

dac   1   15  Vdig  FEC 30 cm cable
dac   2  160  Vana  for 27 mA
dac   3   30  Vsf   linearity
dac   4   12  Vcomp

dac   7  161  VwllPr  for fast Cal at 10 nTrig
dac   9  161  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12  111  VthrComp for cals

dac  13   30  VIBias_Bus
dac  22   99  VIColOr

dac  17  140  PHOffset

dac  19   10  Vcomp_ADC
dac  20  100  PHScale

dac  25  222  Vcal
dac  26  134  CalDel

dac 253    4  CtrlReg
dac 254   20  WBC

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
