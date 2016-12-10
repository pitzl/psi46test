
# FPGA Pattern Generator with cal (like sm)

pgstop
#pgset 0 b010000  16  pg_rest  (reset TBM)
pgset 0 b011000   16  pg_rest pg_resr  (reset TBM and ROCs)
#pgset 1 b000100  106  pg_cal (WBC + 6 for digV2)
pgset 1 b000100  105  pg_cal (WBC + 5 for PROC600
pgset 2 b100010    0  pg_trg pg_sync. (delay zero = end of pgset)

flush