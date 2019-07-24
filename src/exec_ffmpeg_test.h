#ifndef EXEC_FFMPEG_TEST_H_H_
#define EXEC_FFMPEG_TEST_H_H_

#include<unistd.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<sys/wait.h>

int spawn(char *program,char **argv)
{
    int childPid = fork();
    if(childPid == -1) 
    {   
        fprintf(stderr,"error in fork:%s\n",strerror(errno));
        return -1; 
    }   

    if(childPid != 0) //当在父进程中是，fork返回的是子进程id, 当是子进程时，返回0
    {   
        return childPid;
    }   
    else
    {   
        execvp(program,argv);
        fprintf(stderr,"error in exec function:%s\n",strerror(errno));
        abort();
    }   
}

int exec_ffmpeg_test()
{
    printf("start progrom\n");
    int childStatus= 0;
    //char* cmd = "-ss 5 -t 10 -accurate_seek -i ./半壶纱.mp4 -codec copy -avoid_negative_ts 1 ./cut.mp4";
    char *argList[]={"ffmpeg","-ss 5 -t 10 -accurate_seek -i ./半壶纱.mp4 -codec copy -avoid_negative_ts 1 ./cut.mp4",NULL};
    
    spawn("/home/caiyu/ffmpeg_install/bin/ffmpeg",argList);
    wait(&childStatus);
    if(WIFEXITED (childStatus))
    {
        printf ("the child process exited normally, with exit code %d\n",WEXITSTATUS (childStatus));
    }
    else
    {   
        printf ("the child process exited abnormally\n");
    }
    printf("done in main program\n");
    return 0;
}

#endif