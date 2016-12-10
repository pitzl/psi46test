
# DP 12/2013: read single trigger, digital ROCs

#dopen 32000
# deser 5 // now in start
dstart
udelay(500) # cures r/o errors at start
pgsingle
udelay(500)
dstop
dread
#dclose
