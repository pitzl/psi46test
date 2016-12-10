
# take triggered beam data with module

ctl 0 # for small Vcal gain cal

trgsel 2048  # ext
#trgdel  71  # 71 for WBC 100 in TB22 with Calice finger scint
trgdel  73  # 73 for WBC 100 in TB24

pgset 0 b100010  0  pg_trg pg_sync. (delay zero = end of pgset)

allon

flush

modtd 1 # flag for external trigger

mask
trgsel 4 # internal default

pgstop
pgset 0 b011000  16  pg_rest pg_resr  (reset TBM and ROCs)
pgset 1 b000100 106  pg_cal (WBC + 6 for digV2)
pgset 2 b100010   0  pg_trg pg_sync. (delay zero = end of pgset)
