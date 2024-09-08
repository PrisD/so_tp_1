#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t mA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mB = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mC = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mX = PTHREAD_MUTEX_INITIALIZER;

void *procesoA(void *arg);
void *procesoB(void *arg);
void *procesoC(void *arg);

int main(int argc, char *argv[]) {

  int n = atoi(argv[1]);

  pthread_t hiloA, hiloB, hiloC;

  pthread_mutex_lock(&mC);
  pthread_mutex_lock(&mX);

  for (int i = 0; i < n; i++) {
    pthread_create(&hiloA, NULL, procesoA, NULL);
    pthread_create(&hiloB, NULL, procesoB, NULL);
    pthread_create(&hiloA, NULL, procesoA, NULL);
    pthread_create(&hiloC, NULL, procesoC, NULL);
  }

  for (int i = 0; i < n; i++) {
    pthread_join(hiloA, NULL);
    pthread_join(hiloB, NULL);
    pthread_join(hiloC, NULL);
  }

  pthread_mutex_destroy(&mA);
  pthread_mutex_destroy(&mB);
  pthread_mutex_destroy(&mC);
  pthread_mutex_destroy(&mX);

  printf("\n");
  return 0;
}
void *procesoA(void *arg) {
  pthread_mutex_lock(&mA);
  printf("A");
  pthread_mutex_unlock(&mX);
  pthread_exit(0);
}
void *procesoB(void *arg) {
  pthread_mutex_lock(&mB);
  pthread_mutex_lock(&mX);
  printf("B");
  pthread_mutex_unlock(&mA);
  pthread_mutex_unlock(&mC);
  pthread_exit(0);
}
void *procesoC(void *arg) {
  pthread_mutex_lock(&mC);
  pthread_mutex_lock(&mX);
  printf("C");
  pthread_mutex_unlock(&mA);
  pthread_mutex_unlock(&mB);
  pthread_exit(0);
}