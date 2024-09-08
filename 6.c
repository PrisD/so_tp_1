#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  int pipe[2];

  pid_t pid = fork();
  if (pid == 0) {    
    //hijo
    close(pipe[0]);

    if (dup2(pipe[1], STDOUT_FILENO) == -1) {
      perror("Error al redirigir la salida estándar");
      exit(0);
    }
    close(pipe[1]);
    execvp(argv[1], &argv[1]);
    perror("Error al ejecutar el comando");
    exit(0);
  } else { 
    // padre
    close(pipe[1]);
    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
      buffer[bytes_read] =
          '\0'; 
      printf("%s", buffer);
    }
    close(pipe[0]);

    int status;
    waitpid(pid, &status, 0);
  }

  return 0;
}
