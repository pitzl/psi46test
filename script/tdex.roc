
# take data with external trigger

dstart

trgsel 256 # ext

mdelay 5000

dstop

dread

trgsel 4 # default internal
