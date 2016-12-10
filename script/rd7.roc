
# DP 10/2016 read L1 module channel 7 with deser400
# L1 modules: ROC 2, 3

dreset 7
#dopen 32000 7
dsel 400
dstart 7
pgsingle
udelay(500)
dstop 7
dreadm 7
#dclose 7
