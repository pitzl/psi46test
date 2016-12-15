
-- Daniel Pitzl, DESY, Mar 2015
-- start module: psi46digV2.1respin, TBM08b
--------------------------------------------------------------------------------
-- maximum line length: 80

log === startModRespin ===

--- set voltages and current limits

vd 3000 mV
va 1900 mV
id  900 mA
ia  600 mA

#fsel 1 # 0 = 40 MHz, 1 = 20 MHz, influences ID

--- setup timing & levels -------------------

clk  4  (base
ctr  4  (CLK +  0)
sda 19  (CLK + 15)
tin  9  (CLK +  5)

clklvl  13  # for DESY module with TBM08b and KIT adapter
ctrlvl  13  # 15 OK, 12 OK, 8 OK, 5 ineff
sdalvl  13  # D4020 in A needs 12
tinlvl  13

--- power on --------------------------------
pon
mdelay 400

resoff
mdelay 200

--- setup TBM08ab: 2 cores at E and F

#modsel 7
#modsel  9 # M4032
#modsel 15
#modsel 17 # M4066
#modsel 20  # HubID 3229
modsel 22  # HubID M3200, 3262
#modsel 23  # HubID M3255
#modsel 24  # HubID 3056
#modsel 25  # HubID 3130
#modsel 26  # HubID 20 +2
#modsel 27  # HubID 20 +1
#modsel 31 # default hubID

tbmset $E4 b11110000  Init TBM
tbmset $F4 b11110000

tbmset $E0 b10000001  Disable Auto Reset, Disable PKAM Counter
tbmset $F0 b10000001

tbmset $E2 b11000000  Mode: Calibration
tbmset $F2 b11000000

tbmset $E8 9          Set PKAM Counter (x+1)*6.4us
tbmset $F8 9

#tbmset $EA b00000000  delay_TBM_Hdr/Trl  delay_ROC_port1  delay_ROC_port0
#tbmset $FA b00000000  OK
tbmset $EA b01101101  del_tin del_TBM_Hdr/Trl  del_ROC_port1  del_ROC_port0
tbmset $FA b01101101  like pXar: OK

tbmset $EC 99         Auto reset rate (x+1)*256
tbmset $FC 99

#tbmset $EE b00100000  160/400 MHz phase adjust  OK, also cold
tbmset $EE b00101101  3 + 3 + 00 = 160 + 400 MHz phase adjust  pXar
#tbmset $EE b11101000  M3255 with mtp cable, 3056

tbmset $FE $00        Temp measurement control

mdelay 100

select 0:15  # all ROCs active

module 4022

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

dopen  30500100  0  [daq_open]
dopen  30500100  1  [daq_open]

pgsingle

flush
