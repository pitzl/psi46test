
# standard pg rof ROC with reset cal trg tok:

pgstop
pgset 0 b101000  16  pg_resr pg_sync
pgset 1 b000100 106  pg_cal, set WBC = pg_cal - 6
pgset 2 b000010  16  pg_trg  
pgset 3 b000001   0  pg_tok, end of pgset
