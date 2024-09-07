#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_ARGS 100

int foreground_processes = 0;

void h_aviso_termina(int sig);
void h_salir(int sig);
void leer_comando(char *input, char **args, int *background);

int main() {
  char line[MAX_LINE];
  char *args[MAX_ARGS];
  int background;

  signal(SIGINT, h_salir);
  signal(SIGCHLD, h_aviso_termina);

  while (1) {
    printf("shell> ");
    fflush(stdout);

    if (!fgets(line, MAX_LINE, stdin)) {
      break;
    }

    leer_comando(line, args, &background);

    if (args[0] == NULL) {
      continue;
    }

    if (strcmp(args[0], "salir") == 0) {
      printf("Saliendo...\n");
      break;
    }

    pid_t pid = fork();
    if (pid < 0) {
      perror("Error al crear proceso hijo");
    } else if (pid == 0) {
      // Lógica hijo
      execvp(args[0], args);
      perror("Error al ejecutar comando");
      exit(0);
    } else {
      // Lógica padre
      if (!background) {
        // Proceso foreground espera a que termine
        foreground_processes++;
        waitpid(pid, NULL, 0);
        foreground_processes--;
      } else {
        // Informar el pid q se ejecuta en background
        printf("Proceso %d ejecutándose en background\n", pid);
      }
    }
  }

  return 0;
}

void h_aviso_termina(int sig) {
  int status;
  pid_t pid;
  while ((pid = waitpid(0, NULL, 0)) > 0) {
    printf("Terminé! soy %d \n", pid);
  }
}

void h_salir(int sig) {
  printf("\nRecibido CTRL+C. Esperando la finalización de procesos "
         "foreground...\n");
  while (foreground_processes > 0) {
    pause(); 
  }
  printf("Saliendo...\n");
  exit(0);
}

void leer_comando(char *input, char **args, int *background) {
  *background = 0;
  char *token = strtok(input, " ");
  int i = 0;

  while (token != NULL) {
    if (strcmp(token, "&") == 0) {
      *background = 1;
    } else {
      args[i++] = token;
    }
    token = strtok(NULL, " \t\n");
  }
  args[i] = NULL;
}