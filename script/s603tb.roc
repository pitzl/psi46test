
--- from Beat Meier, PSI, Feb 2016

log === start PROC600 ===

clksrc 0
mdelay 100

resoff

--- set voltages ----------------------------
vd 2600 mV
id   65 mA
va 1800 mV
ia   65 mA

--- setup timing & levels -------------------

deser 4

clk  1  (TB21)
ctr  1  (CLK + 0)
sda 16  (CLK + 15)
tin  6  (CLK + 5)

clklvl 12
ctrlvl 12
sdalvl 12
tinlvl 12

rocaddr 0
select 0

--- power on --------------------------------
pon
mdelay 500

getvd
getva

--- program ROC -----------------------------

chip 603

dac   1   0  Vdig
dac   2  77  Vana  for 24 mA
dac   3   8  *NEW* Iph (4bit) (prev Vsf 30)
dac   4  12  Vcomp

dac   7 150  VwllPr
dac   9 150  VwllSh
dac  10 252  VhldDel

dac  11   1  Vtrim
dac  12  99  VthrComp

dac  13 100  *NEW* VColor (prev VIBias_Bus 30)
dac  22  99  VIColOr

dac  17 162  PHOffset

dac  19  50  Vcomp_ADC
dac  20  80  PHScale

dac  25 222  Vcal
dac  26  93  CalDel

dac  253 b01101  CtrlReg 13 (high cal range, StopAcq, TriggerDisable)
dac  254 100 WBC

# used in TB21 8.4.2016:
#rddac ia24-trim40
#rdtrim ia24-trim40
#dac   1   6  Vdig
#dac  12  90  VthrComp gives Vcal 66 rms 2.6

# used in TB21 Sat 9.4.2016:
#rddac ia24-vd6-trim60
#rdtrim ia24-vd6-trim60
#dac   1   2  Vdig # thr 55 Vcal

# used in TB21 Sun 10.4.2016:
rddac ia24-vd6-trim60
rdtrim ia24-vd6-trim60
dac   1   4  Vdig # thr 56 rms 1.7

flush

mask

hvon

mdelay 200
getid
mdelay 20
getia

d1  9   scope trigger (CH1)
d2 12   tout  (CH2)
a1  1    sdata (CH3)
a2  6    tout  (CH4)

--- reset ROC -------------------------------
pgstop
pgset 0 b001000   0  reset
pgsingle
mdelay 20

--- setup readout timing --------------------
pgstop
pgset 0 b001000  15  pg_resr
-pgset 0 b000000  15  no pg_resr
pgset 1 b100100 105  pg_sync, pg_cal = wbc+5
pgset 2 b000010  16  pg_trg
pgset 3 b000001   0  pg_tok, end of pgset

trigdel 200  # delay in trigger loop [BC], 200 = 5 us

dopen 40100100 0 [daq_open]

flush

fire 2 2
