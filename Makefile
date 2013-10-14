ifneq ($(KERNELRELEASE),) 
   obj-m := Squeue.o
   obj-m += HRT.o
else 

KERNELDIR ?= ~/Kernel_Source_Trees/Angstrom_Linux/kernel/kernel/ 

PWD := $(shell pwd)

default: 
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-angstrom-linux-gnueabi- -C $(KERNELDIR) M=$(PWD) modules  
	arm-angstrom-linux-gnueabi-gcc main_2.c -lpthread -o squeue_test
endif 
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm squeue_test
