
-- Daniel Pitzl, DESY, Oct 2016
-- start L2 module: psi46digV2.1respin, TBM09c
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

clk  4  (base
ctr  4  (CLK +  0)
sda 19  (CLK + 15)
tin  9  (CLK +  5)

clklvl  13  # 
ctrlvl  13  # 
sdalvl  13  # 
tinlvl  13

--- power on --------------------------------
pon
mdelay 400

resoff
mdelay 200

--- setup TBM09c: 2 cores at E and F

#modsel 31  # set default HubID
modsel 22 # L2o short
#modsel 24  # M2229 HubID
#modsel 24  # 

#pxar:
#base0   0x81 = 8000'0001
#base2   0xC0 = 1100'0000
#base4   0xF0 = 1111'0000
#base8   0x10 = 16
#basea   0x64 = 0110'0100
#basec   0x00
#basee   0x34 = 0011'0100

tbmset $E4 b11110000  Init TBM
tbmset $F4 b11110000

tbmset $E0 b10000001  Disable Auto Reset, Disable PKAM Counter
tbmset $F0 b10000001

tbmset $E2 b11000000  Mode: Calibration
tbmset $F2 b11000000

tbmset $E8 9          Set PKAM Counter (x+1)*6.4us
tbmset $F8 9

tbmset $EA b01101101  del_tin del_TBM_Hdr/Trl  del_ROC_port1  del_ROC_port0
tbmset $FA b01101101  like 08c

tbmset $EC 99         Auto reset rate (x+1)*256
tbmset $FC 99

tbmset $EE b00110100  3 + 3 + 00 = 160 + 400 MHz phase adjust  pXar

tbmset $FE $00        Temp measurement control

mdelay 100

select 0:15  # all ROCs active

module 2022

chip 500  # must be after select

dac   1    8  Vdig 
dac   2  110  Vana
dac   3   30  Vsh
dac   4   12  Vcomp

dac   7  160  VwllPr
dac   9  160  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12   90  VthrComp

dac  13   30  VIBias_Bus
dac  22   99  VIColOr


dac  17  165  PHOffset

dac  19   10  Vcomp_ADC
dac  20  100  PHScale

dac  25  255  Vcal
dac  26  122  CalDel

dac  253   4  CtrlReg
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
pgset 1 b000100 106  pg_cal (WBC + 6 for digV2)
pgset 2 b100010   0  pg_trg pg_sync. (delay zero = end of pgset)

#trigdel 200  # delay in trigger loop 200 BC =  5 us
trigdel 400  # delay in trigger loop 400 BC = 10 us for large events
#trigdel 0  # delay in trigger loop [BC] Simon says: should work

dopen  15100200  0  [daq_open]
dopen  15100200  1  [daq_open]
dopen  15100200  2  [daq_open]
dopen  15100200  3  [daq_open]

pgsingle

flush
