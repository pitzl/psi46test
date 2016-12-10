
smr

#vb 150
#mdelay 999
#getib

module 4020

#rddac trim36
#rdtrim trim36

rddac ia24-trim32
rdtrim ia24-trim32

# ROC 4 DC 40/41
pixd  4 40 0:79
pixd  4 41 0:79

dac 12 91  4  # 12 = VthrComp

dac 12 95  3  # 12 = VthrComp
pixt  3  3 67 10
pixt  3 17 45 10
pixt  3 30 59 7
pixt  3 33 54 10

pixd 12 42 0:79

pixt 13 48  6 10

pixd 14 41 0:79

modmap 20
