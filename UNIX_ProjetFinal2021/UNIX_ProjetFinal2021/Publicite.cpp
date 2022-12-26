#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ, idShm;
int fd = 0;

TAB_CONNEXIONS * pMem = NULL;

PUBLICITE pub;
MESSAGE msg;

struct sigaction A, B;
void HandlerSIGTSTP(int);
void handlerSIGUSR1(int);

short ouvert = 0;

int main()
{
  // Armement des signaux
  A.sa_handler = HandlerSIGTSTP;
  sigemptyset(&A.sa_mask);
  A.sa_flags = 0;

  if (sigaction(SIGTSTP, &A, NULL) == -1){
    perror("Erreur de sigaction sur SIGTSTP");
    exit(1);
  }

  B.sa_handler = handlerSIGUSR1;
  sigemptyset(&B.sa_mask);
  B.sa_flags = 0;                         // ne surtout pas mettre SA_RESTART car on va re-rentré dans le pause

  if (sigaction(SIGUSR1, &B, NULL) == -1){
    perror("Erreur de sigaction sur SIGUSR1");
    exit(1);
  }


  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());
  if( (idQ = msgget(CLE, 0)) == -1){
    fprintf(stderr, "(PUBLICITE %d) Erreur msgget\n", getpid());
    exit(1);
  }

  // Recuperation de l'identifiant de la mémoire partagée
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la mémoire partagée\n",getpid());
  if( (idShm = shmget(CLE, 0, 0)) == -1){
    fprintf(stderr, "(PUBLICITE %d) Erreur shmget\n", getpid());
    exit(1);
  }

  // Attachement à la mémoire partagée
  if( (pMem = (TAB_CONNEXIONS*)shmat(idShm, NULL, 0)) == (TAB_CONNEXIONS*)-1){
    fprintf(stderr, "(PUBLICITE %d) Erreur shmat\n", getpid());
    exit(1);
  }

  // Envoi d'une requete UPDATE_PUB au serveur pour se faire connaitre à celui-ci
    msg.type = 1;
    msg.expediteur = getpid();
    msg.requete = UPDATE_PUB;

    if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
      fprintf(stderr, "(PUBLICITE %d) Erreur msgget\n");
      exit(1);
    }

  // Ouverture du fichier de publicité
  do{
    if( (fd = open("publicites.dat", O_RDONLY)) == -1){
      fprintf(stderr, "(PUBLICITE %d) Erreur open\tMise en pause\n", getpid());
      pause();
      fprintf(stderr, "(PUBLICITE %d) Nouvelle tentative d'ouverture du fichier\n", getpid());
    }
    else ouvert = 1;
  } while(ouvert == 0);

  // prépaparation pour section critique
  sigset_t mask, oldMask;
  if(sigfillset(&mask) == -1){
    fprintf(stderr, "(PUBLICITE %d) Erreur de sigfillset\n", getpid());
    exit(1);
  }
  if(sigdelset(&mask, SIGTSTP) == -1){
    fprintf(stderr, "(PUBLICITE %d) Erreur de sigdelset\n", getpid());
    exit(1);
  }
    // SIGTSTP est le seul signal qui peut être reçu pendant la section critique
    // les autres seront traité en dehors de la section, par exemple SIGCHLD
    //(ce qui est pas le cas içi car Publicite n'as pas de fils)
    // pas besoin de démasqué SIGUSR1 car celui-ci s'éffectue en dehors du while
    // et donc en dehors de la section critique

  // debut section critiqe
  if(sigprocmask(SIG_SETMASK, &mask, &oldMask) == -1){
    fprintf(stderr, "(PUBLICITE %d) Erreur de sigprocmask debut section critique\n", getpid());
    exit(1);
  }

  while(1)
  {
    memset(&pub, 0, sizeof(PUBLICITE));
    memset(&msg, 0, sizeof(MESSAGE));

    // Lecture d'une publicité dans le fichier
    if(read(fd, &pub, sizeof(PUBLICITE)) != sizeof(PUBLICITE)){
      if(lseek(fd, 0, SEEK_SET) == -1){
        fprintf(stderr, "(PUBLICITE %d) Erreur de lseek\n", getpid());
        exit(1);
      }
      if(read(fd, &pub, sizeof(PUBLICITE)) != sizeof(PUBLICITE)){
        fprintf(stderr, "(PUBLICITE %d) Erreur de read\n", getpid());
      }
    }

    // Ecriture en mémoire partagée
    strcpy(pMem->publicite, pub.texte);

    // Envoi d'une requete UPDATE_PUB au serveur
    msg.type = 1;
    msg.expediteur = getpid();
    msg.requete = UPDATE_PUB;

    if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
      fprintf(stderr, "(PUBLICITE %d) Erreur msgget\n");
      exit(1);
    }

    fprintf(stderr, "(PUBLICITE %d)\t--%s-- a ete envoye\n", getpid(), pub.texte);

    sleep(pub.nbSecondes);
  }
}


void HandlerSIGTSTP(int sig){
  fprintf(stderr, "(PUBLICITE %d) Fermeture par SIGTSTP\n", getpid());
  if(fd != 0) close(fd);
  exit(0);
}

void handlerSIGUSR1(int sig){
  // fais rien
  // sert juste a sortir du pause
}