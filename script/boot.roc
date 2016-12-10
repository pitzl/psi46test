
# bootstrap ROC: set DACs

s603tb

optia 24         # set Vana for target Ia [mA]

ctl 4
caldel 44  4     # set CalDel (from one pixel)
caldelroc

ctl 0
effdac 22 33 25  # one pix threshold, shows start value

vthrcomp 48 55   # set global threshold to min 36, start 55

ctl 4
caldel 44  4     # set CalDel (from one pixel)
caldelroc

trim 40 55       # trim to 36, start guess 48

caldel 44  4
caldelroc

effmap 100

-- adjust trim bits for low eff pixels:

trimbits

-- pixi col row
-- pixt col row trim

thrmap 40

ctl 4
tune 44 4       # set gain and offset

phmap 100

show
ctl 4
wdac ia24-trim30
wtrim ia24-trim30

-- gain cal:

ctl 4
dacscanroc 25 16
ctl 0
dacscanroc 25 16
