
# 205i, chiller -23

# test beam, 1 ft cable

log === set DTB parameters ===

resoff

- supply voltages and current limits:

vd 2600 mV
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
pgset 1 b000100 106  pg_cal = WBC + 7
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

chip 205

rowinvert

dac   1    6  Vdig, low against DCF
dac   2  128  Vana 27 mA
dac   3   30  Vsf   linearity
dac   4   12  Vcomp 

dac   7    0  VwllPr
dac   9    0  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12   80  VthrComp

dac  13    1  VIBias_Bus
dac  14   14  Vbias_sf

dac  15   67  VoffsetOp
dac  17  140  VoffsetRO
dac  18   45  VIon

dac  19   10  Vcomp_ADC
dac  20   79  VIref_ADC
dac  22   20  VIColOr

dac  25  222  Vcal
dac  26  154  CalDel

dac 253    4  CtrlReg
dac 254   99  WBC = tct - 7
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
