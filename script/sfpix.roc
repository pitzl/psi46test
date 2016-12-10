
# 2015
# start FPix module: hubid 15

--------------------------------------------------------------------------------
-- maximum line length: 80

log === startdigmod ===

--- set voltages and current limits

vd 2600 mV
va 1800 mV
id  800 mA
ia  800 mA

--- setup timing & levels -------------------
clk  4  (5 is bad in modcaldel, 3 is best in modtd
ctr  4  (CLK +  0)
sda 19  (CLK + 15)
tin  9  (CLK +  5)

clklvl 10  # 15 needed for ETH adapter 1.2 from 2014
ctrlvl 10
sdalvl 10
tinlvl 10

--- power on --------------------------------
pon
mdelay 400
hvon
mdelay 400
resoff
mdelay 200

--- setup TBM08b: 2 cores at E and F

modsel b01111  # FPix Module address 15

tbmset $E4 $F0    Init TBM, Reset ROC
tbmset $F4 $F0

tbmset $E0 $00    Disable PKAM Counter  81 ETH
tbmset $F0 $00                          81

tbmset $E2 $C0    Mode = Calibration
tbmset $F2 $C0

tbmset $E8 $02    Set PKAM Counter   10
tbmset $F8 $02                       10

tbmset $EA b00000000 Delays         E4
tbmset $FA b00000000                E4

tbmset $EE $00    phase 400 and 160 MHz, once  20
tbmset $FE $00    Temperature                     00


mdelay 100

select 0:15  # all ROCs active
--select 7:8  # test

chip 450  # digV2p1, must be after select

dac   1    8  Vdig 
dac   2  110  Vana
dac   3   33  Vsf
dac   4   12  Vcomp

dac   7  160  VwllPr
dac   9  160  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12   90  VthrComp

dac  13    1  VIBias_Bus
dac  22   20  VIColOr

dac  17  150  PHOffset

dac  19   30  Vcomp_ADC
dac  20  111  PHScale

dac  25  222  Vcal
dac  26  111  CalDel

dac  253   4  CtrlReg
dac  254  50  WBC    // tct - 7 for digV2p1
dac  255  12  RBreg

flush

cald  # ClrCal
mask

getid
getia

--- setup probe outputs ---------------------

# d1 1 (40 MHz clk on D1)
# d1 4 (token on D1, see pixel_dtb.h)
  d2 5 (trigg on D1, see pixel_dtb.h)
  d1 6 (cal   on D2, see pixel_dtb.h)
# d1 7 (reset on D1, see pixel_dtb.h)
# d1 9 (sync  on D1, see pixel_dtb.h)

a1 1  sdata # 400 MHz

--- rctk pattern generator:

- pixel_dtb.h:	#define PG_RESR  0x0800 // ROC reset
- pixel_dtb.h:	#define PG_REST  0x1000 // TBM reset

pgstop
pgset 0 b010000  16  pg_rest  (reset TBM) # reset erases delay registers
#pgset 0 b011000  16  pg_rest pg_resr  (reset TBM and ROCs) # erases delays
pgset 1 b000100   57  pg_cal (WBC + 7 for digV2.1)
pgset 2 b100010   0  pg_trg pg_sync. (delay zero = end of pgset)

trigdel 400  # delay in trigger loop [BC], needed for large WBC

#pgsingle

dopen  30500100  0  [daq_open]
dopen  30500100  1  [daq_open]

flush
