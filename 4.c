#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SEM_KEY 1234

// Funciones para operaciones de semáforo
void P(int semid, int sem) {
  struct sembuf buf;
  buf.sem_num = sem;
  buf.sem_op = -1;
  buf.sem_flg = 0;
  semop(semid, &buf, 1);
}

void V(int semid, int sem) {
  struct sembuf buf;
  buf.sem_num = sem;
  buf.sem_op = 1;
  buf.sem_flg = 0;
  semop(semid, &buf, 1);
}

int semid;
int N;

// Función para crear y configurar semáforos
void init_semaphores(int semid) {
  semctl(semid, 0, SETVAL, 1); // ma
  semctl(semid, 1, SETVAL, 1); // mb
  semctl(semid, 2, SETVAL, 0); // mc
  semctl(semid, 3, SETVAL, 0); // mx
}

void processA() {
  for (int i = 0; i < N*2; ++i) {
    P(semid, 0); // P(ma)
    // Sección crítica de A
    printf("A\n");
    V(semid, 3); // V(mx)
  }
  exit(0);
}

void processB() {
  for (int i = 0; i < N; ++i) {
    P(semid, 1); // P(mb)
    P(semid, 3); // P(mx)
    // Sección crítica de B
    printf("B\n");
    V(semid, 0); // V(ma)
    V(semid, 2); // V(mc)
  }
  exit(0);
}

void processC() {
  for (int i = 0; i < N; ++i) {
    P(semid, 2); // P(mc)
    P(semid, 3); // P(mx)
    // Sección crítica de C
    printf("C\n");
    V(semid, 0); // V(ma)
    V(semid, 1); // V(mb)
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Uso: %s <N>\n", argv[0]);
    exit(1);
  }

  N = atoi(argv[1]);
  if (N <= 0) {
    fprintf(stderr, "El número de iteraciones debe ser mayor que 0.\n");
    exit(1);
  }

  semid = semget(SEM_KEY, 4, IPC_CREAT | 0666);
  if (semid < 0) {
    perror("semget");
    exit(1);
  }

  init_semaphores(semid);

  pid_t pidA, pidB, pidC;

  // Crear procesos
  if ((pidA = fork()) == 0) {
    processA();
  }

  if ((pidB = fork()) == 0) {
    processB();
  }

  if ((pidC = fork()) == 0) {
    processC();
  }

  // Esperar a que terminen los procesos hijos
  waitpid(pidA, NULL, 0);
  waitpid(pidB, NULL, 0);
  waitpid(pidC, NULL, 0);

  // Eliminar semáforos
  semctl(semid, 0, IPC_RMID, 0);

  return 0;
}
