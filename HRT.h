#include <linux/cdev.h>
#include <plat/dmtimer.h>

#define DEVICE_NAME "HRT"
#define CMD_START_TIMER 0xffc1
#define CMD_STOP_TIMER  0xffc2
#define CMD_SET_TIMER_COUNTER_VALUE 0xffc3

// The following structure should not be used in user space program. It is for the driver implementation
struct HRT_dev {
	struct cdev cdev;
	char name[20];
	struct omap_dm_timer *timer;		
	size_t size;
} *hrt_devp;

