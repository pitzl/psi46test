
--- start PROC600v2, Jul 2016

--- DTB FW/SW 4.2/4.2

--- 2 dead pix

log === start PROC600v2 ===

clksrc 0
mdelay 100

resoff

--- set voltages and current limits:

vd 2650 mV
va 1800 mV
id  100 mA
ia  100 mA

--- setup timing & levels:

deser 4

clk  4
ctr  4  (CLK + 0)
sda 19  (CLK + 15)
tin  9  (CLK + 5)

clklvl 10
ctrlvl 10
sdalvl 10
tinlvl 10

rocaddr 0
select 0

--- power on:
pon
mdelay 500

getvd
getva

--- program ROC -----------------------------

chip 703

dac   1  12  Vdig (for caldelroc)
dac   2  76  Vana for 24 mA
dac   3   8  Iph (4bit)
dac   4  12  Vcomp

dac   7 150  VwllPr
dac   9 150  VwllSh

dac  11   1  Vtrim
dac  12  99  VthrComp

dac  13 100  VColor

dac  17 162  PHOffset

dac  19  50  Vcomp_ADC
dac  20  80  PHScale

dac  25 222  Vcal
dac  26  93  CalDel

dac  253 b101  CtrlReg 5 (high cal range, StopAcq, TriggerDisable)
dac  254 100 WBC

# 22.7.2016 e-lab:

#rddac trim35
#rdtrim trim35
# thr 37+-2

rddac trim40
rdtrim trim40

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

--- reset ROC once:

pgstop
pgset 0 b001000   0  reset
pgsingle
mdelay 20

--- pattern generator:

pgstop
pgset 0 b001000  15  pg_resr
pgset 1 b100100 105  pg_sync, pg_cal = wbc+5
pgset 2 b000010  16  pg_trg
pgset 3 b000001   0  pg_tok, end of pgset

trigdel 200  # delay in trigger loop [BC], 200 = 5 us

dopen 40100100 0 [daq_open]

flush

fire 2 2
