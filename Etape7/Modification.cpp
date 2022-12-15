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
#include "MySemaphores.h" //Fonctions personnalisées crées dans le but de manipuler les sémaphores.

int idQ,idSem;

int main()
{
  char nom[40];

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(MODIFICATION %d) Recuperation de l'id de la file de messages\n",getpid());
  if((idQ = msgget(CLE,0))==-1)
  {
      perror("Erreur de msgget");
      exit(1);
  }
  fprintf(stderr, "idQ = %d\n", idQ);

  // Recuperation de l'identifiant du sémaphore
  if((idSem = semget(CLE, 0, 0)) == -1)
  {
    perror("(MODIFICATION) Erreur de semget. ");
    exit(1);
  }
  fprintf(stderr, "idSem = %d\n", idSem);

  MESSAGE m;

  // Lecture de la requête MODIF1
  fprintf(stderr,"(MODIFICATION %d) Lecture requete MODIF1\n",getpid());
  if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
  {
    perror("(SERVEUR) Erreur de msgrcv");
    exit(1);
  }
  fprintf(stderr, "(MODIFICATION %d) Reception de %s du serveur.\n", getpid(), m.data1);

  //Le type est changé avant la tentative de prise non-bloquante.
  m.type = m.expediteur;
  m.expediteur = getpid();
  // Tentative de prise non bloquante du semaphore 0 (au cas où un autre utilisateut est déjà en train de consulter ou modifier)
  if(sem_try(0) == -1)
  {
    //Si tentative échouée.
    perror("ERREUR sem_try. ");
    fprintf(stderr, "(MODIFICATION %d) Tentative de prise non bloquante echouee.\nRequete KO en cours d'envoi...\n", getpid());
    strcpy(m.data1, "KO");
    strcpy(m.data2, "KO");
    strcpy(m.texte, "KO");
    if(msgsnd(idQ,&m, sizeof(MESSAGE)-sizeof(long), 0) == -1) //Envoi d'un KO au client.
    {
      perror("Erreur de msgsnd.\n");
      exit(1);
    }
    exit(2);
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
  sprintf(requete, "select motdepasse, gsm, email from UNIX_FINAL where nom = '%s';", nom);
  mysql_query(connexion,requete);
  resultat = mysql_store_result(connexion);
  tuple = mysql_fetch_row(resultat); // user existe forcement

  // Construction et envoi de la reponse
  strcpy(m.data1, tuple[0]);
  strcpy(m.data2, tuple[1]);
  strcpy(m.texte, tuple[2]);

  fprintf(stderr, "(MODIFICATION) mdp : %s   gsm : %s   email : %s\n", m.data1, m.data2, m.texte);

  fprintf(stderr,"(MODIFICATION %d) Envoi de la reponse\n",getpid());
  if(msgsnd(idQ, &m, sizeof(MESSAGE)-sizeof(long), 0) == -1)  //Envoi de la reponse MODIF1 au client.
  {
      perror("Erreur de msgsnd");
      exit(1);
  }

  // Attente de la requête MODIF2
  fprintf(stderr,"(MODIFICATION %d) Attente requete MODIF2...\n",getpid());
  if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
  {
    perror("(SERVEUR) Erreur de msgrcv");
    exit(1);
  }

  // Mise à jour base de données
  fprintf(stderr,"(MODIFICATION %d) Modification en base de données pour --%s--\n",getpid(),nom);
  //sprintf(requete,...);
  sprintf(requete, "update UNIX_FINAL set motdepasse = '%s', gsm = '%s', email = '%s' where UPPER(nom) = UPPER('%s');", m.data1, m.data2, m.texte, nom);
  mysql_query(connexion,requete);
  
  // Deconnexion BD
  mysql_close(connexion);

  // Libération du semaphore 0
  fprintf(stderr,"(MODIFICATION %d) Liberation du semaphore 0.\n",getpid());
  if(sem_signal(0) == -1)
  {
    perror("(CONSULTATION) Erreur de liberation du semaphore 0.\n");
    exit(1);
  }
  exit(0);
}