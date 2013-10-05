ifneq ($(KERNELRELEASE),) 
   obj-m := HRT.o
else 

KERNELDIR ?= ~/Kernel_Source_Trees/Angstrom_Linux/kernel/kernel/ 

PWD := $(shell pwd)

default: 
	$(MAKE) ARCH=arm CROSS_COMPILE=arm-angstrom-linux-gnueabi- -C $(KERNELDIR) M=$(PWD) modules  
endif 
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
