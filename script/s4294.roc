
smr # respin

vd 3000

module 4294

# 15.4.2016 FullQual p17
# 26.5.2016 TB24 modtrimbits and modtune

rddac trim35
rdtrim trim35

#rddac trim35-tb22
#rdtrim trim35-tb22

# 19.7.2016 subvthr 4 in tb21
#rddac trim37
#rdtrim trim37

# 15.6.2016 with noise:
#rddac trim30
#rdtrim trim30
# thr 29.9 +- 2.5 from 20 to 46

# 19.6.2016
# thr 33.7 +- 1.6
# use tdmr: increase VthrComp until all ROCs have some activity (max 100 in 1M)
# modnoise 100 and modtrimbits 100 -> noise100 (ROC 8 is nice)
# thr 24.6 +- 2.8 from 15 to 41
# modtrimbits 100 -> noise103
# dac 13 90 (VIBias_bus)
# modtrimbits 100 -> noise104
# tune VthrComp -> noise106
# thr 26.1 +- 3.2 from 15 to 45

# 19.6.2016
# dac 13 90 (VIBias_bus)
# vtrim 0 (forgotten, not needed)
# modvthrcomp 30
# modtrim 28
#rddac trim29
#rdtrim trim29
# modtrimbits 100
# thr 30.5 +- 4.3 from 15 to 50
# modnoise 100
# modtrimbits 200

# 1.7.2016 TB24
# modvthrcomp 32
# modvtrim 32
# modtrimscale 32
# modtrimstep 32 (ROC 15 needs 7 iterations)
#rddac trim32
#rdtrim trim32
# thr 31.9+-1.8
# tdmr 0.001

# subvthr -1
# thr 30.7 +- 1.8
# hits 0.0052 / mod / trg  run 1776

# thr 30.1 +- 1.8
# hits 0.022 / mod / trg  run 1777

# thr 29.3 +- 1.83
# hits 0.093 / mod / trg  run 1778

# thr 28.5 +- 2.00
# hits 0.396 / mod / trg  run 1779

# thr 27.7 +- 2.08
# hits 1.509 / mod / trg  run 1780

# thr 27.0 +- 2.25
# hits 7.47 / mod / trg  run 1781
# non-uniform noise: mostly ROC 5 and 14

#rddac trim26n
#rdtrim trim26n
# thr 25.2 +- 2.45

# e-lab: fh1pitzlnb
# thr 35.54 +- 2.35: 9.3E-6 px/mod/trg
# subvthr -1
# thr 34.8 +- 2.2: 2.7E-5 px/mod/trg  run 204
# thr 33.9 +- 2.1: 1.1E-4 px/mod/trg  run 205
# thr 32.9 +- 2.0: 6.0E-4 px/mod/trg  run 206
# thr 32.0 +- 1.8: 3.4E-3 px/mod/trg  run 207
# thr 31.1 +- 1.8: 2.2E-2 px/mod/trg  run 208
# thr 30.4 +- 1.8: 1.1E-1 px/mod/trg  run 209
# thr 29.6 +- 1.8: 5.5E-1 px/mod/trg  run 210
# thr 28.7 +- 1.9: 2.6E+0 px/mod/trg  run 211
# thr 27.7 +- 2.0: 1.4E+1 px/mod/trg  run 212 errors
# thr 26.7 +- 2.2: 2.5E+1 px/mod/trg  run 213 errors
# thr 25.8 +- 2.4: 3.7E+1 px/mod/trg  run 214 errors

modmap 20
