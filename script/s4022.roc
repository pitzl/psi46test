
smr # respin

#vb 150
#mdelay 999
#getib

module 4022

#rddac trim36
#rdtrim trim36
#rddac trim32
#rdtrim trim32
rddac trim32-tb
rdtrim trim32-tb

subvthr 2

pixt  2  5 32 7
pixt  2 51 79 7
pixt  3  0 79 7
pixt  3 51 79 8
pixt  4 51 79 11
pixt  5  0 79 6
pixt  5  5 67 7
pixt  7 20 52 8
pixt 10 51 79 6
pixt 11 28 63 7
pixt 11 51 79 7
pixt 11  1 79 7
pixt 11  0 77 6
pixt 13  0 41 7
pixt 13  0 79 7
pixt 13  1 64 9
pixt 13 11 32 9
pixt 14  0 78 9
pixt 14  3 69 7
pixt 14 13 27 7
pixt 14 51 79 11

dac 13 60 # VIBias_Bus 90 is noisy

modmap 20
