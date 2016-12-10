
-- 3 triggers: works for module

dac  254 100  WBC
flush

pgstop

pgset 0 b011000  16  pg_rest pg_resr (reset TBM and ROCs)

pgset 1 b000100  50  pg_cal (WBC + 6 for digV2)

pgset 2 b000010  55  pg_trg

pgset 3 b100010  80  pg_sync pg_trg (delay zero = end of pgset)

pgset 4 b000010   0  pg_trg
