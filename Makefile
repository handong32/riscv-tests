ARCH=riscv
CC=riscv64-unknown-linux-gnu-gcc
COMPILER=riscv64-unknown-linux-gnu-
obj-m := hello_mod.o
LDFLAGS=-static -march=RV64IMAFDXcustom
LIBS=-lpthread
CFLAGS=${LDFLAGS}
DISK=root.bin
MYDIR = $(MYDIRF)
FPGA=root@fpga0
FESRV=./fesvr-zynq
XD = ~/github/xfiles-dana
#XD = /opt/xfiles-dana
XDR = $(shell pwd)
KERNELDIR := ~/github/riscv-linux/linux

XFILESINCS=-I $(XD) -I $(XD)/usr/include -I $(XDR)/xfd/src/include
XFILESLIBS=-L $(XD)/build/linux -lxfiles-user -L $(XD)/build/fann-rv-linux -lfixedfann -L $(XDR)/xfd/lib -lxfd -lm
FANNLIBS=-L $(XDR)/../xfiles-dana/build/fann/src

CFLAGS+=${XFILESINCS} 
LIBS+=${XFILESLIBS}

#.PHONY: domount doumount mnt install

#all: danabench xdhello testfloat

xdhello: tests/xdhello.c
	${CC} ${CFLAGS} -static $< -o $@ ${LIBS}

mushroom_linux: mushroom.c
	gcc ${FANNLIBS} -I$(XDR)/../xfiles-dana/submodules/fann/src/include -static $< -o $@ -lfann -lfixedfann -lfloatfann -ldoublefann -lm

mushroom_x: mushroom.c
	${CC} -static -march=RV64IMAFDXcustom -static -I$(XD)/src/main/c -I$(XDR)/../rocket-chip/xfiles-dana/submodules/fann/src/include $< -o $@ ${LIBS}

pthreadHello: tests/pthreadHello.c
	${CC} ${CFLAGS} -static $< -o $@ ${LIBS}
	cp pthreadHello initramfs/home/

forkTest: tests/forkTest.c
	${CC} ${CFLAGS} -static $< -o $@ ${LIBS}
	cp forkTest initramfs/home/

forkASID: tests/forkASID.c
	${CC} ${CFLAGS} -static $< -o $@ ${LIBS}
	cp forkASID initramfs/home/

pkxf.rv: tests/fann-xfiles.c
	riscv64-unknown-elf-gcc -DPROXYK -static -march=RV64IMAFDXcustom ${XFILESINCS} -I/tests -I{XD}/build/nets tests/fann-xfiles.c -o $@ -L /opt/xfiles-dana/build/newlib -lxfiles-user -L /opt/xfiles-dana/build/fann-rv-newlib -lfixedfann -lm

lxfx.rv: tests/fann-xfiles.c
	${CC} -static -march=RV64IMAFDXcustom ${XFILESINCS} -I/tests -I{XD}/build/nets tests/fann-xfiles.c -o $@ -L /opt/xfiles-dana/build/linux -lxfiles-user -lxfiles-supervisor -L /opt/xfiles-dana/build/fann-rv-linux -lfixedfann -lm
	cp $@ initramfs/home/

xfmem.rv: tests/xfmem.c
	${CC} ${CFLAGS} -static $< -o $@ ${LIBS}
	cp xfmem.rv initramfs/home/

xfapp.rv: tests/xfapp.c
	${CC} ${CFLAGS} -static $< -o $@ ${LIBS}
	cp xfapp.rv initramfs/home/

xftest.rv: tests/xftest.c
	${CC} ${CFLAGS} -static $< -o $@ ${LIBS}
	cp xftest.rv initramfs/home/

bbl:
	cd /home/handong/github/riscv-linux/linux && make -j ARCH=riscv vmlinux
	cd riscv-pk/build && make -j

run: bbl
	ssh -t ${FPGA}  ". /home/root/.profile; ${FESRV} ${MYDIR}/riscv-pk/build/bbl"

runTest: xfmem.rv xfapp.rv xftest.rv bbl
	ssh -i /opt/etc/fpga-ssh -t ${FPGA}  ". /home/root/.profile; ${FESRV} ${MYDIR}/riscv-pk/build/bbl"

clean:
	${RM} ${wildcard *.rv}
