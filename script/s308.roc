
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
pgset 0 b101000  25  pg_resr pg_sync
pgset 1 b000100 105  pg_cal, set WBC = pg_cal - 6
pgset 2 b000010  16  pg_trg  shorter is better (caldelroc FW 2.11)
pgset 3 b000001   0  pg_tok, end of pgset

trigdel 200  # delay in trigger loop [BC], 200 = 5 us

select 0	  [set roclist, I2C]
rocaddr 0	  [set ROC]

dopen 40100100 [daq_open]

--- power on --------------------------------
pon
hvon

mdelay 500

--- set divgV2 chip 309 DACs-----------------------------

chip 308

dac   1    8  Vdig  needed for large events
dac   2  160  Vana  ia 27 mA
dac   3  130  Vsf   linearity
dac   4    0  Vcomp for eff at trim 30

dac   7    1  VwllPr
dac   9    1  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12   90  VthrComp

dac  13    1  VIBias_Bus
dac  14   14  Vbias_sf
dac  22   20  VIColOr

dac  15   64  VoffsetOp
dac  17  140  VoffsetRO
dac  18   45  VIon

dac  19   40  Vcomp_ADC
dac  20   60  VIref_ADC

dac  25  125  Vcal
dac  26  125  CalDel

dac 253    4  CtrlReg
dac 254   99  WBC (159 to get 79 pixel/DC, but not 80 = erase)

flush

show

mask

getvd
getva
getid
getia

fire 2 2
