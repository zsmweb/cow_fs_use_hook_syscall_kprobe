
main_cowfs := cowfs_module
obj-m += $(main_cowfs).o
$(main_cowfs)-objs += cowfs.o kprobe_hook.o handlers.o utils.o

all: clean
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean