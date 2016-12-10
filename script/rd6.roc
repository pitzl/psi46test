
# DP 10/2016 read L1 module channel 6 with deser400
# L1 modules: ROC 0, 1

dreset 6
#dopen 32000 6
dsel 400
dstart 6
pgsingle
udelay(500)
dstop 6
dreadm 6
#dclose 6
