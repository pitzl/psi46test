
log === set DTB parameters ===

resoff

- supply voltages and current limits:

vd 2500 mV
va 1800 mV
id  100 mA
ia  100 mA

- timing & levels:
clk  5
ctr  5  (CLK +  0)
sda 20  (CLK + 15)
tin 10  (CLK +  5)

clklvl 10
ctrlvl 10
sdalvl 10
tinlvl 10

--- pattern generator: set redout timing --------------------
pgstop
pgset 0 b101000  25  pg_resr pg_sync
pgset 1 b000100 105  pg_cal, set WBC = pg_cal - 5
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

--- set divg chip 205 DACs-----------------------------

chip 205

rowinvert

dac   1   15  Vdig  beam test 2013 run 6656 or 10891
dac   2   81  Vana  ia 25 mA
dac   3   30  Vsf   linearity
dac   4   12  Vcomp

dac   7   60  VwllPr
dac   9   60  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12   72  VthrComp

dac  13   20  VIBias_Bus
dac  14   14  Vbias_sf

dac  15   50  VoffsetOp
dac  17  140  VoffsetRO
dac  18   55  VIon

dac  19  100  Vcomp_ADC
dac  20   70  VIref_ADC
dac  22  100  VIColOr

dac  25  222  Vcal
dac  26  109  CalDel

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
