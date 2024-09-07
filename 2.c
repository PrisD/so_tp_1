#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int retorno_hijo;

void h_retorno_hijo(int sig);

int main(int argc, char *argv[]) {

  int n = atoi(argv[1]);
  pid_t pids[n];
  int suma_retorno = 0;

  // crear n hijos
  for (int i = 0; i < n; i++) {
    pid_t pid = fork();
    if (pid == -1) {
      perror("Error al crear el proceso");
      exit(1);
    } else if (pid == 0) {
      // hijo
      printf("hijo: %d \n", getpid());
      retorno_hijo = i + 1;
      signal(SIGUSR1, h_retorno_hijo);
      signal(SIGINT, h_retorno_hijo);

      while (1) {
        pause();
      }
    } else {
      pids[i] = pid; // lo guardo así el padre sabe que esperar
    }
  }

  // padre
  for (int i = 0; i < n; i++) {
    int status;
    pid_t hijo_terminado = waitpid(pids[i], &status, 0);
    if (hijo_terminado > 0 && WIFEXITED(status)) {
      int retorno = WEXITSTATUS(status);
      suma_retorno += retorno;
    }
  }

  printf("La suma de los retornos de los procesos hijos es: %d\n",
         suma_retorno);
  return 0;
}

void h_retorno_hijo(int sig) { exit(retorno_hijo); }