#ifndef __GARBAGE__H
#define __GARBAGE__H

void garbage_init(void);
void garbage_final(void);
char *garbage_category(char *category);

// 拍照指令  照片路径的宏定义
#define WGET_CMD "wget http://192.168.18.11:8080/?action=snapshot -O /home/pi/wf/me/python-rubbish/Rubbish3.jpg"
#define GARBAGE_FILE "/home/pi/wf/me/python-rubbish/Rubbish3.jpg"
// 开启摄像头指令
#define VIDEO0 "/usr/local/bin/mjpg_streamer -i \"/usr/local/lib/mjpg-streamer/input_uvc.so -n -f 30 -r 1280x720\" -o \"/usr/local/lib/mjpg-streamer/output_http.so -p 8080 -w /usr/local/share/mjpg-streamer/www\" &"

#endif