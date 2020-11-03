MODULE_NAME=net_log_module

obj-m += $(MODULE_NAME).o

KDIR ="/lib/modules/$(shell uname -r)/build"

.PHONY: clean build test all uninstall install test_full

all: build test_full

build:
	echo "make -C ${KDIR} M=${PWD} modules"
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean


install:
	dmesg -C
	insmod $(MODULE_NAME).ko

uninstall:
	rmmod $(MODULE_NAME).ko

test:
	curl https://google.com
	curl https://yandex.com	
	cat /proc/group-ib

test_full: install test uninstall
	dmesg