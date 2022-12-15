#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "protocole.h" // contient la cle et la structure d'un message
#include "MySemaphores.h"

int idQ, idShm, idSem;
int fd;
PUBLICITE* pub;
TAB_CONNEXIONS * tab;

void HandlerSIGINT(int sig);
void HandlerSIGUSR1(int sig);

int main()
{
  // Armement des signaux
  struct sigaction signal_int;
  signal_int.sa_handler = HandlerSIGINT;
  signal_int.sa_flags = 0;
  if(sigemptyset(&signal_int.sa_mask) == -1)
	{
		perror("Erreur de sigemptyset.");
		exit(1);
	}
	if(sigaction(SIGINT, &signal_int, NULL) == -1)
	{
		perror("Erreur de sigaction.");
		exit(1);
	}

  struct sigaction signal_usr1;
  signal_usr1.sa_handler = HandlerSIGUSR1;
  signal_usr1.sa_flags = 0;
  if(sigemptyset(&signal_usr1.sa_mask) == -1)
  {
    perror("Erreur de sigemptyset.");
    exit(1);
  }
  if(sigaction(SIGUSR1, &signal_usr1, NULL) == -1)
  {
    perror("Erreur de sigaction.");
    exit(1);
  }

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());
    if(( idQ = msgget(CLE,0))==-1)
    {
        perror("Erreur de msgget");
        exit(1);
    }

  // Recuperation de l'identifiant de la mémoire partagée
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la mémoire partagée\n",getpid());
  if((idShm = shmget(CLE, 0, 0)) == -1)
  {
    perror("(PUBLICITE) Erreur de shmget. ");
    exit(1);
  }

  // Attachement à la mémoire partagée
  if((tab = (TAB_CONNEXIONS *)shmat(idShm, NULL, 0)) == (TAB_CONNEXIONS *)-1)
  {
    perror("(PUBLICITE) Erreur de shmat. ");
    exit(1);
  }

  //
  if((idSem = semget(CLE, 0, 0)) == -1)
  {
    perror("(MODIFICATION) Erreur de semget. ");
    exit(1);
  }

  // Ouverture du fichier de publicité
  if((fd = open("publicites.dat", O_RDONLY)) == -1)
  {
    if(errno != ENOENT)
    {
      perror("(PUBLICITE) Erreur d'ouverture du fichier publicites.dat ");
      exit(1);
    }
    else
    {
      fprintf(stderr, "(PUBLICITE %d) Attente de création du fichier publicites.dat ...\n", getpid());
      errno = 0;
      pause();
    }
    
  }

  int nb;
  PUBLICITE tmp;
  MESSAGE m;
  while(1)
  {

    // Lecture d'une publicité dans le fichier
    nb = read(fd, &tmp, sizeof(PUBLICITE));
    if(nb == -1)
    {
      perror("(PUBLICITE) Erreur de lecture. ");
      exit(1);
    }
    if(nb < sizeof(PUBLICITE)) //Si on atteint la fin du fichier.
    {
      fprintf(stderr, "(PUBLICITE) Fin du fichier atteinte. Retour au debut.\n");
      lseek(fd, 0, SEEK_SET);
      continue; //continue repart au début de la boucle.
    }

    // Ecriture en mémoire partagée
    sem_wait(1);    
    strcpy(tab->publicite, tmp.texte); //Copie du texte lu dans la mémoire partagée.
    sem_signal(1);

    // Envoi d'une requete UPDATE_PUB au serveur
    m.type = 1;
    m.expediteur = getpid();
    m.requete = UPDATE_PUB;
    if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0) == -1)
    {
        perror("Erreur de msgsnd. ");
        exit(1);
    }
    sleep(tmp.nbSecondes);

  }
}

void HandlerSIGINT(int numsig)  //OK
{
  fprintf(stderr, "(PUBLICITE) Reception d'un signal SIGINT %d\n", numsig);
  if(shmdt(tab) == -1) //Détachement de la Memoire partagee.
  {
    perror("Erreur de detachement de la memoire partagee. ");
    exit(1);
  }

  exit(0);
}

void HandlerSIGUSR1(int sig)
{
  if((fd = open("publicites.dat", O_RDONLY)) == -1)
  {
    perror("(PUBLICITE) Erreur de open SIGUSR1. ");
  }
}
