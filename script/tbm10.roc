
# Nov 2016
# set TBM 10c

#pxar:
#base0   0x81 = 1000'0001
#base2   0xC0 = 1100'0000
#base4   0xF0 = 1111'0000
#base8   0x10 = 16
#basea   0xe4 = 1110'0100
#basec   0x00
#basee   0x08 = 0000'1000

tbmset $E4 b11110000  Init TBM
tbmset $F4 b11110000

tbmset $E0 b10000001  Disable Auto Reset, Disable PKAM Counter
tbmset $F0 b10000001

tbmset $E2 b11000000  Mode: Calibration
tbmset $F2 b11000000

tbmset $E8 9          Set PKAM Counter (x+1)*6.4us
tbmset $F8 9

tbmset $EA b11100100  1+1+3+3 = del_tin  del_TBM_Hdr/Trl  del_ROC_port1  del_ROC_port0
tbmset $FA b11100100  pXar

tbmset $EC 99         Auto reset rate (x+1)*256
tbmset $FC 99

tbmset $EE b00001000  00 + 3 + 3 = 160 + 400 MHz phase adjust  pXar

tbmset $FE $00        Temp measurement control
