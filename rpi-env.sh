export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
file=$(which ${CROSS_COMPILE}gcc)
if [ ! -x "$file" ];
then 
	echo "ERROR: Cross compiler not found!"; 
fi
