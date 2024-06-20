一、操作部分
// 编译代码
gcc main.c op.c voice.c socket.c smoke.c ds18b20.c sr04.c receive.c tiktok.c face.c oled.c myoled.c living.c bed.c fan.c servo.c sound.c check_face.c ini.c hc01.c -o main -I /usr/include/python3.9/ -l python3.9 -lpthread -lwiringPi


// 添加阿里云的环境变量
export ALIBABA_CLOUD_ACCESS_KEY_ID="***"
export ALIBABA_CLOUD_ACCESS_KEY_SECRET="***"

// 打开摄像头
/usr/local/bin/mjpg_streamer -i "/usr/local/lib/mjpg-streamer/input_uvc.so -n -f 30 -r 1280x720" -o "/usr/local/lib/mjpg-streamer/output_http.so -p 8080 -w /usr/local/share/mjpg-streamer/www" &

// make一下下面驱动程序，且都加入驱动
// 客厅灯
sudo insmod gpio4_driver.ko
sudo chmod 777 /dev/gpio4

// 舵机
sudo insmod servo_drv.ko
sudo chmod 777 /dev/servo

// 烟雾报警
sudo insmod smoke_drv.ko
sudo chmod 777 /dev/smoke

// 温度检测
sudo insmod ds18b20_drv.ko
sudo chmod 777 /dev/ds18b20

// 超声波测距
sudo insmod sr04_drv.ko
sudo chmod 777 /dev/sr04

// 卧室灯
sudo insmod gpio13_drv.ko
sudo chmod 777 /dev/gpio13

// 风扇
sudo insmod gpio5_drv.ko
sudo chmod 777 /dev/gpio5

// 网页查看摄像头监控实时视频
http://192.168.18.4:8080/?action=stream

// 拍照截图保存到-O指定的路径
wget http://192.168.18.4:8080/?action=snapshot -O /home/pi/myproject/smart-house/zhang4.jpg

二、讲解部分
main2.c是定义的全局变量，简单易懂（我在main里面用的是二级指针传参带回链表地址）

