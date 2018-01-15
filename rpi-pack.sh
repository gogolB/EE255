export INSTALL_MOD_PATH=$PWD/rfs/mod
export ARCH=arm
# pack modules
rm -rf $INSTALL_MOD_PATH 2> /dev/null
mkdir -p $INSTALL_MOD_PATH
make modules_install
cd $INSTALL_MOD_PATH
# remove symlinks that point to files we do not need in root file system
find . -name source | xargs rm
find . -name build | xargs rm
# compress modules
tar --owner=root --group=root -cvzf ../../modules.tgz .
# copy kernel and dtb files
cd ../../
export INSTALL_KERN_PATH=$PWD/rfs/kern
rm -rf $INSTALL_KERN_PATH 2> /dev/null
mkdir -p $INSTALL_KERN_PATH/boot/overlays
cd $INSTALL_KERN_PATH
cp ../../arch/arm/boot/zImage boot/kernel7.img
cp ../../arch/arm/boot/dts/*.dtb boot/
cp ../../arch/arm/boot/dts/overlays/*.dtb* boot/overlays/
tar --owner=root --group=root -cvzf ../../boot.tgz .
cd ../../
