
-- Daniel Pitzl, DESY, Oct 2014
-- start module DESY: psi46digV2, TBM08b
--------------------------------------------------------------------------------
-- maximum line length: 80

log === startdigmod ===

--- set voltages and current limits

vd 2600 mV
va 1800 mV
id  800 mA
ia  800 mA

#fsel 1 # 0 = 40 MHz, 1 = 20 MHz, influences ID

--- setup timing & levels -------------------
clk  4  (was 4
ctr  4  (CLK +  0)
sda 19  (CLK + 15)
tin  9  (CLK +  5)

clklvl  4  # for DESY module with TBM08b and KIT V2
ctrlvl  4
sdalvl  4
tinlvl  4

--- power on --------------------------------
pon
mdelay 400
hvon
mdelay 400

resoff
mdelay 200

--- setup TBM08: 2 cores at E and F

modsel b11111  # Module address 31

tbmset $E4 $F0    Init TBM, Reset ROC
tbmset $F4 $F0

tbmset $E0 $01    Disable PKAM Counter  81
tbmset $F0 $01                          81

tbmset $E2 $C0    Mode = Calibration
tbmset $F2 $C0

tbmset $E8 $10    Set PKAM Counter      02
tbmset $F8 $10                          02

tbmset $EA $00    Delays                E4
tbmset $FA $00                          E4

tbmset $EE $00    phase 400 and 160 MHz 20
tbmset $FE $00    Temperature           00

mdelay 100

-- digV2 ROCs: DCF problem

select 0:15  # all ROCs active

module 4016

chip 350  # must be after select

dac   1    8  Vdig 
dac   2  140  Vana
dac   3  130  Vsf
dac   4   12  Vcomp

dac   7   10  VwllPr
dac   9   10  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12   40  VthrComp

dac  13    1  VIBias_Bus
dac  14   14  Vbias_sf
dac  22   20  VIColOr

dac  15   60  VoffsetOp
dac  17  150  VoffsetRO
dac  18   45  VIon

dac  19   30  Vcomp_ADC
dac  20   70  VIref_ADC

dac  25  222  Vcal
dac  26  122  CalDel

dac  253   4  CtrlReg
dac  254  50  WBC    // tct - 6
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
pgset 1 b000100  56  pg_cal (WBC + 6 for digV2)
pgset 2 b100010   0  pg_trg pg_sync. (delay zero = end of pgset)

trigdel 400  # delay in trigger loop [BC], needed for large WBC

#pgsingle

dopen  30500100  0  [daq_open]
dopen  30500100  1  [daq_open]

flush
