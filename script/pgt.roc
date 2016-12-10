
# FPGA Pattern Generator with trigger

pgstop
pgset 0 b100000   15  pg_sync
pgset 1 b000010    0  pg_trg (delay zero = end of pgset)
flush