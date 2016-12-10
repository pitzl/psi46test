
-- 1 trigger: works for module

dac  254 100  WBC
flush

pgstop

pgset 0 b011000  16  pg_rest pg_resr (reset TBM and ROCs)

pgset 1 b000100 106  pg_cal (WBC + 6 for digV2)

pgset 2 b100010   0  pg_sync pg_trg (delay zero = end of pgset)
