
# ROC take data with random trigger

pgstop
# just trig and tok, no reset, no cal:
pgset 0 b000010  16  pg_trg
pgset 1 b000001   0  pg_tok, end of pgset

#trigdel 200  # delay in trigger loop [BC], 200 = 5 us
trigdel 400  # delay in trigger loop [BC], 400 = 10 us

allon

#stretch 1 7 999  # for psi46digV2
#stretch 1 8  100 # for psi46digV2.1

takedata 1000 (f = 40 MHz/period)

pgstop
pgset 0 b001000  15  pg_resr
pgset 1 b100100 105  pg_sync, pg_cal = wbc+5
pgset 2 b000010  16  pg_trg
pgset 3 b000001   0  pg_tok, end of pgset

mask
stretch 0 0 0
