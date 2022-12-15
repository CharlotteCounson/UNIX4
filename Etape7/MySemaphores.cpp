#include "./MySemaphores.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

extern int idSem;

int sem_wait(int num)   //Prise bloquante
{
    struct sembuf action;
    action.sem_num = num;
    action.sem_op = -1;
    action.sem_flg = SEM_UNDO;
    return semop(idSem, &action, 1);
}

int sem_try(int num)    //Prise non-bloquante
{
    struct sembuf action;
    action.sem_num = num;
    action.sem_op = -1;
    action.sem_flg = IPC_NOWAIT | SEM_UNDO;
    return semop(idSem, &action, 1);
}

int sem_signal(int num) //Libération sémaphore
{
    struct sembuf action;
    action.sem_num = num;
    action.sem_op = +1;
    action.sem_flg = SEM_UNDO;
    return semop(idSem, &action, 1);
}