#include <stdio.h>
#include <string.h>

// Required by for routine
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <stdlib.h>   // Declaration for exit()

#include <xfiles.h>
#include <fixedfann.h>
#include <fann_train.h>

enum XD_LearningTypes{ FEED_FORWARD_TRANS = 0 };

void pingXD()
{
    nnid_type mynnid2 = 0;
    int ltype2 = FEED_FORWARD_TRANS;
    element_type nto2 = 0;
    
    tid_type tt2=0;
    
    xfiles_dana_id(!0);
    
    tt2=new_write_request(mynnid2, ltype2, nto2);
    
    printf("tt2=0x%x\n", tt2);

}
int main()
{
    pid_t pID = fork();
    if (pID == 0)                // child
    {
	// Code only executed by child process
	printf("\n\nChild process: %d\n", getpid());
	pingXD();
    }
    else if (pID < 0)            // failed to fork
    {
	printf("failed to fork\n");
        exit(1);
    }
    else                                   // parent
    {	
	// Code only executed by parent process
	printf("\n\nParent process: %d\n", getpid());
	pingXD();
    }

    // Code executed by both parent and child.
    syscall(245);
    return 0;
}
