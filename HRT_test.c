#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#define CMD_START_TIMER 0xffc1
#define CMD_STOP_TIMER  0xffc2
#define CMD_SET_TIMER_COUNTER_VALUE 0xffc3


int main()
{
  int hrt_timer; 
  unsigned int read_buffer;
  hrt_timer = open("/dev/HRT", O_RDWR);
  
  ioctl(hrt_timer, CMD_START_TIMER);
  sleep(1);
  read(hrt_timer,&read_buffer,sizeof(int));
  printf("Timer value = %d\n",read_buffer);
  sleep(1);
  read(hrt_timer,&read_buffer,sizeof(int));
  printf("Timer value = %d\n",read_buffer);
  sleep(1);
  read(hrt_timer,&read_buffer,sizeof(int));
  printf("Timer value = %d\n",read_buffer);
  sleep(1);
  read(hrt_timer,&read_buffer,sizeof(int));
  printf("Timer value = %d\n",read_buffer);
  ioctl(hrt_timer,CMD_SET_TIMER_COUNTER_VALUE,50);
  read(hrt_timer,&read_buffer,sizeof(int));
  printf("Timer value = %d\n",read_buffer);
  ioctl(hrt_timer, CMD_STOP_TIMER);
  close(hrt_timer);
  
}