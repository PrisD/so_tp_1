#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void P(int semid, int sem);
void V(int semid, int sem);

int main(int argc, char *argv[]) {
 
  int N = atoi(argv[1]);
 
  int semid = semget(1234, 3, IPC_CREAT | 0666);

  // Inicializa los semáforos
  semctl(semid, 0, SETVAL, 1); // semPadre: 1 
  semctl(semid, 1, SETVAL, 0); // semHijoB: 0 
  semctl(semid, 2, SETVAL, 0); // semHijoC: 0 

  pid_t pidB, pidC;

  if ((pidB = fork()) == 0) {
    // Proceso HijoB
    for (int i = 0; i < N; ++i) {
      P(semid, 1);
      printf("HijoB %d\n", i + 1);
      V(semid, 0);
    }
    exit(0);
  }

  if ((pidC = fork()) == 0) {
    // Proceso HijoC
    for (int i = 0; i < N; ++i) {
      P(semid, 2); 
      printf("HijoC %d\n", i + 1);
      V(semid, 0); 
    }
    exit(0);
  }

  // Proceso Padre
  for (int i = 0; i < N; ++i) {
    P(semid, 0); 
    printf("PadreA %d \n", i + 1);
    V(semid, 1); 

    P(semid, 0);
    printf("PadreA %d \n", i + 1);
    V(semid, 2); 
  }
  wait(NULL);
  wait(NULL);

  semctl(semid, 0, IPC_RMID, 0);

  return 0;
}
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