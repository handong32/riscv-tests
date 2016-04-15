#include <sys/mman.h>
#include <sys/stat.h>
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>

#include "src/main/c/xfiles-user.h"

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM
 
void writedev(int fd, char* s, int len)
{
    int ret = write(fd, s, len); // Send the string to the LKM
    if (ret < 0){
	perror("Failed to write the message to the device.");
	exit(0);
    }
}

int main()
{
    int ret, fd;
    char s[BUFFER_LENGTH];
 
    //set xs bits
    xfiles_dana_id(1);

    printf("Starting device test code example...\n");
    fd = open("/dev/ebbchar", O_RDWR);             // Open the device with read/write access
    if (fd < 0){
	perror("Failed to open the device...");
	return errno;
    }
    
    strcpy(s, "createant");
    writedev(fd, s, strlen(s));
    
    strcpy(s, "showant");
    writedev(fd, s, strlen(s));

    struct stat stat;
    int nfd, fbytes, file_size;
    nfd = open("xorSigmoidSymmetric-fixed.train", O_RDONLY);
    fstat(nfd, &stat);
        
    fbytes = (int) stat.st_size;
    file_size = stat.st_size / sizeof(xlen_t);
    file_size += (stat.st_size % sizeof(xlen_t)) ? 1 : 0;
    printf("bytes %d, file_size %d\n", (int)stat.st_size, file_size);
    
    char *addr;
    addr = (char *) mmap(NULL, fbytes, PROT_READ, MAP_SHARED | MAP_LOCKED, nfd, 0);
    if (addr == MAP_FAILED)
	handle_error("mmap");
  
    
    close(nfd);
    printf("End of the program\n");
    return 0;
}

/*

  ssize_t s2;

    s2 = write(STDOUT_FILENO, addr, fbytes);
    if (s2 != fbytes) {
	if (s2 == -1)
	    handle_error("write");
	
	fprintf(stderr, "partial write");
	exit(EXIT_FAILURE);
    }


  xlen_t *buf;
  buf = (xlen_t *) malloc(file_size * sizeof(xlen_t));
    
  // Open the file and find out how big it is so that we can allocate
  // the correct amount of space
  if (!(fp = fopen("xorSigmoidSymmetric-fixed.16bin", "rb"))) {
  return -1;
  }
  // Write the configuration
  fread(buf, sizeof(xlen_t), file_size, fp);
  fclose(fp);

  int* faddr;
  faddr = &fbytes;
  writedev(fd, (char*)faddr, sizeof(int*));


  printf("Type in a short string to send to the kernel module:\n");
  scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)
  printf("Writing message to the device [%s].\n", stringToSend);
  ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
  if (ret < 0){
  perror("Failed to write the message to the device.");
  return errno;
  }
 
  printf("Press ENTER to read back from the device...\n");
  getchar();
 
  printf("Reading from the device...\n");
  ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
  if (ret < 0){
  perror("Failed to read the message from the device.");
  return errno;
  }
  printf("The received message is: [%s]\n", receive);*/
