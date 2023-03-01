obj-m = enable_pmu.o
all:
		make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
		gcc cycles_and_flush.c -o cycles_and_flush -O3
clean:
		make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
		rm cycles_and_flush
