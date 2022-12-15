#ifndef MY_SEMAPHORES_H
#define MY_SEMAPHORES_H

int sem_wait(int num);  //Tentative de prise BLOQUANTE
int sem_try(int num);   //Tentative de prise NON-BLOQUANTE
int sem_signal(int num);    //Liberation du sémaphore.

#endif