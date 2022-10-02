ifeq ($(KERNELRELEASE),)

ifeq ($(KDIR),)
KDIR:=/lib/modules/$(shell uname -r)/build
endif

default : leds-gpio-multi.ko pine64-leds-multi.dtbo

leds-gpio-multi.ko : leds-gpio-multi.c
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules

%.dtbo : %.dtbs
	gcc -x c -E -I $(KDIR)/include $< | dtc -I dts -O dtb -@ -o $@ - || (rm -f $@ ; false)

.PHONY: clean
clean :
	rm -f *~ *.o *.ko *.mod.[co] *.mod .*.cmd 
	rm -rf .tmp_versions Module.symvers Modules.symvers modules.order *.dtbo
else
###
# objects for our simple module
###

obj-m:= leds-gpio-multi.o

endif
