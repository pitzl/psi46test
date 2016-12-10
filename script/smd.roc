
-- Daniel Pitzl, DESY, Oct 2014
-- start module DESY: psi46digV2, TBM08b
--------------------------------------------------------------------------------
-- maximum line length: 80

log === startdigmod ===

--- set voltages and current limits

vd 2800 mV # needed for TBM08b
va 1800 mV
id  900 mA
ia  600 mA

#fsel 1 # 0 = 40 MHz, 1 = 20 MHz, influences ID

--- setup timing & levels -------------------

clk  4  (base
ctr  4  (CLK +  0)
sda 19  (CLK + 15)
tin  9  (CLK +  5)

clklvl 13  # for DESY module with TBM08b and KIT adapter
ctrlvl 13  # 15 OK, 12 OK, 8 OK, 5 ineff
sdalvl 13  # D4020 in A needs 13
tinlvl 13

--- power on --------------------------------
pon
mdelay 400

resoff
mdelay 200

--- setup TBM08abc: 2 cores at E and F

modsel b11111  # Module address 31

tbmset $E4 b11110000  Init TBM
tbmset $F4 b11110000

tbmset $E0 b10000001  Disable Auto Reset, Disable PKAM Counter
tbmset $F0 b10000001

tbmset $E2 b11000000  Mode: Calibration
tbmset $F2 b11000000

tbmset $E8 9          Set PKAM Counter (x+1)*6.4us
tbmset $F8 9

tbmset $EC 9         Auto reset rate (x+1)*256
tbmset $FC 9


#tbmset $EA b10000000  delay_tin: no data
#tbmset $FA b01000000  delay_TBM_Hdr/Trl: no data

#tbmset $EA b01100100  delay_TBM_Hdr/Trl  delay_ROC_port1  delay_ROC_port0
#tbmset $FA b01100100  like pXar: OK, but extra data after ROC 7, 15

#tbmset $EA b01110110  delay_TBM_Hdr/Trl  delay_ROC_port1  delay_ROC_port0
#tbmset $FA b01110110  OK, but extra data after ROC 7, 15

#tbmset $EA b11110110  delay_tin  delay_TBM_Hdr/Trl  delay_ROC_port1  port0
#tbmset $FA b11110110  OK, but extra data after ROC 7, 15

tbmset $EA b00000000  delay_TBM_Hdr/Trl  delay_ROC_port1  delay_ROC_port0
tbmset $FA b00000000  OK, but extra data after ROC 7, 15


#tbmset $EE b00000000  160/400 MHz phase adjust  OK
tbmset $EE b00100000  160/400 MHz phase adjust  OK, also cold
#tbmset $EE b01000000  160/400 MHz phase adjust  some ROCs missing
#tbmset $EE b10000000  160/400 MHz phase adjust  some ROCs missing

tbmset $FE $00        Temp measurement control

#pXar TBM08b settings 10.2.2015
#    base4 1111 0000   clear clear clear reset
#    base0 1000 0001   disable_auto_reset    disable_PKAM_counter
#    base2 11 000000   mode_cal
#    base8 00010000    PKAM counter
#    baseA 01 100 100  delay_TBM_head_trail  delay_ROC_port1  delay_ROC_port0
#    baseC 0000 0000   auto_reset
#    baseE 000 000 00  phase_160  phase_400

mdelay 100

-- digV2 ROCs: DCF problem

select 0:15  # all ROCs active

module 4000

chip 351  # must be after select

dac   1    8  Vdig 
dac   2  160  Vana
dac   3  130  Vsf
dac   4   12  Vcomp

dac   7   10  VwllPr
dac   9   10  VwllPr
dac  10  252  VhldDel

dac  11    1  Vtrim
dac  12   50  VthrComp

dac  13    1  VIBias_Bus
dac  14   14  Vbias_sf
dac  22   20  VIColOr

dac  15   60  VoffsetOp
dac  17  150  VoffsetRO
dac  18   45  VIon

dac  19    1  Vcomp_ADC
dac  20   70  VIref_ADC

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
#pgset 0 b011000  16  pg_rest pg_resr  (reset TBM and ROCs)
#pgset 1 b000100 106  pg_cal (WBC + 6 for digV2)
#pgset 2 b100010   0  pg_trg pg_sync. (delay zero = end of pgset)
pgset 0 b100010   0  pg_trg pg_sync. (delay zero = end of pgset)

#trigdel  200  # delay in trigger loop [BC]
trigdel  400  # delay in trigger loop [BC], needed for large WBC
#trigdel 1000  # delay in trigger loop [BC], needed for large WBC

dopen  30500100  0  [daq_open]
dopen  30500100  1  [daq_open]

pgsingle

flush

# vd 2800
# clk 4 -4
# pgloop 2222
# A1 = SDATA = 400 MHz, idle pattern = 5 low + 5 high looks like 40 MHz
