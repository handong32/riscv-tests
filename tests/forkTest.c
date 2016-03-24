#include <stdio.h>
#include <string.h>

// Required by for routine
#include <sys/types.h>
#include <unistd.h>

#include <stdlib.h>   // Declaration for exit()

int globalVariable = 2;
int *array;

int main()
{
    int iStackVariable = 20;
    
    pid_t pID = fork();
    if (pID == 0)                // child
    {
	array = (int *) malloc (10 * sizeof(int));
	int i;
	for(i=0;i<10;i++)
	{
	    array[i] = i+1;
	}
	// Code only executed by child process
	printf("Child process\n");
	globalVariable++;
	iStackVariable++;
    }
    else if (pID < 0)            // failed to fork
    {
	printf("failed to fork\n");
        exit(1);
    }
    else                                   // parent
    {
	// Code only executed by parent process
	printf("Parent process\n");
    }

    // Code executed by both parent and child.
    printf("global: %d stack: %d\n", globalVariable, iStackVariable);
    printf("0x%x\n", array);
    printf("%d\n", *array);
    
    return 0;
}
