#
# D. Pitzl. 2011
# init USB for psi46 test board
#
sudo mount --bind /dev/bus /proc/bus
sudo ln -s /sys/kernel/debug/usb/devices /proc/bus/usb/devices

sudo chmod 666 /proc/bus/usb/00*/*

# remove modules from kernel:
#old trivial:
# sudo /sbin/rmmod ftdi_sio usbserial
# intelligent:
# modprobe -c  lists kernel config
sudo modprobe -r ftdi_sio
#sudo modprobe -r usbserial
