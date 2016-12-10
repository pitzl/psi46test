
-- 13.8.2014 at FEC: bump bond height test
-- sfec

optia 30

vcal 255

ctl 4
caldel 22 33
effmap 100

tune 22 33

# check threshold:

ctl 0
phdac 22 33 25

# PH vs small Vcal:

#dacscanroc 25 10 2  # cal vs Vcal
dacscanroc 25 25 1 127

# bb height:

ctl 4
dacscanroc 25 -25 2  # cals vs Vcal
