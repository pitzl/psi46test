
# high rate test pulses irrad

pgstop

mask
arm 0:51 71:78  (8 px/col, 16 px/DC, 416 pix)

# d1 1 (40 MHz clk on D1)
# d1 4 (token on D1, see pixel_dtb.h)
  d1 5 (trigg on D1, see pixel_dtb.h)
  d2 6 (cal   on D2, see pixel_dtb.h)
# d1 7 (reset on D1, see pixel_dtb.h)
# d1 9 (sync  on D1, see pixel_dtb.h)

#dac   1  15  Vdig  needed for large events

dac 254  50  WBC (minimum on 312i

pgset  0 b101000   8  pg_resr pg_sync

pgset  1 b000100  40  pg_cal
pgset  2 b000100  40  pg_cal
pgset  3 b000100  40  pg_cal
pgset  4 b000100  40  pg_cal
pgset  5 b000100  40  pg_cal
pgset  6 b000100  40  pg_cal
pgset  7 b000100  40  pg_cal

pgset  8 b000100  56  pg_cal, WBC + 6 for digV2
#gset  8 b000100  57  pg_cal, WBC + 7 for digV2.1
pgset  9 b000010  20  pg_trg  
pgset 10 b000001   0  pg_tok, end of pgset

single

getid2 0 (clear)

pgloop 500 (f = 40 MHz/period = 125 kHz trigger rate)
mdelay 999 (activity = 8*8*52*40/f)
getid2 1
