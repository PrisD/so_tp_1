#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_ARGS 100

volatile sig_atomic_t foreground_processes = 0;

void h_aviso_termina(int sig);
void h_salir(int sig);

// Función para dividir el input en comandos y argumentos
void parse_command(char *input, char **args, int *background);
int main() {
  char line[MAX_LINE];
  char *args[MAX_ARGS];
  int background;

  // Configurar los manejadores de señales
  struct sigaction sa_chld, sa_int;

  sa_chld.sa_handler = h_aviso_termina;
  sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  sigemptyset(&sa_chld.sa_mask);
  sigaction(SIGCHLD, &sa_chld, NULL);

  sa_int.sa_handler = h_salir;
  sigemptyset(&sa_int.sa_mask);
  sa_int.sa_flags = 0;
  sigaction(SIGINT, &sa_int, NULL);

  while (1) {
    printf("shell> ");
    fflush(stdout);

    // Leer línea de comando
    if (!fgets(line, MAX_LINE, stdin)) {
      break; // Terminar si se encuentra EOF
    }

    // Parsear línea de comando
    parse_command(line, args, &background);

    if (args[0] == NULL) {
      continue; // Si no hay comando, continuar
    }

    // Comando especial para salir
    if (strcmp(args[0], "salir") == 0) {
      printf("Saliendo...\n");
      break;
    }

    // Crear un proceso hijo para ejecutar el comando
    pid_t pid = fork();
    if (pid < 0) {
      perror("Error al crear proceso hijo");
    } else if (pid == 0) {
      // Proceso hijo
      execvp(args[0], args);
      perror("Error al ejecutar comando");
      exit(0);
    } else {
      // Proceso padre
      if (!background) {
        // Proceso foreground: esperar a que termine
        foreground_processes++;
        waitpid(pid, NULL, 0);
        foreground_processes--;
      } else {
        // Proceso background: no esperar
        printf("[Proceso %d ejecutándose en background]\n", pid);
      }
    }
  }

  return 0;
}

void h_aviso_termina(int sig) {
  int status;
  pid_t pid;
  while ((pid = waitpid(0, NULL, 0)) > 0) {
    printf("[Terminé! soy %d ]\n", pid);
  }
}

void h_salir(int sig) {
  while (foreground_processes > 0) {
    printf("Esperando la finalización de procesos foreground...\n");
    pause(); 
    printf("Todos los procesos foreground han terminado.");
  }
  printf("\n Saliendo...\n");
  exit(0);
}

void parse_command(char *input, char **args, int *background) {
  *background = 0;
  char *token = strtok(input, " \t\n");
  int i = 0;

  while (token != NULL) {
    if (strcmp(token, "&") == 0) {
      *background = 1;
    } else {
      args[i++] = token;
    }
    token = strtok(NULL, " \t\n");
  }
  args[i] = NULL; // Terminar la lista de argumentos con NULL
}
