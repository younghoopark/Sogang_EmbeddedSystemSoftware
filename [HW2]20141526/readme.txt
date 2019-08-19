<Sogang Embedded System Programming HW2>
<ParkYoungHoo / 20141526>

driver name : dev_driver
major number : 242

(Host PC)
cd app
make 	-> create app
adb push app /data/local/tmp
cd ..

cd module
make 	-> create dev_driver.ko
adb push dev_driver.ko /data/local/tmp

(minicom(board))
echo "7 6 1 7" > /proc/sys/kernel/printk
insmod dev_driver.ko
mknod /dev/dev_driver c 242 0
./app [time interval] [print count] [startoption]