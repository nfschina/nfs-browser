INC := \
    -I../ \
    -I/home/lsun/appexcvs/src/engine/include \
    -I/home/lsun/appexcvs/src/platform/linux/include \

CFLAGS := -g $(INC) -L../

LIB := -luci

all:
	$(CROSS_COMPILE)gcc $(CFLAGS) apxclsfcfg.c -o apxclsfcfg $(LIB)

clean:
	rm -f *.o apxclsfcfg
