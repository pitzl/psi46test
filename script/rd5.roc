
# DP 10/2016 read L1 module channel 5 with deser400
# L1 modules: ROC 14, 15

dreset 5
#dopen 32000 5
dsel 400
dstart 5
pgsingle
udelay(500)
dstop 5
dreadm 5
#dclose 5
