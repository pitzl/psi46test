
# take module data with external trigger

dstart

trgsel 256 # ext

mdelay 2000

dstop

dreadm # module

trgsel 4 # default internal
