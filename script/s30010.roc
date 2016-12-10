
-- 10 MHz, no hits

log === set DTB parameters ===

resoff

-- supply voltages and current limits:

vd 2500 mV
va 1800 mV
id  100 mA
ia  100 mA

-- timing & levels:

fsel 2 # 0 = 40 MHz, 1 = 20 MHz, 2 = 10 MHz

deser 2

clk 19
ctr 19
sda 50
tin 60

clklvl 14
ctrlvl 14
sdalvl 14
tinlvl 14

--- pattern generator: reset cal trig token
pgstop
pgset 0 b101000  16  pg_resr pg_sync
pgset 1 b000100  66  pg_cal, set WBC = pg_cal - 
pgset 2 b000010  16  pg_trg  
pgset 3 b000001   0  pg_tok, end of pgset

trigdel 400  # delay in trigger loop [BC], 400 = 40 us, needed

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

--- set divgV2 chip 300 DACs-----------------------------

chip 300

dac   1    8  Vdig  needed for large events
dac   2  140  Vana  for 27 mA
dac   3  130  Vsf   linearity
dac   4   12  Vcomp

dac   7   11  VwllPr  for fast Cal at 10 nTrig
dac   9   11  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim    [trim below Vcal 50 is noise]
dac  12   80  VthrComp [noisy after trim at VthrComp 50]

dac  13    1  VIBias_Bus
dac  14   14  Vbias_sf
dac  22   20  VIColOr

dac  15   66  VoffsetOp
dac  17  140  VoffsetRO
dac  18   55  VIon

dac  19   40  Vcomp_ADC
dac  20   76  VIref_ADC

dac  25  225  Vcal
dac  26  147  CalDel

dac 253    4  CtrlReg
dac 254   60  WBC

flush

show

mask

getvd
getva
getid
getia

fire 2 2
