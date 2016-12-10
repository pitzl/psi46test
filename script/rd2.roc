
# DP 10/2016 read L2 module channel 2 with deser400
# L1 modules: ROC 8, 9

dreset 3
#dopen 32000 2
dsel 400
dstart 2
pgsingle
udelay(500)
dstop 2
dreadm 2
#dclose 2
