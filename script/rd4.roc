
# DP 10/2016 read L1 module channel 4 with deser400
# L1 modules: ROC 12, 13

dreset 4
#dopen 32000 4
dsel 400
dstart 4
pgsingle
udelay(500)
dstop 4
dreadm 4
#dclose 4
