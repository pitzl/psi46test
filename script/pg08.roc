
# high rate test pulses

pgstop

mask
arm 0:51 11:18  (8 px/col, 16 px/DC, 416 pix)

# d1 1 (40 MHz clk on D1)
# d1 4 (token on D1, see pixel_dtb.h)
  d1 5 (trigg on D1, see pixel_dtb.h)
  d2 6 (cal   on D2, see pixel_dtb.h)
# d1 7 (reset on D1, see pixel_dtb.h)
# d1 9 (sync  on D1, see pixel_dtb.h)

dac   1  15  Vdig  needed for large events

dac 254  37  WBC (37 is minimum with 16 px/DC)

pgset  0 b101000   8  pg_resr pg_sync

pgset  1 b000100  33  pg_cal  delay 33 BC
pgset  2 b000100  33  pg_cal  need 2 BC / px
pgset  3 b000100  33  pg_cal  time for 16 px / DC = 8 px / col
pgset  4 b000100  33  pg_cal
pgset  5 b000100  33  pg_cal
pgset  6 b000100  33  pg_cal
pgset  7 b000100  33  pg_cal

pgset  8 b000100  43  pg_cal, WBC + 6 for digV2
#pgset  8 b000100  44  pg_cal, WBC + 7 for digV2.1
pgset  9 b000010  20  pg_trg  
pgset 10 b000001   0  pg_tok, end of pgset

single

getid2 0 (clear)

pgloop 320 (f = 40 MHz/period = 125 kHz trigger rate)
mdelay 999 (activity = 8*8*52*40/f)
getid2 1

pgloop 333 (act 400)
mdelay 999
getid2 1

pgloop 341 (act 390)
mdelay 999
getid2 1

pgloop 350 (act 380)
mdelay 999
getid2 1

pgloop 360 (act 370)
mdelay 999
getid2 1

pgloop 370 (act 360)
mdelay 999
getid2 1

pgloop 380 (act 350)
mdelay 999
getid2 1

pgloop 392 (act 340)
mdelay 999
getid2 1

pgloop 403 (act 330)
mdelay 999
getid2 1

pgloop 416 (act 320)
mdelay 999
getid2 1

pgloop 429 (act 310)
mdelay 999
getid2 1

pgloop 444 (act 300)
mdelay 999
getid2 1

pgloop 459 (act 290)
mdelay 999
getid2 1

pgloop 475 (act 280)
mdelay 999
getid2 1

pgloop 493 (act 270)
mdelay 999
getid2 1

pgloop 512 (act 260)
mdelay 999
getid2 1

pgloop 532 (act 250)
mdelay 999
getid2 1

pgloop 555 (act 240)
mdelay 999
getid2 1

pgloop 579 (act 230)
mdelay 999
getid2 1

pgloop 605 (act 220)
mdelay 999
getid2 1

pgloop 634 (act 210)
mdelay 999
getid2 1

pgloop 666 (act 200)
mdelay 999
getid2 1

pgloop 701 (act 190)
mdelay 999
getid2 1

pgloop 740 (act 180)
mdelay 999
getid2 1

pgloop 783 (act 170)
mdelay 999
getid2 1

pgloop 832 (act 160)
mdelay 999
getid2 1

pgloop 887 (act 150)
mdelay 999
getid2 1

pgloop 951 (act 140)
mdelay 999
getid2 1

pgloop 1024 (act 130)
mdelay 999
getid2 1

pgloop 1109 (act 120)
mdelay 999
getid2 1

pgloop 1210 (act 110)
mdelay 999
getid2 1

pgloop 1331 (act 100)
mdelay 999
getid2 1

pgloop 1479 (act  90)
mdelay 999
getid2 1

pgloop 1664 (act  80)
mdelay 999
getid2 1

pgloop 1901 (act  70)
mdelay 999
getid2 1

pgloop 2219 (act  60)
mdelay 999
getid2 1

pgloop 2662 (act  50)
mdelay 999
getid2 1

pgloop 3328 (act  40)
mdelay 999
getid2 1

pgloop 4437 (act  30)
mdelay 999
getid2 1

pgloop 6656 (act  20)
mdelay 999
getid2 1

pgloop 13312 (act  10)
mdelay 999
getid2 1

pgstop       (act   0)
mdelay 999
getid2 1

getid2 2 (print for ROOT)
