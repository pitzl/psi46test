
-- Daniel Pitzl, DESY, Oct 2016
-- start L1 module: proc600v2, TBM10c, FW4.2
--------------------------------------------------------------------------------
-- maximum line length: 80

log === startModRespin ===

--- set voltages and current limits

vd 2900 mV
va 1900 mV
id  900 mA
ia  600 mA

#fsel 1 # 0 = 40 MHz, 1 = 20 MHz, influences ID

--- setup timing & levels -------------------

#clk 17  (Beat
#ctr 17  (CLK +  0)
#sda 32  (CLK + 15)
#tin 22  (CLK +  5)

clk  4  (pXar
ctr  4  (CLK +  0)
sda 19  (CLK + 15)
tin  9  (CLK +  5)

clklvl  15
ctrlvl  15
sdalvl  15
tinlvl  15

--- power on --------------------------------
pon
mdelay 400

resoff
mdelay 200

--- setup TBM10c: 2 cores at E and F

module 1039 # must be 1000..1099 for L1 modules with 2 TBMs

#modsel2 30  # set HubID (and i+1)
modsel 30  # set HubID

select 0:15  # all ROCs active

seltbm 0 # ROCs 0-4 and 12-15
tbm10

seltbm 1 # ROCs 4-7 and 8-11
tbm10

mdelay 100

chip 700  # must be after select

dac   1   12  Vdig 
dac   2   99  Vana
dac   3    8  Iph (4 bit)
dac   4   12  Vcomp

dac   7  160  VwllPr
dac   9  160  VwllPr

dac  11    1  Vtrim
dac  12  111  VthrComp

dac  13   99  VColor

dac  17  165  PHOffset

dac  19   10  Vcomp_ADC
dac  20  100  PHScale

dac  22  99  VIColor (removed?)

dac  25  255  Vcal
dac  26  122  CalDel

dac  253 b101  CtrlReg 5 (high cal range, StopAcq, TriggerDisable)
dac  254 100  WBC    // tct - 6
dac  255  12  RBreg

flush

cald  # ClrCal
mask

hvon
mdelay 2000

getid
getia
getib

--- setup probe outputs ---------------------

  d1 1 (40 MHz clk on D1)
# d1 4 (token on D1, see pixel_dtb.h)
  d2 5 (trigg on D2, see pixel_dtb.h)
# d1 6 (cal   on D2, see pixel_dtb.h)
# d1 7 (reset on D1, see pixel_dtb.h)
# d1 9 (sync  on D1, see pixel_dtb.h)

a1 1  sdata # 400 MHz

--- rctk pattern generator:

- pixel_dtb.h:	#define PG_RESR  0x0800 // ROC reset
- pixel_dtb.h:	#define PG_REST  0x1000 // TBM reset

pgstop
pgset 0 b011000  16  pg_rest pg_resr  (reset TBM and ROCs)
pgset 1 b000100 105  pg_cal (WBC + 5 for PROC600)
pgset 2 b100010   0  pg_trg pg_sync. (delay zero = end of pgset)

#trigdel 200  # delay in trigger loop 200 BC =  5 us
trigdel 400  # delay in trigger loop 400 BC = 10 us for large events
#trigdel 0  # delay in trigger loop [BC] Simon says: should work

dopen  5100200  0  [daq_open]
dopen  5100200  1  [daq_open]
dopen  5100200  2  [daq_open]
dopen  5100200  3  [daq_open]
dopen  5100200  4  [daq_open]
dopen  5100200  5  [daq_open]
dopen  5100200  6  [daq_open]
dopen  5100200  7  [daq_open]

pgsingle

flush
