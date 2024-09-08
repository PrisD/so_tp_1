#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int semid;
int n;

void P(int semid, int sem);
void V(int semid, int sem);

void procesoA();
void procesoB();
void procesoC();

int main(int argc, char *argv[]) {

  n = atoi(argv[1]);
  semid = semget(1234, 4, IPC_CREAT | 0666);
  pid_t pidA, pidB, pidC;

  semctl(semid, 0, SETVAL, 1); // ma
  semctl(semid, 1, SETVAL, 1); // mb
  semctl(semid, 2, SETVAL, 0); // mc
  semctl(semid, 3, SETVAL, 0); // mx

  //  procesos emparentados
  if ((pidA = fork()) == 0) {
    procesoA();
  }

  if ((pidB = fork()) == 0) {
    procesoB();
  }

  if ((pidC = fork()) == 0) {
    procesoC();
  }

  waitpid(pidA, NULL, 0);
  waitpid(pidB, NULL, 0);
  waitpid(pidC, NULL, 0);

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
void procesoA() {
  for (int i = 0; i < n * 2; ++i) {
    P(semid, 0); // P(ma)
    // SC
    printf("A\n");
    V(semid, 3); // V(mx)
  }
  exit(0);
}
void procesoB() {
  for (int i = 0; i < n; ++i) {
    P(semid, 1); // P(mb)
    P(semid, 3); // P(mx)
    // SC
    printf("B\n");
    V(semid, 0); // V(ma)
    V(semid, 2); // V(mc)
  }
  exit(0);
}

void procesoC() {
  for (int i = 0; i < n; ++i) {
    P(semid, 2); // P(mc)
    P(semid, 3); // P(mx)
    // SC
    printf("C\n");
    V(semid, 0); // V(ma)
    V(semid, 1); // V(mb)
  }
  exit(0);
}