cd ../../build_arm
sudo stty -F /dev/ttyAMA0 -echo
sudo stty -F /dev/ttyAMA0 ospeed 9600 2> /dev/null
sudo stty -F /dev/ttyAMA0 ispeed 9600
sudo stty -F /dev/ttyAMA0 ospeed 9600
sudo ./main
