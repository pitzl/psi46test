
# take source data with module
# sm
# modcaldel 2 2

dac  254  50  # WBC 160 gives no hits ?

pgstop
pgset 0 b010000  16  # pg_rest (reset TBM)
pgset 1 b100000  56  # pg_sync (stretch after sync)
pgset 2 b000010   0  # pg_trg (delay zero = end of pgset)

# stretch 1 8 999 # clock stretch does not work for modules

ctl 0 # for small Vcal gain cal

allon
flush
#modtd 200  # trigger f = 40 MHz / period
modtd 2000  # for direct rays

mask
