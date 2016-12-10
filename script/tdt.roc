
# ROC take data with external trigger

trgsel 256 # ext trigger
clksrc 1 # ext clk

#trgdel 46 # 703 TB21 with WBC 100 empty
trgdel 47 # 703 TB21 with WBC 100
#trgdel 48 # 703 TB21 with WBC 100 empty

#trgtimeout 60000  #  wait for ROC token out

pgstop
# just trig and tok, no reset, no cal:
pgset 0 b000010  16  pg_trg
pgset 1 b000001   0  pg_tok, end of pgset

allon

takedata 1 # flag

pgstop
pgset 0 b001000  15  pg_resr
#pgset 1 b100100 105  pg_sync, pg_cal = wbc+5 for 703
pgset 1 b100100 106  pg_sync, pg_cal = wbc+6 for 507
pgset 2 b000010  16  pg_trg
pgset 3 b000001   0  pg_tok, end of pgset

clksrc 0 # internal clock

mask

trgsel 4 # internal default

flush
