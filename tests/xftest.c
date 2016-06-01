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

#define handle_error(msg)                                                      \
    do {								\
	perror(msg);							\
	exit(EXIT_FAILURE);						\
    } while (0)

#define MAJOR_NUM 101
#define IOCTL_SET_FILESIZE _IOR(MAJOR_NUM, 0, xlen_t *)
#define IOCTL_SET_NN _IOR(MAJOR_NUM, 1, xlen_t *)
#define IOCTL_SHOW_ANT _IO(MAJOR_NUM, 2)
#define IOCTL_PHYS_ADDR _IO(MAJOR_NUM, 3)
#define IOCTL_TRANS_PHYS_ADDR _IOR(MAJOR_NUM, 4, xlen_t*)
#define IOCTL_TEST _IO(MAJOR_NUM, 5)

int main (int argc, char **argv)
{
    int ret, fd;
    
    printf("[USER] Starting XFDdevice code example...\n");
    fd = open("/dev/xfd", O_RDWR); // Open the device with read/write access
    if (fd < 0) {
	perror("Failed to open the device...");
	return errno;
    }

    printf("[USER] PID = %d\n", getpid());

    ret = ioctl(fd, IOCTL_TEST);
    if (ret < 0) handle_error("IOCTL_TEST");
    
    close(fd);
    
    return 0;
}
