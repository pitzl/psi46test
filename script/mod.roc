
# module boot procedure 2015, 2016

smr   // respin ROCs
hvon  // DTB
vb 150 // iseg

# set analog current per ROC [mA]:
optiamod 24

# set timing:
ctl 4
modcaldel 22 33

# response map:
modmap 20

# set global treshold to Vcal target
modvthrcomp 35

# set timing:
modcaldel 22 33

# set Vtrim to Vcal target: 30 is too noisy
modvtrim 32

# scale trim bits to Vcal target:
modtrimscale 32

# iterate trim bits to Vcal target:
modtrimstep 32
modtrimstep 32
modtrimstep 32

# set timing:
modcaldel 22 33

# adjust noisy pixels:
pgt # without resets. like tdmr
modtrimbits 99
pgm

# response map:
modmap 20

# rthreshold map:
modthrmap 66

# PH tuning:
ctl 4
modtune 22 33

# save:
wdac trim30
wtrim trim30

# gaincal:
ctl 4
dacscanmod 25 16

ctl 0
dacscanmod 25 16

# BBtest:
ctl 4
  optiamod 30  # better separation
  modcaldel 22 33
dacscanmod 12 -16 2

# bump height:
#ctl 4
#dacscanmod 25 -16 2
