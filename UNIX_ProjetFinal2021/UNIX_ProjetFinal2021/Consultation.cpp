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
#include "protocole.h"

#include "SemaControl.h"

int idQ,idSem;

char numGsm[20];
char email[200];
short trouve = 0;

int main()
{
  memset(numGsm, 0, 20);
  memset(email, 0, 200);


  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(CONSULTATION %d) Recuperation de l'id de la file de messages\n",getpid());
  if( (idQ = msgget(CLE, 0)) == -1){
    fprintf(stderr, "(CONSULTATION %d) Erreur msgget\n", getpid());
    exit(1);
  }

  // Recuperation de l'identifiant du sémaphore
  if( (idSem = semget(CLE, 0, 0)) == -1){
    fprintf(stderr, "(CONSULTATION %d) Erreur semget\n", getpid());
    exit(1);
  }

  // Lecture de la requête CONSULT
  MESSAGE m;
  memset(&m, 0, sizeof(MESSAGE));
  fprintf(stderr,"(CONSULTATION %d) Lecture requete CONSULT\n",getpid());
  if(msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1){
    fprintf(stderr, "(CONSULTATION %d) Erreur msgrcv\n", getpid());
    exit(1);
  }

  // Tentative de prise bloquante du semaphore 0
  fprintf(stderr,"(CONSULTATION %d) Prise bloquante du sémaphore %d\n", getpid(), SEMA_MYSQL);
  if(sem_wait(SEMA_MYSQL) == -1){
    fprintf(stderr, "(CONSULTATION %d) Erreur de semop de la fonction sem_wait\n", getpid());
  }

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
  sprintf(requete, "SELECT * FROM UNIX_FINAL WHERE nom='%s' ;", m.data1);
  mysql_query(connexion,requete),
  resultat = mysql_store_result(connexion);

  if ((tuple = mysql_fetch_row(resultat)) != NULL){
    char * val1 = tuple[1];                           // nom utilisateur
    //char * val2 = tuple[2];                         // mdp
    char * val3 = tuple[3];                           // gsm
    char * val4 = tuple[4];                           // email
    fprintf(stderr, "(CONSULTATION %d) Resultat MySql %s %s %s\n", getpid(), val1, val3, val4);
    if(strcmp(val1, m.data1) == 0){
      trouve = 1;
      strcpy(numGsm, val3);
      strcpy(email, val4);
    }
  }
  else trouve = -1;


  	
  // Construction et envoi de la reponse
  fprintf(stderr,"(CONSULTATION %d) Envoi de la reponse\n",getpid());
  MESSAGE reponse;
  memset(&reponse, 0, sizeof(MESSAGE));
  reponse.type = m.expediteur;
  reponse.expediteur = getpid();
  reponse.requete = CONSULT;

  if(trouve == 1){
    strcpy(reponse.data1, "OK");
    strcpy(reponse.data2, numGsm);
    strcpy(reponse.texte, email);
  }
  else strcpy(reponse.data1, "KO");

  if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(CONSULTATION %d) Erreur de msgsnd\n", getpid());
    exit(1);
  }
  if(kill(reponse.type, SIGUSR1) == -1){
    fprintf(stderr, "(CONSULTATION %d) Erreur de kill sur Client %d\n", getpid(), reponse.type);
    exit(1);
  }


  // Deconnexion BD
  mysql_close(connexion);

  // Libération du semaphore 0
  fprintf(stderr,"(CONSULTATION %d) Libération du sémaphore %d\n", getpid(), SEMA_MYSQL);
  if(sem_signal(SEMA_MYSQL) == -1){
    fprintf(stderr, "(CONSULTATION %d) Erreur de semop de la fonction sem_signal\n", getpid());
    exit(1);
  }

  exit(0);
}