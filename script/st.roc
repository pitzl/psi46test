
-- Daniel Pitzl, DESY, Nov 2014
-- start HDI with TBM08b
--------------------------------------------------------------------------------
-- maximum line length: 80

log === start_HDI_TBM08b ===

--- set voltages and current limits

vd 2500 mV
va 1800 mV
id  800 mA
ia  800 mA

#fsel 1 # 0 = 40 MHz, 1 = 20 MHz, influences ID

--- setup timing & levels -------------------
clk  4  (was 4 0
ctr  4  (CLK +  0)
sda 19  (CLK + 15)
tin  9  (CLK +  5)

clklvl 15  # for DESY module with TBM08b and ETH 1.2
ctrlvl 15
sdalvl 15
tinlvl 15

--- power on --------------------------------
pon
mdelay 400

resoff
mdelay 200

--- setup TBM08a: 2 cores at E and F = ch 0 and ch 1

modsel b11111  # Module address 31

tbmset $E4 $F0    Init TBM F0 + Reset ROC F4
tbmset $F4 $F0

tbmset $E0 $01    no AutoReset 80, no PKAM 01, none 81
tbmset $F0 $01

tbmset $E2 $C0    Mode C0 = Calibration
tbmset $F2 $C0

tbmset $E8  10    ROC timeout limit, unit 6.4 us
tbmset $F8  10

tbmset $EA b10000000   Delays 1+1+3+3 tin trl port port
tbmset $FA b10000000
# 5.12.2014 HDI 280: tin delay essential: b10000000 = $80

tbmset $EE b00000000    3+3+00 phase 160 and 400 MHz
tbmset $FE $03    Temperature           00

mdelay 100

flush

getid

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

#pgset 0 b010000  16  pg_rest  (reset TBM)
#pgset 1 b000100 106  pg_cal (WBC + 6 for digV2)
#pgset 2 b100010   0  pg_trg pg_sync. (delay zero = end of pgset)

pgset 0 b000100 106  pg_cal (WBC + 6 for digV2)
pgset 1 b100010   0  pg_trg pg_sync. (delay zero = end of pgset)

trigdel 400  # delay in trigger loop [BC], needed for large WBC

dopen  30500100  0  [daq_open]
dopen  30500100  1  [daq_open]

flush

mdelay 100

rd0
rd1

# pgloop 2222
# A1 = SDATA = 400 MHz, idle pattern = 5 low + 5 high looks like 40 MHz
