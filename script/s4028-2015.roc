
smr # respin

#vb 150
#mdelay 999
#getib

module 4028

rddac ia24-trim32-lab
rdtrim ia24-trim32-lab

dac 13  90  # VIBias_Bus (max 144)
dac 22 255  # VIColOr

subvthr 4

dac 12 81 12 # harder VthrComp for ROC 12

pixt 12 18 18  7
pixt 12 18 17  8
pixt 12 18 16  7
pixt 12 18 15  6

pixt 12 17 19  8
pixt 12 17 18 10
pixt 12 17 17 15
pixt 12 17 16 15
pixd 12 17 17
pixd 12 17 16
pixt 12 17 15 10
pixt 12 17 14  6

pixt 12 16 19  9
pixt 12 16 18 11
pixt 12 16 16 15
pixd 12 16 16
pixt 12 16 15 10
pixt 12 16 14  9

pixt 12 15 19  5
pixt 12 15 18  8
pixt 12 15 17 13
pixt 12 15 16 12
pixt 12 15 15  8

pixt 12 14 18  9
pixt 12 14 17  9
pixt 12 14 16  6
pixt 12 14 15  6

pixt 15 51 78 4

modmap 20
