#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <mysql.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "protocole.h"
#include "./MySemaphores.h"
int idQ,idSem;
key_t key = CLE;

int main()
{
  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(CONSULTATION %d) Recuperation de l'id de la file de messages\n",getpid());
  if((idQ=msgget(key,0))==-1)
  {
    perror("Erreur de msgget");
    exit(0);
  }
  printf("idQ = %d\n",idQ);
  /*********************************************************/
  /*********************************************************/


  // Recuperation de l'identifiant du sémaphore
  if ((idSem = semget(key,0,0)) == -1)
  {
    perror("Erreur de semget");
    exit(1);
  }
  printf("idSem = %d\n",idSem);
  /*********************************************************/
  /*********************************************************/


  MESSAGE m;
  // Lecture de la requête CONSULT
  fprintf(stderr,"(CONSULTATION %d) Lecture requete CONSULT\n",getpid());
  if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1) 
  {
    perror("(CONSULTATION) Erreur de msgrcv : ");
    exit(1);
  }
  /*********************************************************/
  /*********************************************************/

  // Tentative de prise bloquante du semaphore 0
  if(sem_wait(0)==-1) 
    perror("(CONSULTATION) Erreur de semop (start) : ");

  // Connexion à la base de donnée
  MYSQL *connexion = mysql_init(NULL);
  fprintf(stderr,"(CONSULTATION %d) Connexion à la BD\n",getpid());
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(CONSULTATION) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Recherche des infos dans la base de données
  fprintf(stderr,"(CONSULTATION %d) Consultation en BD (%s)\n",getpid(),m.data1);
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  char requete[200];
  //sprintf(requete,...);
  sprintf(requete, "select gsm, email from UNIX_FINAL where nom = '%s';", m.data1);
  mysql_query(connexion,requete),
  resultat = mysql_store_result(connexion);
  
  m.type = m.expediteur;
  m.expediteur = getpid();
  
  if ((tuple = mysql_fetch_row(resultat)) != NULL)
  {
    fprintf(stderr, "(CONSULTATION %d) Resultat trouve en base de donnes.\n", getpid());
    strcpy(m.data1,"OK"); 
    strcpy(m.data2,tuple[0]);
    strcpy(m.texte,tuple[1]);
  }
  else
  {
    fprintf(stderr,"(CONSULTATION %d) Resultat inexistant.\n",getpid());
    strcpy(m.data1,"KO");
  }
  fprintf(stderr,"(CONSULTATION %d) Envoi de la reponse\n",getpid());
  if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0) == -1) //Envoi de la reponse "CONSULT".
  {
    perror("Erreur de msgsnd");
    exit(0);
  }
  kill(m.type, SIGUSR1);
  // Deconnexion BD
  mysql_close(connexion);

  // Libération du semaphore 0
  fprintf(stderr,"(CONSULTATION %d) Libération du sémaphore 0\n",getpid());
  if(sem_signal(0)==-1) 
    perror("(CONSULTATION %d) Erreur de semop (end) : ");

  exit(0);
}