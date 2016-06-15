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

#define handle_error(msg)    \
    do {	             \
	perror(msg);         \
	exit(EXIT_FAILURE);  \
    } while (0)


struct nndesc {
    int fd;
    void *addr;
    struct stat stat;
};

void
nnDump(struct nndesc *this)
{
    printf("0x%p: fd=%d addr=0x%p stat.st_size=%ld\n", 
	   this, this->fd, this->addr, this->stat.st_size);
}     

int main (int argc, char **argv)
{
    int ret, xfd;
    struct nndesc nn;
    char *buffer;

    printf("[USER] Starting XFDdevice code example...\n");
    xfd = open(argv[1], O_RDWR); // Open the device with read/write access
    if (xfd < 0) {
	perror("Failed to open the device...");
	return errno;
    }

    printf("[USER] PID = %d\n", getpid());

    nn.fd = open(argv[2], O_RDWR);
    assert(nn.fd != -1);

    assert(fstat(nn.fd, &nn.stat)==0);

    nn.addr = mmap(NULL, nn.stat.st_size, PROT_WRITE, MAP_PRIVATE, nn.fd, 0);

    nnDump(&nn);

    if (write(xfd, nn.addr + 1, nn.stat.st_size) != -1) assert(0);
    perror("ERROR: write failed: ");

    assert(posix_memalign((void **)&buffer, 4096, 4096)==0);
    assert(write(xfd, buffer, 4096) == 4096);

    assert(write(xfd, nn.addr, nn.stat.st_size) == nn.stat.st_size);
   
    close(xfd);
    
    return 0;
}
