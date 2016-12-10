
# prepare production modules for test beam
# cd mods
# mkdir M4499
# cd M4499
# ssh cmspix@cmspixel  Pxl-team
# locate M4499
# scp /home/cmspix/ModuleTest/Testing_2016-04-26/M4499_FullQualification_2016-04-26_09h08m_1461654488/005_Fulltest_p17/trimParameters35_C*.dat pitzl@fh1pitzlnb:~/mods/M4499/.
# scp /home/cmspix/ModuleTest/Testing_2016-04-26/M4499_FullQualification_2016-04-26_09h08m_1461654488/005_Fulltest_p17/dacParameters35_C*.dat pitzl@fh1pitzlnb:~/mods/M4499/.
# exit
# ../merge -m 4499
# mv dacParameters_D4499_trim35.dat ~/psi/dtb/tst406
# mv trimParameters_D4499_trim35.dat ~/psi/dtb/tst406
# prepare psi/dtb/tst400/script/s4499.roc
# use in psi46test

s4499  # with module 4499; rddac trim35; rdtrim trim35

# check noise, readout errors:
tdmr

# adjust noisy pixels:
modtrimbits  // 350s

wtrim trim35

# response map:
modmap 20
upd 12

# PH tuning:
ctl 4
dac 17 166
dac 20 111
modtune 22 33 // 142s
show1 17
show1 20

wdac trim35

# gaincal:
modcal
