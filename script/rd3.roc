
# DP 10/2016 read L2 module channel 3 with deser400
# L1 modules: ROC 10, 11

dreset 3
#dopen 32000 3
dsel 400
dstart 3
pgsingle
udelay(500)
dstop 3
dreadm 3
#dclose 3
