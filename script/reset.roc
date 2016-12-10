
# reset ROCs and TBM, for tdmt

pgset 0 b011000  0  pg_rest pg_resr  (reset TBM and ROCs)
pgsingle
pgset 0 b100010  0  pg_trg pg_sync. (delay zero = end of pgset)
