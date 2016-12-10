
-- 1.9.2014 at FEC: bump bond test
-- sfec

optia 30

vcal 255
ctl 4
caldel 22 33
effmap 100

# check threshold:

ctl 0
effdac 22 33 25

# adjust PH range:

ctl 4
tune 22 33

# for bump height, threshold, noise:

ctl 0
# dacscanroc 25 10 2
dacscanroc 25 25 1 127

# bb test:

ctl 4
dacdac 22 33 26 12  # cals, sets CalDel and VthrComp

dacscanroc 12 -10   # cals, scan VthrComp

# bb height:

dacscanroc 25 -25 2
