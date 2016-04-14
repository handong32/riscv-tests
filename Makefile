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
XD = /opt/xfiles-dana
XDR = $(shell pwd)
KERNELDIR := ~/github/riscv-linux/linux

XFILESINCS=-I $(XD) -I $(XD)/usr/include
XFILESLIBS=-L /opt/xfiles-dana/build/linux -lxfiles -L /opt/xfiles-dana/build/fann-rv-linux -lfann -lfixedfann -lfloatfann -ldoublefann -lm
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

test: tests/test.c
	${CC} ${CFLAGS} -static $< -o $@ ${LIBS}
	cp test initramfs/home/

ebbioctl: tests/ebbioctl.c
	${CC} ${CFLAGS} -static $< -o $@ ${LIBS}
	cp ebbioctl initramfs/home/

#hello_mod: tests/hello_mod.c
#	$(MAKE) -C $(KERNELDIR) M=$(XDR) ARCH=$(ARCH) CROSS_COMPILE=$(COMPILER) modules

bbl:
	cd /home/handong/github/riscv-linux/linux && make -j ARCH=riscv vmlinux
	cd riscv-pk/build && make -j

#mnt:
#	mkdir mnt

#domount: 
#	sudo mount -o loop ${DISK} mnt

#doumount:
#	sudo umount mnt

#install: danabench xdhello domount testfloat
#	sudo cp danabench mnt/home
#	sudo cp xdhello mnt/home
#	sudo cp mushroom_x mnt/home
#	sudo cp testfloat mnt/home
#	sudo cp xorSigmoidSymmetric-fixed.16bin /mnt/home
#	sudo umount mnt

#install_mod: domount
#	sudo cp tests/hello_mod.ko mnt/home
#	make doumount

#install_xdhello: domount xdhello
#	sudo cp xdhello mnt/home
#	make doumount

#install_pthreadHello: domount pthreadHello
#	sudo cp pthreadHello mnt/home
#	make doumount

#install_forkTest: domount forkTest
#	sudo cp forkTest mnt/home
#	make doumount

#install_forkASID: domount forkASID
#	sudo cp forkASID mnt/home
#	make doumount

#runInstall: install
#	ssh -t ${FPGA}  ". /home/root/.profile; ${FESRV} +disk=${MYDIR}/${DISK} ${MYDIR}/bbl ${MYDIR}/vmlinux"

#runOrig:
#	ssh -t ${FPGA}  ". /home/root/.profile; ${FESRV} +disk=${MYDIR}/${DISK} ${MYDIR}/bbl ${MYDIR}/vmlinux.orig"

#runWorking:
#	ssh -t ${FPGA}  ". /home/root/.profile; ${FESRV} +disk=${MYDIR}/${DISK} ${MYDIR}/bbl ${MYDIR}/vmlinux.working"

run:
	ssh -t ${FPGA}  ". /home/root/.profile; ${FESRV} ${MYDIR}/riscv-pk/build/bbl"

clean:
	${RM} ${wildcard danabench xdhello pthreadHello forkTest forkASID}
#$(MAKE) -C $(KERNELDIR) M=$(XDR) ARCH=$(ARCH) clean
