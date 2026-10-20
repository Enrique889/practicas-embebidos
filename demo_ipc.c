#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main() {
    int fd[2];
    char buf[50];
    char *msg = "hola por el pipe";

    // Crea la tubería
    if (pipe(fd) == -1) return 1;

    pid_t pid = fork(); // Crea el proceso hijo

    if (pid == 0) {
        // Hijo: lee del pipe
        close(fd[1]); 
        read(fd[0], buf, strlen(msg));
        buf[strlen(msg)] = '\0';
        printf("Hijo leyo: %s\n", buf);
        close(fd[0]);
    } else {
        // Padre: escribe en el pipe
        close(fd[0]); 
        write(fd[1], msg, strlen(msg));
        close(fd[1]);
        wait(NULL); 
    }
    return 0;
}
