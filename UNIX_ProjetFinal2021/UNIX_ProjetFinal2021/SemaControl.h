#ifndef SEMACONTROL
#define SEMACONTROL

// passage en param le numero du semaphore

int sem_wait(int);				// demande le semaphore	-1 en bloquant

int sem_signal(int);			// libere le semaphore	+1
								// pas besoin de fonction non bloquante car on libère donc çà se fait direct

int sem_wait_NOWAIT(int);		// demande le semaphore	-1 en non bloquant


#endif