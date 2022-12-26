#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <mysql.h>
#include <setjmp.h>
#include "protocole.h" // contient la cle et la structure d'un message

#include "SemaControl.h"


// structure arg pour initialisé les sémaphores

union semun
{
  int val;
  struct semid_ds *buf;
  unsigned short *array;
} arg;




int idQ,idShm,idSem;
TAB_CONNEXIONS * tab;

struct sigaction B, C;
void HandlerSIGINT(int);
void HandlerSIGCHLD(int);

void afficheTab();

MYSQL* connexion;

pid_t pidPub, pidConsult, pidModif1;

int main()
{
  // Connection à la BD
  connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(SERVEUR) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  // Armement des signaux
  B.sa_handler = HandlerSIGINT;
  sigemptyset(&B.sa_mask);
  B.sa_flags = SA_RESTART;

  if (sigaction(SIGINT,&B,NULL) == -1)
  {
    perror("Erreur de sigaction sur SIGUSR1");
    exit(1);
  }

  C.sa_handler = HandlerSIGCHLD;
  sigemptyset(&C.sa_mask);
  C.sa_flags = SA_RESTART;

  if(sigaction(SIGCHLD, &C, NULL) == -1){
    perror("Erreur de sigaction sur SIGCHLD");
    exit(1);
  }


  // creation des ressources

  // Recuperation de l'identifiant de la mémoire partagée
  fprintf(stderr,"(SERVEUR %d) Recuperation de l'id de la mémoire partagée\n",getpid());
  if( (idShm = shmget(CLE, 0, 0)) == -1){
    fprintf(stderr, "(SERVEUR %d) Erreur shmget\tRecup\n", getpid());
    // création de la mémoire partagé
    fprintf(stderr, "(SERVEUR %d) Creation de la memoire partage\n", getpid());
    if( (idShm = shmget(CLE, sizeof(TAB_CONNEXIONS), IPC_CREAT | IPC_EXCL | 0600)) == -1){
      perror("(SERVEUR) Erreur de shmget");
      exit(1);
    }
  }

  // Attachement à la mémoire partagée
  if( (tab = (TAB_CONNEXIONS*)shmat(idShm, NULL, 0)) == (TAB_CONNEXIONS*)-1){
    fprintf(stderr, "(PUBLICITE %d) Erreur shmat\n", getpid());
    exit(1);
  }

  if(tab->pidServeur1 == 0 && tab->pidServeur2 == 0){         // cas où le serveur n'a jamais été initialisé
    // Initilisation de la table de connexions
    for (int i=0 ; i<6 ; i++){
      tab->connexions[i].pidFenetre = 0;
      strcpy(tab->connexions[i].nom,"");
      for (int j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
      tab->connexions[i].pidModification = 0;
    }
    tab->pidServeur1 = 0;
    tab->pidServeur2 = 0;
    tab->pidAdmin = 0;
    tab->pidPublicite = 0;
  }

  if(tab->pidServeur1 == 0){
    tab->pidServeur1 = getpid();
  }
  else{
    if(kill(tab->pidServeur1, 0) == -1){    // le serveur pidServeur1 est mort donc ce serveur va le remplacé
      tab->pidServeur1 = getpid();
    }
    else{                                   // teston l'excistance du serveur 2
      if(tab->pidServeur2 == 0){
        tab->pidServeur2 = getpid();
      }
      else{
        if(kill(tab->pidServeur2, 0) == -1){  // le serveur pidServeur2 est mort donc ce serveur va le remplacé
          tab->pidServeur2 = getpid();
        }
        else {
          fprintf(stderr, "(SERVEUR %d) Pas plus de 2 Serveur en meme temps\n", getpid());
          exit(1);                         // cas où plus de 2 serveurs sont lancé en même temps et sont actif
        }
      }
    }
  }

  // récupération de la file de messages
  fprintf(stderr, "(SERVEUR %d) Recuperation de la file de messages\n", getpid());
  if( (idQ = msgget(CLE, 0)) == -1){
    // création de la file de messages
    fprintf(stderr,"(SERVEUR %d) Creation de la file de messages\n",getpid());
    if ((idQ = msgget(CLE, IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
    {
      perror("(SERVEUR) Erreur de msgget");
      exit(1);
    }
  }

  // récupération des sémaphores
  fprintf(stderr, "(SERVEUR %d) Récupération des semaphores\n", getpid());
  if( (idSem = semget(CLE, 0, 0)) == -1){
    // création des 2 sémaphores
    fprintf(stderr, "(SERVEUR %d) Creation des semaphores\n", getpid());
    if( (idSem = semget(CLE, 2, IPC_CREAT | IPC_EXCL | 0600)) == -1){
      perror("(SERVEUR) Erreur de semget");
      exit(1);
    }

    // initialisation des semaphores
    unsigned short valeurDefautSema[] = {1, 1};
    arg.array = valeurDefautSema;

    if(semctl(idSem, 0, SETALL, arg.array) == -1){
      fprintf(stderr, "(SERVEUR %d) Erreur semctl dans le HandlerSIGINT\n", getpid());
      exit(1);
    }
  }


  afficheTab();


  // Creation du processus Publicite
  if(tab->pidPublicite == 0){
    if ((pidPub = fork()) == -1) {
      perror("Erreur de fork() creation process Pub");
      exit(1);
    }
    if (pidPub == 0) {                                    // Processus fils
      if (execl("./Publicite", "Publicite",NULL) == -1)
      {
        perror("Erreur de exelp() process Pub");
        exit(1);
      }
    }
    tab->pidPublicite = pidPub;
  }


  int i,k,j;
  MESSAGE m;
  MESSAGE reponse;
  char requete[200];
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  PUBLICITE pub;

  short trouve = 0;
  short mdpCheck = 0;
  char sqlbuf[512];
  int numFields = 0;

  // prépaparation pour section critique
  sigset_t mask, oldMask;
  if(sigfillset(&mask) == -1){                                          // masquage de tous les signaux (sauf SIGKILL(-9))
    fprintf(stderr, "(SERVEUR %d) Erreur de sigfillset\n", getpid());
    exit(1);
  }
  if(sigdelset(&mask, SIGINT) == -1){   // démasquage de signal SIGINT  pour pouvoir fermé le serveur proprement même si celui-ci est dans la section critique
    fprintf(stderr, "(SERVEUR %d) Erreur de sigdelset\n", getpid());
    exit(1);
  }
                  // SIGINT est le seul signal qui peut être reçu pendant la section critique
                  // les autres seront traité en dehors de la section, par exemple SIGCHLD


  while(1)
  {
  	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());

    // réinitialisation des structs messages pour éviter les erreurs de bits "baladeurs" ...
    memset(&m, 0, sizeof(MESSAGE));
    memset(&reponse, 0, sizeof(MESSAGE));

    // debut section critique
    if(sigprocmask(SIG_SETMASK, &mask, &oldMask) == -1){                                        // application du masque et récupération de l'ancien
      fprintf(stderr, "(SERVEUR %d) Erreur de sigprocmask debut section critique\n", getpid());
      exit(1);
    }

    if (msgrcv(idQ, &m, sizeof(MESSAGE)-sizeof(long), 1, 0) == -1)                              // récupération du message
    {
      perror("(SERVEUR) Erreur de msgrcv");
      msgctl(idQ,IPC_RMID,NULL);
      exit(1);
    }


    fprintf(stderr,"(SERVEUR %d) Prise bloquante du sémaphore %d\n", getpid(), SEMA_SER);   // prise bloquante du sémaphore 1
    if(sem_wait(SEMA_SER) == -1){
      fprintf(stderr, "(SERVEUR %d) Erreur de semop de la fonction sem_wait\n", getpid());
    }

    switch(m.requete)                                                                       // début traitement de la requete
    {
      case CONNECT :  
                      fprintf(stderr, "(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);
                      trouve = 0;
                      i = 0;

                      while(i < NBCONNECTMAX && trouve == 0){
                        if(tab->connexions[i].pidFenetre == 0) trouve = 1;    // une place de libre a été trouvé ...
                        else i++;
                      }

                      if(trouve == 1){                  // une place a été trouvé
                        tab->connexions[i].pidFenetre = m.expediteur;
                      }

                      break; 

      case DECONNECT :  
                      fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);
                      trouve = 0;
                      i = 0;

                      memset(&reponse, 0, sizeof(MESSAGE));
                      reponse.expediteur = 1;
                      reponse.requete = REMOVE_USER;

                      // recherche dans la table de connection le pidFenetre
                      while(i < NBCONNECTMAX && trouve == 0){
                        if(tab->connexions[i].pidFenetre == m.expediteur) trouve = 1;
                        else i++;
                      }
                      
                      if(trouve = 1){               // vidage du tableau de connection
                        tab->connexions[i].pidFenetre = 0;
                        strcpy(reponse.data1, tab->connexions[i].nom);
                        strcpy(tab->connexions[i].nom,"");
                        for (int j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
                        tab->connexions[i].pidModification = 0;
                        fprintf(stderr, "(SERVEUR %d) Le Client %d a été déconnecté\n", getpid(), m.expediteur);

                        // requete REMOVE_USER
                        i = 0;
                        while(i < NBCONNECTMAX){
                          if(tab->connexions[i].pidFenetre != 0)
                          {
                            reponse.type = tab->connexions[i].pidFenetre;
                            if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1){
                              fprintf(stderr, "(SERVEUR %d) Erreur msgsnd REMOVE_USER Client%d\n", getpid(), reponse.type);
                              exit(1);
                            }
                            if(kill(reponse.type, SIGUSR1) == -1){
                              fprintf(stderr, "(SERVEUR %d) Erreur kill REMOVE_USER Client %d\n", getpid(), reponse.type);
                              exit(1);
                            }
                            fprintf(stderr, "(SERVEUR %d) REMOVE_USER envoyé à %d pour %s\n", getpid(), reponse.type, reponse.data1);
                          }
                          i++;
                        }

                      }
                      else{
                        fprintf(stderr, "(SERVEUR %d) Le Client %d n'a pas été trouvé dans la table de connection!\n", getpid(), m.expediteur);
                      }



                      break; 

      case LOGIN :  
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);

                      mdpCheck = 0;
                      memset(sqlbuf, 0, sizeof(sqlbuf));
                      sprintf(sqlbuf, "SELECT motdepasse FROM UNIX_FINAL WHERE nom='%s' ;", m.data1);

                      if(mysql_query(connexion, sqlbuf) != 0){          // éxécution de la requete sql
                        fprintf(stderr, "(SERVEUR %d) Erreur de SELECT (MySQL) au Login\n", getpid());
                        mdpCheck = -1;
                      }
                      resultat = mysql_store_result(connexion);         // récupération de l'entièreté des tuples
                      numFields = mysql_num_fields(resultat);           // nombre de colonne

                      while( (tuple = mysql_fetch_row(resultat)) && mdpCheck == 0){     // récupération de tuple par tuple
                        for (int j = 0; j < numFields; j++){
                          char * val = tuple[j];
                          if(strcmp(val, m.data2) == 0) mdpCheck = 1; // l'user et mot de passe correspond
                        }
                      }

                      memset(&reponse, 0, sizeof(MESSAGE));
                      reponse.type = m.expediteur;
                      reponse.expediteur = 1;
                      reponse.requete = LOGIN;
                      if(mdpCheck == 1){
                        trouve = 0;
                        i = 0;

                        // recherche du pidFenetre dans la table de co de l'utilisateur qui veut se connecté
                        while(i < NBCONNECTMAX && trouve == 0){
                          if(tab->connexions[i].pidFenetre == m.expediteur){
                            if(strcmp(tab->connexions[i].nom, "") == 0) trouve = 1;
                            else trouve = -1;
                          }
                          else i++;
                        }

                        // vérification si l'utilisateur est déjà connecté ou non
                        j = 0;
                        while(j < NBCONNECTMAX){
                          if(tab->connexions[j].pidFenetre !=0){
                            if(strcmp(tab->connexions[j].nom, m.data1) == 0) trouve = -1; // utilisateur déjà logué
                          }
                          j++;
                        }

                        if(trouve == 1){
                          strcpy(tab->connexions[i].nom, m.data1);
                          strcpy(reponse.data1, "OK");
                        }
                        else strcpy(reponse.data1, "KO");
                      
                      }
                      else strcpy(reponse.data1, "KO");

                      if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1){
                        fprintf(stderr, "(SERVEUR %d) Erreur msgsnd requete Login pour Client %d\n", getpid(), m.expediteur);
                        exit(1);
                      }

                      if(kill(m.expediteur, SIGUSR1) == -1){
                        fprintf(stderr, "(SERVEUR %d) Erreur kill requete Login pour Client %d\n", getpid(), m.expediteur);
                        exit(1);
                      }

                      // envoie de requetes ADD_USER
                      i = 0;
                      memset(&reponse, 0, sizeof(MESSAGE));
                      reponse.expediteur = 1;
                      reponse.requete = ADD_USER;

                      while(i < NBCONNECTMAX){
                        j = 0;
                        if(tab->connexions[i].pidFenetre != 0){
                          if(strcmp(tab->connexions[i].nom, "") != 0){    // le test sur le nom est seulement pour avoir les client connecté

                            reponse.type = tab->connexions[i].pidFenetre;
                            /*  on va envoyé à l'utilisateur du pid contenus dans reponse.type
                                tous les utilisateurs connectés (sauf lui même) à lui de faire le tri entre ceux qui connait ou pas
                            */
                            while(j < NBCONNECTMAX){
                              if(tab->connexions[j].pidFenetre != 0 && tab->connexions[j].pidFenetre != reponse.type){
                                if(strcmp(tab->connexions[j].nom, "") != 0){

                                  strcpy(reponse.data1, tab->connexions[j].nom);

                                  if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1){
                                    fprintf(stderr, "(SERVEUR %d) Erreur msgsnd ADD_USER pour %d\n", getpid(), reponse.type);
                                    exit(1);
                                  }
                                  if(kill(reponse.type, SIGUSR1) == -1){
                                    fprintf(stderr, "(SERVEUR %d) Erreur kill ADD_USER pour %d\n", getpid(), reponse.type);
                                    exit(1);
                                  }
                                  fprintf(stderr, "(SERVEUR %d) ADD_USER %d --> %s\n", getpid(), reponse.type, reponse.data1);
                                  sleep(1);

                                }
                              }
                              j++;
                            }

                          }
                        }
                        i++;
                      }


                      break; 

      case LOGOUT :  
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                      trouve = 0;
                      i = 0;

                      memset(&reponse, 0, sizeof(MESSAGE));
                      reponse.expediteur = 1;
                      reponse.requete = REMOVE_USER;

                      // recherche du pid fenetre dans le tab de co de l'utilisateur qui veut se logout
                      while(i < NBCONNECTMAX && trouve == 0){
                        if(tab->connexions[i].pidFenetre == m.expediteur) trouve = 1;
                        else i++;
                      }

                      strcpy(reponse.data1, tab->connexions[i].nom);
                      strcpy(tab->connexions[i].nom,"");
                      for (int j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
                      tab->connexions[i].pidModification = 0;

                      // requete REMOVE_USER
                      i = 0;
                      while(i < NBCONNECTMAX){
                        if(tab->connexions[i].pidFenetre != 0)
                        {
                          reponse.type = tab->connexions[i].pidFenetre;
                          if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1){
                            fprintf(stderr, "(SERVEUR %d) Erreur msgsnd REMOVE_USER Client%d\n", getpid(), reponse.type);
                            exit(1);
                          }
                          if(kill(reponse.type, SIGUSR1) == -1){
                            fprintf(stderr, "(SERVEUR %d) Erreur kill REMOVE_USER Client %d\n", getpid(), reponse.type);
                            exit(1);
                          }
                          fprintf(stderr, "(SERVEUR %d) REMOVE_USER envoyé à %d pour %s\n", getpid(), reponse.type, reponse.data1);
                        }
                        i++;
                      }


                      // retier le pid du client dans la table de co avec qui l'utilisateur avait accepté de parlé
                      i = 0;
                      while(i < NBCONNECTMAX){
                        j = 0;
                        while(j < (NBCONNECTMAX - 1)){
                          if(tab->connexions[i].autres[j] == m.expediteur) tab->connexions[i].autres[j] = 0;
                          j++;
                        }
                        i++;
                      }

                      break;

      case ACCEPT_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete ACCEPT_USER reçue de %d\n",getpid(),m.expediteur);

                      i = 0;
                      j = 0;
                      k = 0;
                      trouve = 0;       // recherche de la tantième connection connection du client
                      while(i < NBCONNECTMAX && trouve == 0){
                        if(tab->connexions[i].pidFenetre == m.expediteur) trouve = 1;
                        else i++;
                      }

                      trouve = 0;
                      while(j < NBCONNECTMAX && trouve == 0){   // recherche de l'amis
                        if(strcmp(tab->connexions[j].nom, m.data1) == 0) trouve = 1;
                        else j++;
                      }

                      trouve = 0;                               // recherche de la place libre dans la connection client
                      while(k < (NBCONNECTMAX - 1) && trouve == 0){
                        if(tab->connexions[i].autres[k] == 0) trouve = 1;
                        else k++;
                      }

                      tab->connexions[i].autres[k] = tab->connexions[j].pidFenetre;


                      break;

      case REFUSE_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete REFUSE_USER reçue de %d\n",getpid(),m.expediteur);

                      i = 0;
                      j = 0;
                      k = 0;
                      trouve = 0;       // recherche de la tantième connection connection du client
                      while(i < NBCONNECTMAX && trouve == 0){
                        if(tab->connexions[i].pidFenetre == m.expediteur) trouve = 1;
                        else i++;
                      }

                      trouve = 0;
                      while(j < NBCONNECTMAX && trouve == 0){   // recherche de l'amis
                        if(strcmp(tab->connexions[j].nom, m.data1) == 0) trouve = 1;
                        else j++;
                      }

                      trouve = 0;                               // recherche de la place libre dans la connection client
                      while(k < (NBCONNECTMAX - 1) && trouve == 0){
                        if(tab->connexions[i].autres[k] == tab->connexions[j].pidFenetre) trouve = 1;
                        else k++;
                      }

                      tab->connexions[i].autres[k] = 0;

                      break;

      case SEND :  
                      fprintf(stderr,"(SERVEUR %d) Requete SEND reçue de %d\n",getpid(),m.expediteur);

                      i = 0;
                      j = 0;
                      trouve = 0;

                      reponse.expediteur = 1;
                      reponse.requete = SEND;
                      strcpy(reponse.texte, m.texte);

                      while(i < NBCONNECTMAX && trouve == 0){
                        if(tab->connexions[i].pidFenetre == m.expediteur) trouve = 1;
                        else i++;
                      }
                      strcpy(reponse.data1, tab->connexions[i].nom);
                      while(j < (NBCONNECTMAX - 1)){
                        if(tab->connexions[i].autres[j] != 0){
                          reponse.type = tab->connexions[i].autres[j];
                          if(msgsnd(idQ, &reponse, sizeof(MESSAGE) - sizeof(long), 0) == -1){
                            fprintf(stderr, "(SERVEUR %d) Erreur msgsnd Client %d", getpid(), reponse.type);
                            exit(1);
                          }
                          if(kill(reponse.type, SIGUSR1) == -1){
                            fprintf(stderr, "(SERVEUR %d) Erreur kill Client %d", getpid(), reponse.type);
                            exit(1);
                          }
                          fprintf(stderr, "(SERVEUR %d) Requete SEND envoye a %d", getpid(), reponse.type);
                          sleep(1);
                        }
                        j++;
                      }


                      break; 

      case UPDATE_PUB :
                      fprintf(stderr,"(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n",getpid(),m.expediteur);
                      /*if(tab->pidPublicite != m.expediteur){
                        tab->pidPublicite = m.expediteur;
                        fprintf(stderr, "(SERVEUR %d) Le nouveau process de pub est %d\n", getpid(), m.expediteur);
                      }*/

                      // prévient les clients qu'il y a une nouvelle pub
                      i = 0;
                      while(i < NBCONNECTMAX){
                        if(tab->connexions[i].pidFenetre != 0){
                          if(kill(tab->connexions[i].pidFenetre, SIGUSR2) == -1){
                            fprintf(stderr, "(SERVEUR %d) Erreur kill UPDATE_PUB au client %d\n", getpid(), tab->connexions[i].pidFenetre);
                          }
                        }
                        i++;
                      }

                      break;

      case CONSULT :
                      fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);

                      // creation du processus Consultation
                      if( (pidConsult = fork()) == -1){
                        perror("(SERVEUR) Erreur de fork CONSULT");
                        exit(1);
                      }
                      if(pidConsult == 0){    // code fils Consultation
                        if(execl("./Consultation", "Consultation", NULL) == -1){
                          perror("(SERVEUR) Erreur de execl CONSULT");
                          exit(1);
                        }
                        exit(0);
                      }
                      // code pere Serveur
                      m.type = pidConsult;

                      if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1){
                        fprintf(stderr, "(SERVEUR %d) Erreur de msgsnd CONSULT", getpid());
                        exit(1);
                      }

                      fprintf(stderr, "(SERVEUR %d) Requete CONSULT renvoyé au process Consultation %d\n", getpid(), pidConsult);


                      break;

      case MODIF1 :
                      fprintf(stderr,"(SERVEUR %d) Requete MODIF1 reçue de %d\n",getpid(),m.expediteur);

                      i = 0;
                      trouve = 0;
                      while(i < NBCONNECTMAX && trouve == 0) {    // recherche de l'utilisateur dans la table
                        if(tab->connexions[i].pidFenetre == m.expediteur) trouve = 1;
                        else i++;
                      }
                      
                      if( (pidModif1 = fork()) == -1){                      // creation du precessus de modification
                        perror("(SERVEUR) Erreur de fork MODIF1");
                        exit(1);
                      }
                      if(pidModif1 == 0){         // code fils modication
                        if(execl("./Modification", "Modification", NULL) == -1){
                          perror("(SERVEUR) Erreur de execl Modification");
                          exit(1);
                        }
                        exit(0);
                      }

                      // code pere serveur
                      tab->connexions[i].pidModification = pidModif1;

                      // modification des données utilies pour les processus de modification
                      m.type = pidModif1;
                      strcpy(m.data1, tab->connexions[i].nom);

                      if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1){
                        fprintf(stderr, "(SERVEUR %d) Erreur de msgsnd au process Modification %d", getpid(), pidModif1);
                        exit(1);
                      }

                      break;

      case MODIF2 :
                      fprintf(stderr,"(SERVEUR %d) Requete MODIF2 reçue de %d\n",getpid(),m.expediteur);

                      i = 0;
                      trouve = 0;
                      while(i < NBCONNECTMAX && trouve == 0){                         // recherche de l'utilisateur
                        if(tab->connexions[i].pidFenetre == m.expediteur) trouve = 1;
                        else i++;
                      }
                                                                    // redirection du message vers le processus de modification
                      m.type = tab->connexions[i].pidModification;

                      if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1){
                        fprintf(stderr, "(SERVEUR %d) Erreur de msgsnd MODIF2\n", getpid());
                      }

                      fprintf(stderr, "(SERVEUR %d) Le msg MODIF2 du Client %d a ete redirige vers Modification %d\n",
                        getpid(), m.expediteur, m.type);

                      break;

      case LOGIN_ADMIN :
                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN_ADMIN reçue de %d\n",getpid(),m.expediteur);

                      if(tab->pidAdmin == 0){
                        tab->pidAdmin = m.expediteur;
                        strcpy(m.data1, "OK");
                      }
                      else strcpy(m.data1, "KO");

                      m.type = m.expediteur;
                      m.expediteur = 1;

                      if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1){
                        fprintf(stderr, "(SERVEUR %d) Erreur de msgsnd LOGIN_ADMIN %d", getpid(), m.type);
                        exit(1);
                      }

                      break;

      case LOGOUT_ADMIN :
                      fprintf(stderr,"(SERVEUR %d) Requete LOGOUT_ADMIN reçue de %d\n",getpid(),m.expediteur);

                      if(tab->pidAdmin == m.expediteur){
                        tab->pidAdmin = 0;
                        fprintf(stderr, "(SERVEUR %d) L'admin %d vient de se deconnecte\n", getpid(), m.expediteur);
                      }
                      else fprintf(stderr, "(SERVEUR %d) Un admin inconnu %d vient de se deconnecte", getpid(), m.expediteur);

                      break;

      case NEW_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_USER reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);
                      memset(sqlbuf, 0, sizeof(sqlbuf));
                      sprintf(sqlbuf, "INSERT INTO UNIX_FINAL VALUES (NULL,'%s','%s','---','---');", m.data1, m.data2);
                      if(mysql_query(connexion, sqlbuf) != 0){
                        fprintf(stderr, "(SERVEUR %d) Erreur MySQL NEW_USER\n", getpid());
                        break;
                      }
                      fprintf(stderr, "(SERVEUR %d) Ajout de l'user --%s--\n", getpid(), m.data1);

                      break;

      case DELETE_USER :
                      fprintf(stderr,"(SERVEUR %d) Requete DELETE_USER reçue de %d : --%s--\n",getpid(),m.expediteur,m.data1);
                      memset(sqlbuf, 0, sizeof(sizeof(sqlbuf)));
                      sprintf(sqlbuf, "DELETE FROM UNIX_FINAL WHERE nom = '%s';", m.data1);
                      if(mysql_query(connexion, sqlbuf) != 0){
                        fprintf(stderr, "(SERVEUR %d) Erreur MySQL DELETE_USER\n", getpid());
                        break;
                      }
                      fprintf(stderr, "(SERVEUR %d) Delete de l'user --%s--\n", getpid(), m.data1);


                      break;

      case NEW_PUB :
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);

                      {
                        PUBLICITE pub;
                        strcpy(pub.texte, m.texte);
                        pub.nbSecondes = atoi(m.data1);

                        int fd = 0;
                        if( (fd = open("publicites.dat", O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1){
                          fprintf(stderr, "(SERVEUR %d) Erreur ouverture fichier publicite.dat\n", getpid());
                          exit(1);
                        }

                        if(write(fd, &pub, sizeof(PUBLICITE)) != sizeof(PUBLICITE)){
                          fprintf(stderr, "(SERVEUR %d) Erreur de write publicite.dat\n", getpid());
                          exit(1);
                        }

                        if(close(fd) == -1){
                          fprintf(stderr, "(SERVEUR %d) Erreur de close fichier publicite.dat\n", getpid());
                          exit(1);
                        }

                        if(tab->pidPublicite != 0){
                          if(kill(tab->pidPublicite, 0) == 0){            // test si le processus publicite vis toujours
                            if(kill(tab->pidPublicite, SIGUSR1) == -1){
                              fprintf(stderr, "(SERVEUR %d) Erreur de kill SIGUSR1 sur process Publicite\n", getpid());
                            }
                          }
                          else fprintf(stderr, "(SERVEUR %d) Processus Publicite a crashé ... \n", getpid());
                        }

                      }

                      break;
    }

    // Libération du semaphore 1
    fprintf(stderr,"(SERVEUR %d) Libération du sémaphore %d\n", getpid(), SEMA_SER);
    if(sem_signal(SEMA_SER) == -1){
      fprintf(stderr, "(SERVEUR %d) Erreur de semop de la fonction sem_signal\n", getpid());
      exit(1);
    }

    if(sigprocmask(SIG_SETMASK, &oldMask, NULL) == -1){                                             // restauration de l'ancien masque
      fprintf(stderr, "(SERVEUR %d) Erreur de sigprocmask fin de la section critique\n", getpid());
      exit(1);
    }

    // fin section critique


    afficheTab();

  }
}

void afficheTab()
{
  fprintf(stderr,"Pid Serveur 1 : %d\n",tab->pidServeur1);
  fprintf(stderr,"Pid Serveur 2 : %d\n",tab->pidServeur2);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid Admin     : %d\n",tab->pidAdmin);
  for (int i=0 ; i<6 ; i++)
    fprintf(stderr,"%6d -%20s- %6d %6d %6d %6d %6d - %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].autres[0],
                                                      tab->connexions[i].autres[1],
                                                      tab->connexions[i].autres[2],
                                                      tab->connexions[i].autres[3],
                                                      tab->connexions[i].autres[4],
                                                      tab->connexions[i].pidModification);
  fprintf(stderr,"\n");
}


void HandlerSIGINT(int Sig){
  int desaloue = 0;
  fprintf(stderr, "(SERVEUR %d) Entrée dans le HandlerSIGINT\n", getpid());

  if(tab->pidServeur1 == getpid()){
    if(tab->pidServeur2 != 0){
      if(kill(tab->pidServeur2, 0) == -1) desaloue++;
    }
    else desaloue++;
    tab->pidServeur1 = 0;
  }
  else{
    if(tab->pidServeur2 == getpid()){
      if(tab->pidServeur1 != 0){
        if(kill(tab->pidServeur1, 0) == -1) desaloue++;
      }
      else desaloue++;
      tab->pidServeur2 = 0;
    }
  }

  if(desaloue > 0){
    // kill du processus publicite
    if(tab->pidPublicite != 0){
      if(kill(tab->pidPublicite, 0) == 0){                              // le processs publicite existe
        if(kill(tab->pidPublicite, SIGTSTP) == -1){
          fprintf(stderr, "(SERVEUR %d) Erreur kill Publicite %d\n", getpid(), tab->pidPublicite);
          exit(1);
        }
      }
    }

    // fermé la file de message
    if(msgctl(idQ, IPC_RMID, NULL) == -1){
      fprintf(stderr, "(SERVEUR %d) Erreur de msgctl dans le HandlerSIGINT\n", getpid());
      exit(1);
    }

    // fermé la mémoire partagé
    if(shmctl(idShm, IPC_RMID, NULL) == -1){
      fprintf(stderr, "(SERVEUR %d Erreur shmctl (mémoire partagé) dans le HandlerSIGINT\n", getpid());
      exit(1);
    }

    // fermé le sémaphore
    if(semctl(idSem, 0, IPC_RMID, NULL) == -1){
      fprintf(stderr, "(SERVEUR %d) Erreur semctl (sémaphore) dans le HandlerSIGINT\n", getpid());
      exit(1);
    }

    // fermé connection mysql
    mysql_close(connexion);
  }


  fprintf(stderr, "(SERVEUR %d) Sortie du HandlerSIGINT\n", getpid());
  exit(0);
}


void HandlerSIGCHLD(int sig){
  int pidFils = 0;
  int status;
  int i = 0;
  short trouve = 0;

  fprintf(stderr, "(SERVEUR %d) Entrée dans HandlerSIGCHLD\n", getpid());

  if( (pidFils = wait(&status)) == -1){
    fprintf(stderr, "(SERVEUR %d) Erreur de wait HandlerSIGCHLD\n", getpid());
    /*exit(1);    --> mis en commentaire, imaginon un OS qui envoie un SIGCHLD au mauvais process et çà tombe sur le serveur, ... celui-ci continue
                      mais c'est vrai que l'OS est jamais censé faire çà donc en toute logique faudrais que l'exit ne soit pas en commentaire,

                      2ème raison qu'il est en commentaire, 
                      juste pour voir ce qu'il se passe quand je fais des kill -9 sur les processus telle que Consultation, Modification et Publicite*/
  }
  if(!WIFEXITED(status)){
    fprintf(stderr, "(SERVEUR %d) Le fils %d ne sait pas terminé par un exit\n", getpid(), pidFils);
    trouve = -1;
  }
  
  while(i < NBCONNECTMAX && trouve == 0){
    if(tab->connexions[i].pidModification == pidFils) trouve = 1;
    else i++;
  }

  if(trouve == 1){
    tab->connexions[i].pidModification = 0;
    fprintf(stderr, "(SERVEUR %d) Le process Modification %d est terminé pour le Client %d\n",
      getpid(), pidFils, tab->connexions[i].pidFenetre);
  }
  else fprintf(stderr, "(SERVEUR %d) Est un process %d autre que Modification est terminé\n", getpid(), pidFils);

  fprintf(stderr, "(SERVEUR %d) Sortie du HandlerSIGCHLD\n", getpid());

}


