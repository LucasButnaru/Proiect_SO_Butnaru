#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define BUFFERSIZE 1024

int main()
{
    int hub_pid;

    if((hub_pid = fork())<0)
    {
        perror("Fork Hub Failed\n");
        exit(-1);
    }
    else if(hub_pid == 0)
    {
        int pfd[2];
	    int pid;

	    if(pipe(pfd)<0)
	    {
		    printf("Eroare la crearea pipe-ului\n");
		    exit(1);
	    }

	    if((pid=fork())<0)
	    {
		    printf("Eroare la fork\n");
		    exit(1);
	    }

	    if(pid==0)
        {
		    close(pfd[0]); 
		    dup2(pfd[1],STDOUT_FILENO); 
		    execlp("./monitor","monitor_reports",NULL); 
		    perror("EXECLP Failed\n");
            exit(0);
		
	    }

        close(pfd[1]);
    
        char buffer[BUFFERSIZE];
        int n;
        while(n=read(pfd[0],buffer, sizeof(buffer)>0)){
            buffer[n]='\0';
            printf("[MONITOR] %s", buffer);
        }
    
        close(pfd[0]);

    }

        return 0;
}



