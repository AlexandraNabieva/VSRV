#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(){
    int fd = open("result_file" , O_CREAT | O_WRONLY);
    dup2(fd , 1);
    close(fd);
    int newpid = fork();
    if (newpid == 0){
        char *args[] = {"./child_process" , NULL};
        execvp(args[0] , args);
    }
    else if(newpid > 0){
        char *args[] = {"./parent_process" , NULL};
        execvp(args[0] , args);
    }
    else {
        perror("fork");
    }
    return 0;
}