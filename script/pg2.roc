
-- 2 triggers: works for module

dac  254 100  WBC
flush

pgstop

pgset 0 b011000  16  pg_rest pg_resr (reset TBM and ROCs)

pgset 1 b000100  53  pg_cal (WBC + 6 for digV2)

pgset 2 b100010  52  pg_trg pg_sync

pgset 3 b100010   0  pg_trg pg_sync (delay zero = end of pgset)
