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

char numTuple[32];
char nomUser[20];
char mdpUser[20];
char gsmUser[20];
char emailUser[200];

int main()
{
  char nom[20];

  memset(nom, 0, 20);
  memset(numTuple, 0, 32);
  memset(nomUser, 0, 20);
  memset(mdpUser, 0, 20);
  memset(gsmUser, 0, 20);
  memset(emailUser, 0, 20);

  strcpy(numTuple, "0");
  strcpy(nomUser, "KO");
  strcpy(mdpUser, "KO");
  strcpy(gsmUser, "KO");
  strcpy(emailUser, "KO");

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(MODIFICATION %d) Recuperation de l'id de la file de messages\n",getpid());
  if( (idQ = msgget(CLE, 0)) == -1){
    fprintf(stderr, "(MODIFICATION %d) Erreur de msgget\n", getpid());
    exit(1);
  }

  // Recuperation de l'identifiant du sémaphore
  if( (idSem = semget(CLE, 0, 0)) == -1){
    fprintf(stderr, "(MODIFICATION %d) Erreur de semget\n", getpid());
    exit(1);
  }

  MESSAGE m;
  // Lecture de la requête MODIF1
  fprintf(stderr,"(MODIFICATION %d) Lecture requete MODIF1\n",getpid());
  if(msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1){
    fprintf(stderr, "(MODIFICATION %d) Erreur de msgrcv MODIF1\n", getpid());
    exit(1);
  }

  // Tentative de prise non bloquante du semaphore 0 (au cas où un autre utilisateur est déjà en train de modifier)
  if(sem_wait_NOWAIT(SEMA_MYSQL) == -1){
    fprintf(stderr, "(MODIFICATION %d) Erreur de semop de la fonction sem_wait\n", getpid());
    strcpy(m.data1, "KO");
    strcpy(m.data2, "KO");
    strcpy(m.texte, "KO");
    m.type = m.expediteur;
    m.expediteur = getpid();
    if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1){
      fprintf(stderr, "(MODIFICATION %d) Erreur de msgsnd (sem_wait_NOWAIT(SEMA_MYSQL))\n", getpid());
      exit(1);
    }
    exit(0);
  }

  // Connexion à la base de donnée
  MYSQL *connexion = mysql_init(NULL);
  fprintf(stderr,"(MODIFICATION %d) Connexion à la BD\n",getpid());
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(MODIFICATION) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Recherche des infos dans la base de données
  fprintf(stderr,"(MODIFICATION %d) Consultation en BD pour --%s--\n",getpid(),m.data1);
  strcpy(nom,m.data1);
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  char requete[200];
  // sprintf(requete,...);
  sprintf(requete, "SELECT * FROM UNIX_FINAL WHERE nom = '%s';", m.data1);
  mysql_query(connexion,requete);
  resultat = mysql_store_result(connexion);
  tuple = mysql_fetch_row(resultat); // user existe forcement

  strcpy(numTuple, tuple[0]);
  strcpy(nomUser, tuple[1]);
  strcpy(mdpUser, tuple[2]);
  strcpy(gsmUser, tuple[3]);
  strcpy(emailUser, tuple[4]);


  // Construction et envoi de la reponse
  fprintf(stderr, "(MODIFICATION %d) Envoi de la reponse\n",getpid());
  m.type = m.expediteur;
  m.expediteur = getpid();
  m.requete = MODIF1;
  strcpy(m.data1, mdpUser);
  strcpy(m.data2, gsmUser);
  strcpy(m.texte, emailUser);

  if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(MODIFICATION %d) Erreur msgsnd MODIF1 Client %d\n", getpid(), m.type);
    exit(1);
  }
  
  // Attente de la requête MODIF2
  fprintf(stderr,"(MODIFICATION %d) Attente requete MODIF2...\n",getpid());
  if(msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1){
    fprintf(stderr, "(MODIFICATION %d) Erreur msgrcv MODIF2\n", getpid());
    exit(1);
  }

  // Mise à jour base de données
  fprintf(stderr,"(MODIFICATION %d) Modification en base de données pour --%s--\n", getpid(), nom);
  memset(requete, 0, 200);
  sprintf(
    requete, 
    "UPDATE UNIX_FINAL SET motdepasse='%s', gsm='%s', email='%s' WHERE id=%d;",
    m.data1,
    m.data2,
    m.texte,
    atoi(numTuple));

  mysql_query(connexion,requete);

  // Deconnexion BD
  mysql_close(connexion);

  // Libération du semaphore 0
  fprintf(stderr,"(MODIFICATION %d) Libération du sémaphore 0\n",getpid());
  if(sem_signal(SEMA_MYSQL) == -1){
    fprintf(stderr, "(MODIFICATION %d) Erreur de semop de la fonction sem_signal\n", getpid());
    exit(1);
  }

  fprintf(stderr, "(MODIFICATION %d) Termine avec succes pour Client %d\n", getpid(), m.expediteur);

  exit(0);
}