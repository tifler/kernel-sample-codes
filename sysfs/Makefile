TARGET	:=sysfs-sample

obj-m += $(TARGET).o

$(TARGET)-objs	:= main.o

MODULE_DIR	:= "$(PWD)"

all:
	CROSS_COMPILE= ARCH=x86 make -C /lib/modules/$(shell uname -r)/build M=$(MODULE_DIR) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(MODULE_DIR) clean
