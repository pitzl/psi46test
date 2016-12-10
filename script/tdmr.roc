
# take random data with module

wbc 160 # time to read full DC
pgsingle # WBC needs a reset
allon
pgt
flush

#modtd 400  # trigger f = 40 MHz / period
modtd 1000  # trigger f = 40 MHz / period

mask
wbc 100 # default
pgm
pgsingle
flush
