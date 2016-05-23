#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdint.h>

#include "usr/include/fixedfann.h"
#include "src/main/c/xfiles-user.h"

#define handle_error(msg)                                                      \
    do {								\
	perror(msg);							\
	exit(EXIT_FAILURE);						\
    } while (0)

//typedef uint64_t xlen_t;

#define MAJOR_NUM 101
#define IOCTL_SET_FILESIZE _IOR(MAJOR_NUM, 0, xlen_t *)
#define IOCTL_SET_NN _IOR(MAJOR_NUM, 1, xlen_t *)
#define IOCTL_SHOW_ANT _IO(MAJOR_NUM, 2)
#define IOCTL_PHYS_ADDR _IO(MAJOR_NUM, 3)
#define IOCTL_TRANS_PHYS_ADDR _IOR(MAJOR_NUM, 4, xlen_t*)

void writedev(int fd, char *s, int len) {
  int ret = write(fd, s, len); // Send the string to the LKM
  if (ret < 0) {
    perror("Failed to write the message to the device.");
    exit(0);
  }
}

void
dumpNNBytes(xlen_t *addr, xlen_t size)
{
    printf("[INFO] dumpNNBytes in user: \n");
    printf("\nNN: 0x%lx size: %ld\n", addr, size);
    for (xlen_t i=0; i<size; i++) {
	if (i%8 == 0) printf("\n %03ld: ", i);
	printf("%016lx ", addr[i]);
    }
    printf("\n\n");
}


int main(int argc, char **argv) {
    int ret, fd, nfd;
    xlen_t temp, fbytes, file_size;
    struct stat stat;
    
    printf("Starting device test code example...\n");
    fd = open("/dev/nndev", O_RDWR); // Open the device with read/write access
    if (fd < 0) {
	perror("Failed to open the device...");
	return errno;
    }

    printf("[USER] PID = %d\n", getpid());

    xlen_t* temp2 = (xlen_t*)malloc(5*sizeof(xlen_t));
    for(xlen_t i = 0; i < 5; i++)
    {
	temp2[i] = 99-i;
	//debug_write_mem((uint32_t)i, temp2+i);
	printf("[USER] temp2[%ld] = 0x%lx\n", i, temp2[i]);
    }
  
    printf("[USER] virt temp2: %p \n", temp2);
  
    xlen_t xfphys = debug_virt_to_phys(temp2);
    printf("[USER] debug_virt_to_phys(temp2) = 0x%lx\n", xfphys);

    xlen_t xfphys2 = debug_virt_to_phys(temp2+1);
    printf("[USER] debug_virt_to_phys(temp2+1) = 0x%lx\n", xfphys2);

    xlen_t xfphys_val_1 = debug_read_utl((void*)xfphys);
    printf("[USER] debug_read_utl((void*)xfphys) = 0x%lx\n", xfphys_val_1);

    xlen_t xfphys_val_2 = debug_read_utl((void*)xfphys2);
    printf("[USER] debug_read_utl((void*)xfphys2) = 0x%lx\n\n", xfphys_val_2);
  
    ret = ioctl(fd, IOCTL_TRANS_PHYS_ADDR, &temp2);
    if (ret < 0) handle_error("IOCTL_TRANS_PHYS_ADDR");
  
    nfd = open("xorSigmoidSymmetric-fixed.16bin", O_RDONLY);
    assert(nfd!=-1);
    fstat(nfd, &stat);
    fbytes = (xlen_t)stat.st_size;
    file_size = stat.st_size / sizeof(xlen_t);
    file_size += (stat.st_size % sizeof(xlen_t)) ? 1 : 0;

    xlen_t *addr;
    addr = (xlen_t *)mmap(NULL, fbytes, PROT_READ, MAP_SHARED | MAP_LOCKED, nfd, 0);
    if (addr == MAP_FAILED) handle_error("mmap");

    dumpNNBytes(addr, file_size);
    
    ret = ioctl(fd, IOCTL_TRANS_PHYS_ADDR, &addr);
    if (ret < 0) handle_error("IOCTL_TRANS_PHYS_ADDR");

    close(nfd);
    close(fd);
    free(temp2);
  
    return 0;
}
