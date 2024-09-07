#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>

// Definir operaciones de semáforos
void sem_wait(int semid, int semnum) ;

void sem_signal(int semid, int semnum) {
  struct sembuf sb = {semnum, 1, 0};
  semop(semid, &sb, 1);
}

void proceso_A1(int semid, int N) {
  for (int i = 0; i < N; i++) {
    sem_wait(semid, 0); // Espera a que Sa sea 1
    printf("Proceso A: iteración %d\n", i + 1);
    sem_signal(semid, 1); // Señala a Sb
  }
  exit(0);
}

void proceso_B(int semid, int N) {
  for (int i = 0; i < N; i++) {
    sem_wait(semid, 1); // Espera a que Sb sea 1
    printf("Proceso B: iteración %d\n", i + 1);
    sem_signal(semid, 2); // Señala a Sc
  }
  exit(0);
}

void proceso_A2(int semid, int N) {
  for (int i = 0; i < N; i++) {
    sem_wait(semid, 2); // Espera a que Sc sea 1
    printf("Proceso A: iteración %d\n", i + 1);
    sem_signal(semid, 3); // Señala a Sx para la ejecución de C
  }
  exit(0);
}

void proceso_C(int semid, int N) {
  for (int i = 0; i < N; i++) {
    sem_wait(semid, 3); // Espera a que Sx sea 1
    printf("Proceso C: iteración %d\n", i + 1);
    sem_signal(semid, 0); // Señala a Sa para continuar con el ciclo ABAC
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Uso: %s N\n", argv[0]);
    return 1;
  }

  int N = atoi(argv[1]);

  // Crear semáforos
  int semid = semget(IPC_PRIVATE, 4, IPC_CREAT | 0660);
  if (semid == -1) {
    perror("Error de creación");
    return 1;
  }

  // Inicializar semáforos
  semctl(semid, 0, SETVAL, 1); // Sa inicia en 1 para proceso A1
  semctl(semid, 1, SETVAL, 0); // Sb inicia en 0 para proceso B
  semctl(semid, 2, SETVAL, 0); // Sc inicia en 0 para proceso A2
  semctl(semid, 3, SETVAL, 0); // Sx inicia en 0 para proceso C

  // Crear procesos
  if (fork() == 0) {
    proceso_A1(semid, N);
  }
  if (fork() == 0) {
    proceso_B(semid, N);
  }
  if (fork() == 0) {
    proceso_A2(semid, N);
  }
  if (fork() == 0) {
    proceso_C(semid, N);
  }

  // Esperar a que terminen los procesos hijos
  for (int i = 0; i < 4; i++) {
    wait(NULL);
  }

  // Eliminar semáforos
  semctl(semid, 0, IPC_RMID, 0);

  return 0;
}

void sem_wait(int semid, int semnum) {
  struct sembuf sb = {semnum, -1, 0};
  semop(semid, &sb, 1);
}
