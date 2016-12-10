
# take random beam data with module

pgset 0 b100010  0  pg_trg pg_sync. (delay zero = end of pgset)

allon

flush

modtd 400  # trigger f = 40 MHz / period

mask
trgsel 4 # internal default

pgstop
pgset 0 b011000  16  pg_rest pg_resr  (reset TBM and ROCs)
pgset 1 b000100 106  pg_cal (WBC + 6 for digV2)
pgset 2 b100010   0  pg_trg pg_sync. (delay zero = end of pgset)
