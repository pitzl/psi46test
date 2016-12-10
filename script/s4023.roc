
smr # respin

module 4023

#rddac ia24-trim32-tb
#rdtrim ia24-trim32-tb
#pixt  8 40 22 15
#pixt 10 51 34 11
#pixt 15 31 11  7
#pixt 15 47 72 11
#pixt 15 47 73  7

rddac trim32
rdtrim trim32
dac 13  90  # VIBias_Bus (max 124)
dac 22 255  # VIColOr
subvthr 4
pixt  8 40 22 16  # 16 = mask
pixt 10 51 34 11
pixt 15 47 72 15
pixt 11 47 17  9

modmap 20
