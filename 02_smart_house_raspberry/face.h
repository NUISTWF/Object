#ifndef __FACE__H
#define __FACE__H

void face_init(void);
void face_final(void);
double face_category();

// 拍照指令  照片路径的宏定义
#define WGET_CMD "wget http://192.168.18.4:8080/?action=snapshot -O /home/pi/myproject/smart-house/zhang4.jpg"
#define FACE_FILE "/home/pi/myproject/smart-house/zhang4.jpg"
// 开启摄像头指令
#define VIDEO0 "/usr/local/bin/mjpg_streamer -i \"/usr/local/lib/mjpg-streamer/input_uvc.so -n -f 30 -r 1280x720\" -o \"/usr/local/lib/mjpg-streamer/output_http.so -p 8080 -w /usr/local/share/mjpg-streamer/www\" &"

#endif