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
#include <errno.h>
#include "protocole.h" // contient la cle et la structure d'un message
#include "MySemaphores.h"

int idQ,idShm,idSem;
TAB_CONNEXIONS *tab;
pid_t idFils1;

void afficheTab();
void HandlerSIGINT(int numsig);
void HandlerSIGCHLD(int numsig);
void send_txt(pid_t pid_dest, MESSAGE msg);

MYSQL* connexion;

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
  struct sigaction signal_int;
  signal_int.sa_handler = HandlerSIGINT;
  signal_int.sa_flags = 0;
  if(sigemptyset(&signal_int.sa_mask) == -1)
	{
		perror("Erreur de sigemptyset.");
		exit(2);
	}
	if(sigaction(SIGINT, &signal_int, NULL) == -1)
	{
		perror("Erreur de sigaction.");
		exit(3);
	}

  struct sigaction signal_chld;
  signal_chld.sa_handler = HandlerSIGCHLD;
  sigemptyset(&signal_chld.sa_mask);
  signal_chld.sa_flags = 0;

	if(sigaction(SIGCHLD, &signal_chld, NULL) == -1)
	{
		perror("Erreur de sigaction.");
		exit(3);
	}

  if((idShm = shmget(CLE, 0, 0)) == -1)
  {
    fprintf(stderr, "(SERVEUR %d) Echec de recup de l'idShm. Creation des ressources.\n", getpid());
    
    // creation des ressources
    fprintf(stderr,"(SERVEUR %d) Creation de la file de messages.\n",getpid());
    if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
    {
      perror("(SERVEUR) Erreur de msgget. ");
      exit(1);
    }

    fprintf(stderr, "(SERVEUR %d) Creation de la memoire partagee.\n", getpid());
    if((idShm = shmget(CLE,sizeof(TAB_CONNEXIONS), IPC_CREAT | 0766)) == -1 )
    {
      perror("(SERVEUR) Erreur de shmget. ");
      exit(1);
    }
    if((tab = (TAB_CONNEXIONS *)shmat(idShm, NULL, 0)) == (TAB_CONNEXIONS *)-1) //Attachement à la mémoire partagée.
    {
      perror("(PUBLICITE) Erreur de shmat. ");
      exit(1);
    }

    fprintf(stderr, "(SERVEUR %d) Creation de la semaphore.\n", getpid());
    if((idSem = semget(CLE, 2, IPC_CREAT | 0755)) == -1)
    {
      perror("(SERVEUR) Erreur de semget. ");
      exit(1);
    }
    
    if(semctl(idSem, 0, SETVAL, 1)  == -1)
    {
      perror("(SERVEUR) Erreur de semctl. ");
      exit(1);
    }
    if(semctl(idSem, 1, SETVAL, 1)  == -1)
    {
      perror("(SERVEUR) Erreur de semctl. ");
      exit(1);
    }

    for(int i=0 ; i<6 ; i++)
    {
      tab->connexions[i].pidFenetre = 0;
      strcpy(tab->connexions[i].nom,"");
      for (int j=0 ; j<5 ; j++) tab->connexions[i].autres[j] = 0;
      tab->connexions[i].pidModification = 0;
    }
    tab->pidServeur1 = getpid();
    tab->pidServeur2 = 0;
    tab->pidAdmin = 0;

    // Creation du processus Publicite
    if((idFils1 = fork()) == -1 )
    {
      perror("Erreur de fork. ");
      exit(1);
    }

    if(idFils1 == 0)  //Processus fils seulement.
    {
      execl("./Publicite", NULL);
    }
    tab->pidPublicite = idFils1;
  }
  else
  {
    //Attachement à la mémoire partagée.
    if((tab = (TAB_CONNEXIONS *)shmat(idShm, NULL, 0)) == (TAB_CONNEXIONS *)-1) //Attachement à la mémoire partagée.
    {
      perror("(PUBLICITE) Erreur de shmat. ");
      kill(getpid(), SIGINT);
      exit(1);
    }

    //Récuperation de l'id de la file de message.
    if((idQ = msgget(CLE,0))==-1)
    {
      perror("Erreur de msgget");
      exit(1);
    }

    //Récupération de l'id des sémaphores.
    if((idSem = semget(CLE, 0, 0)) == -1)
    {
      perror("(MODIFICATION) Erreur de semget. ");
      exit(1);
    }

    sem_wait(1);  //Le serveur attend que le sémaphore 1 soit libre.
    if(tab->pidServeur1 != 0)
    {
      if((kill(tab->pidServeur1, 0) == -1) && (errno == ESRCH)) //on ne rentre dans le if que si le proc n'existe plus.
      {
        errno = 0;
        tab->pidServeur1 = getpid();
      }
      else
      {
        if(tab->pidServeur2 != 0)
        {
          if((kill(tab->pidServeur1, 0) == -1) && (errno == ESRCH)) //on ne rentre dans le if que si le proc n'existe plus.
          {
            errno = 0;
            tab->pidServeur2 = getpid();
          }
          else
          {
            fprintf(stderr, "(SERVEUR %d) Il y a déjà trop de serveurs.\n");
            kill(getpid(), SIGINT);
          }
        }
        else
          tab->pidServeur2 = getpid();
      }
    }
    else
    {
      tab->pidServeur1 = getpid();
    }
    sem_signal(1);  //Le serveur libère le sémaphore 1.

    //Récupération des id de la file de messages et des sémaphores.
    fprintf(stderr,"(SERVEUR %d) Recuperation de l'id de la file de messages.\n",getpid());
    if(( idQ = msgget(CLE,0))==-1)
    {
        perror("Erreur de msgget");
        exit(1);
    }
    fprintf(stderr, "(SERVEUR %d) Recuperation de l'id des semaphores.\n", getpid());
    if ((idSem = semget(CLE,0,0)) == -1)
    {
      perror("Erreur de semget");
      exit(1);
    }

  }

  afficheTab();
  
  int i,k,j,compteur = 0;
  MESSAGE m,reponse;
  char tmp[20],requete[256];
  MYSQL_RES  *resultat;
  MYSQL_ROW  tuple;
  PUBLICITE pub;

  while(1)
  {
  	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
    if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1, 0) == -1)
    {
      if(errno == EINTR)
      {
        errno = 0;
        continue;
      }
      perror("(SERVEUR) Erreur de msgrcv principal");
      kill(getpid(), SIGINT);
      exit(1);
    }
    //sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 à la réception d'un message.
    switch(m.requete)
    {
      case CONNECT :  //OK
            fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de client %d\n",getpid(),m.expediteur);
            i = 0;
            sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
            while((i<6) && (tab->connexions[i].pidFenetre != 0))  //On parcourt le tableau à la recherche du premier emplacement libre dans le tableau
            {
              i++;
            }
            if(i<6) 
              tab->connexions[i].pidFenetre = m.expediteur; //Enregistrement du pid du client.
            else 
              fprintf(stderr,"(SERVEUR %d) Impossible de connecter %d. Nombre max de client deja atteint.\n", getpid(), m.expediteur);
            sem_signal(1);  //Libération du sémaphore 1 (TAB).
      break; 

      case DECONNECT :  //OK
            fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de client %d\n",getpid(),m.expediteur);
            i = 0;
            sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
            while((i<6) && (tab->connexions[i].pidFenetre != m.expediteur)) //On parcourt le tableau à la recherche de la ligne correspondant au pid de l'expediteur.
              i++;

            if(i<6) 
              tab->connexions[i].pidFenetre = 0;  //On supprime le pid précédemment enregistré.
            else 
              fprintf(stderr,"(SERVEUR %d) Le client %d n'est pas connecte, impossible de le deconnecter.", getpid(), m.expediteur);
            sem_signal(1);  //Libération du sémaphore 1 (TAB).
      break; 

      case LOGIN :  //OK
            fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de client %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);
                      
            reponse.type = m.expediteur;
            reponse.expediteur = getpid();
            reponse.requete = LOGIN;
            
            sem_wait(0);  //Tentative de prise bloquante du sémaphore 0 (BD).
            sprintf(requete,"select count(*) from UNIX_FINAL where nom = '%s' and motdepasse = '%s';",m.data1, m.data2);
            mysql_query(connexion, requete);  //Requête sql à la BD.
            resultat = mysql_store_result(connexion);
            if(resultat)  
            {
              tuple = mysql_fetch_row(resultat);
              compteur = atoi(tuple[0]);  
            }
            sem_signal(0);  //Libération du sémaphore 0 (BD).

            if(compteur)  //Si le compteur est non nul, ça veut dire que le mdp et l'identifiant sont justes.
            {
              fprintf(stderr,"Resultat requete SQL : %d\n", compteur);
              i = 0;
              sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
              while((i<6) && (tab->connexions[i].pidFenetre != m.expediteur)) //On parcourt le tableau jusqu'a la bonne ligne (pid de l'expediteur).
              {
                i++;
              }
              if(i < 6)  //Si l'expéditeur existe dans la table des connexions.
              {
                strcpy(reponse.data1, "OK");
                strcpy(tab->connexions[i].nom, m.data1);  //On écrit dans le tableau le nom de l'utilisateur connecté.
              }
              else
              {
                fprintf(stderr,"(SERVEUR %d) Le client %d n'est pas connecte, impossible de LOGIN.\n", getpid(), m.expediteur);
                strcpy(reponse.data1, "KO");
              }
              sem_signal(1);  //Libération du sémaphore 1 (TAB).
            }
            else
              strcpy(reponse.data1, "KO");

            if(msgsnd(idQ,&reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1)  //Envoi de la reponse requête login.
            {
              perror("Erreur de msgsnd.\n");
              kill(getpid(), SIGINT);
              exit(1);
            }
            fprintf(stderr,"Envoi d'un signal %d a %d\n", SIGUSR1, tab->connexions[i].pidFenetre);
            kill(reponse.type, SIGUSR1); //Envoi du signal SIGUSR1 au client.

            ////////////////////////////////////////////////////////////////////

            if(compteur)
              for(i=0;i<6;i++)
                {
                  sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
                  if((strlen(tab->connexions[i].nom) != 0) && (strcmp(tab->connexions[i].nom, m.data1) != 0 )) //Si il y a un nom (si un user est login) et que ce n'est pas le nom de l'expediteur.
                  {
                    reponse.type = tab->connexions[i].pidFenetre;
                    reponse.expediteur = getpid();
                    reponse.requete = ADD_USER;
                    strcpy(reponse.data1,m.data1);
                    //L'expediteur est déjà dans la structure, il s'agit du serveur.
                    
                    fprintf(stderr, "(SERVEUR %d) Envoi de ADD_USER %s a %d.\n", getpid(),reponse.data1, reponse.type);
                    if(msgsnd(idQ,&reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1)  //Envoi du nom de l'expéditeur à l'utilisateur deja connecté.
                    {
                      perror("Erreur de msgsnd.\n");
                      kill(getpid(), SIGINT);
                      exit(1);
                    }
                    kill(tab->connexions[i].pidFenetre, SIGUSR1); //Envoi du signal SIGUSR1 au client déjà connecté.

                    reponse.type = m.expediteur;
                    strcpy(reponse.data1,tab->connexions[i].nom);
                    
                    fprintf(stderr, "(SERVEUR %d) Envoi de ADD_USER %s a %d.\n", getpid(), reponse.data1, reponse.type);
                    if(msgsnd(idQ,&reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1)  //Envoi de la reponse ADD_USER à l'expéditeur.
                    {
                      perror("Erreur de msgsnd.\n");
                      kill(getpid(), SIGINT);
                      exit(1);
                    }
                    kill(m.expediteur, SIGUSR1); //Envoi du signal SIGUSR1 au client expéditeur.
                  }
                  sem_signal(1);  //Libération du sémaphore 1 (TAB).
                }


      break; 

      case LOGOUT : //OK
            fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de client %d\n",getpid(),m.expediteur);

            i = 0;
            while((i<6) && (tab->connexions[i].pidFenetre != m.expediteur))   //La liste des pid connectes est parcouru
            {
              i++;
            }
            if(i < 6)  
            {
              strcpy(tmp,tab->connexions[i].nom);
              strcpy(tab->connexions[i].nom, ""); //On efface le nom de l'utilisateur qui vient de se logout.
              for(j=0;j<5;j++)  //Suppression des pid acceptés.
                tab->connexions[i].autres[j] = 0;
            }
            else
            {
              fprintf(stderr,"(SERVEUR %d) Le client %d n'est pas connecte, impossible de LOGOUT.\n", getpid(), m.expediteur);
              strcpy(reponse.data1, "KO");
            }

            for(i=0;i<6;i++) //On parcourt la liste
            {
              if(strlen(tab->connexions[i].nom)) //Pour chaque user logged in, on envoye un message REMOVE_USER (strlen() renvoie 0 si on a mis "" dans un vecteur).
              {
                reponse.type = tab->connexions[i].pidFenetre;
                reponse.requete = REMOVE_USER;
                //Même expéditeur qu'avant (serveur).
                strcpy(reponse.data1, tmp);  //Copie du pid à enlever chez le client.
                fprintf(stderr, "(SERVEUR %d) Envoi d'un message REMOVE_USER a : %d\n", getpid(), reponse.type);
                if(msgsnd(idQ,&reponse, sizeof(MESSAGE)-sizeof(long), IPC_NOWAIT) == -1)  //Envoi d'un message REMOVE_USER à l'utilisateur toujours connecté.
                {
                  perror("Erreur de msgsnd.\n");
                  kill(getpid(), SIGINT);
                  exit(1);
                }
                kill(reponse.type, SIGUSR1);
              }
            }
            
      break;

      case ACCEPT_USER :  //OK
            fprintf(stderr,"(SERVEUR %d) Requete ACCEPT_USER reçue de %d\n",getpid(),m.expediteur);
            i=0;
            sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
            while(i<6 && (tab->connexions[i].pidFenetre != m.expediteur)) //Recherche du pid de l'expéditeur.
              i++;

            if(i<6) //Si l'expéditeur existe dans la table de connexions.
            {
              j=0;
              while(j<6 && (strcmp(tab->connexions[j].nom, m.data1)!=0) ) //Recherche du pid de l'user à accepter.
                j++;
              
              k=0;
              while(k<5 && (tab->connexions[i].autres[k] !=0)) //Parcours du vecteur "autres" à la recherche d'une case vide.
                k++;

              if(k<5) //Si une case du vecteur "autres" est disponible.
                tab->connexions[i].autres[k] = tab->connexions[j].pidFenetre;  //Copie du pid de l'user accepté dans le vecteur "autres" de l'expéditeur.
              else
                fprintf(stderr, "(SERVEUR %d) Pas de place pour accepter cet utilisateur.\n", getpid());
            }
            else
            {
              fprintf(stderr, "(SERVEUR %d) Erreur, le user n'existe pas.\n", getpid());
            }
            sem_signal(1);  //Libération du sémaphore 1 (TAB).
      break;

      case REFUSE_USER :  //OK
            fprintf(stderr,"(SERVEUR %d) Requete REFUSE_USER reçue de %d\n",getpid(),m.expediteur);
            i=0;
            sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
            while(i<6 && (tab->connexions[i].pidFenetre != m.expediteur))  //Parcours de la table de connexions à la recherche de la ligne de l'expéditeur.
              i++;
            
            if(i<6) //Si l'expéditeur existe dans la table de connexions
            {
              j=0;
              while(j<6 && (strcmp(tab->connexions[j].nom, m.data1) !=0))  //Parcours de la table à la recherche la ligne de l'utilisateur à refuser.
                j++;

              if(j<6) //Si l'utilisateur à refuser existe dans la table de connexions.
              {
                k=0;
                for(k=0;k<5;k++)  //On parcourt tout pour s'assurer que rien de bizarre ne reste dans le vecteur.
                {
                  if(tab->connexions[i].autres[k] == tab->connexions[j].pidFenetre)
                    tab->connexions[i].autres[k] = 0; //Suppression du pid de l'utilisateur refusé dans le vecteurs "autres".
                } 
              }
              else
                fprintf(stderr, "(SERVEUR %d) L'utilisateur \"%s\" n'est pas connecte ou n'existe pas.\n", getpid(), m.data1);
            }
            else
              fprintf(stderr, "(SERVEUR %d) L'expediteur n'est pas accepté ou n'existe pas.\n", getpid());

            sem_signal(1);  //Libération du sémaphore 1 (TAB).
      break;

      case SEND : //OK
            fprintf(stderr,"(SERVEUR %d) Requete SEND reçue de %d contenant : %s\n", getpid(), m.expediteur, m.texte);
            i=0;
            sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
            while(i<6 && (tab->connexions[i].pidFenetre != m.expediteur)) //On parcourt la liste jusqu'a la ligne de l'expéditeur.
              i++;

            if(i<6) //Si l'expéditeur est trouvé.
            {
              for(j=0;j<5;j++)  //On parcourt la liste "autres".
                if(tab->connexions[i].autres[j] != 0) //Si il y'a un utilisateur.
                {
                  fprintf(stderr, "Envoi de message de %s a %d.\n", m.data1, tab->connexions[i].autres[j]);
                  //Construction du message.
                  reponse.type = tab->connexions[i].autres[j];
                  reponse.expediteur = getpid();
                  reponse.requete = SEND; 
                  strcpy(reponse.data1, m.data1);
                  strcpy(reponse.texte, m.texte);

                  if(msgsnd(idQ,&reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1)  //Envoi du message au client.
                  {
                    perror("Erreur de msgsnd.\n");
                    kill(getpid(), SIGINT);
                    exit(1);
                  }
                  fprintf(stderr, "Envoi d'un signal SIGUSR1 a %d.\n", reponse.type);
                  kill(reponse.type, SIGUSR1);  //Envoi d'un signal SIGUSR1 au client.
                }
            }
            sem_signal(1);  //Libération du sémaphore 1 (TAB).
      break; 

      case UPDATE_PUB : //OK
            fprintf(stderr,"(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n",getpid(),m.expediteur);
            sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
            for(i=0;i<6;i++)
              if(tab->connexions[i].pidFenetre) //Si un client est connecté
              {
                fprintf(stderr, "(SERVEUR %d) Envoi d'un signal SIGUSR2 a %d.\n", getpid(), tab->connexions[i].pidFenetre);
                kill(tab->connexions[i].pidFenetre, SIGUSR2); //Envoi d'un signal SIGUSR2 au client concerné.
              }
            sem_signal(1);  //Libération du sémaphore 1 (TAB).
      break;

      case CONSULT :  //OK
            fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
            if((idFils1 = fork()) == -1 ) //Création du processus fils consultation.
            {
              perror("Erreur de fork. ");
              kill(getpid(), SIGINT);
              exit(1);
            }
            if(idFils1 == 0)  //Processus fils.
              execl("./Consultation", NULL);  //Lancement de Consultation.

            fprintf(stderr, "(SERVEUR %d) Processus Consultation (PId = %d) lance.\n", getpid(), idFils1);
            reponse.type = idFils1;
            reponse.expediteur = m.expediteur;
            reponse.requete = m.requete;
            strcpy(reponse.data1, m.data1);
            if(msgsnd(idQ,&reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1)  //Envoi du message au processus consultation.
            {
              perror("Erreur de msgsnd.\n");
              exit(1);
            }
      break;

      case MODIF1 : //OK
            fprintf(stderr,"(SERVEUR %d) Requete MODIF1 reçue de %d\n",getpid(),m.expediteur);
            i = 0;
            sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
            while((i<6) &&(tab->connexions[i].pidFenetre != m.expediteur))  //Parcours de la liste à la recherche du nom qui veut modifier.
              i++;

            strcpy(reponse.data1, tab->connexions[i].nom);
            sem_signal(1);  //Libération du sémaphore 1 (TAB).
            fprintf(stderr, "(SERVEUR %d) Copie nom \"%s\". Lancement de modification.\n", getpid(), reponse.data1);

            if((idFils1 = fork()) == -1 ) //Création d'un processus fils.
            {
              perror("Erreur de fork. ");
              kill(getpid(), SIGINT);
              exit(1);
            }
            if(idFils1 == 0)  //Processus fils.
              execl("./Modification", NULL);  //Lancement de Modification.

            fprintf(stderr, "(SERVEUR %d) Processus Modification (PId = %d) lance.\n", getpid(), idFils1);
            tab->connexions[i].pidModification = idFils1; //Enregistrement du processus modifiant la BD.

            //Construction de la reponse.
            reponse.type = idFils1;
            reponse.expediteur = m.expediteur;
            reponse.requete = m.requete;
            if(msgsnd(idQ,&reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1)  //Envoi du message.
            {
              perror("Erreur de msgsnd.\n");
              exit(1);
            }
      break;

      case MODIF2 : //OK
            fprintf(stderr,"(SERVEUR %d) Requete MODIF2 reçue de %d\n",getpid(),m.expediteur);
            i = 0;
            sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
            while((i<6) && (tab->connexions[i].pidFenetre != m.expediteur)) //on parcourt le tableau à la recherche de l'utilisateur qui souhaite modifier ses informations.
              i++;

            reponse.type = tab->connexions[i].pidModification;
            sem_signal(1);  //Libération du sémaphore 1 (TAB).

            reponse.expediteur = m.expediteur;
            reponse.requete = m.requete;
            strcpy(reponse.data1, m.data1);
            strcpy(reponse.data2, m.data2);
            strcpy(reponse.texte, m.texte);
            if(msgsnd(idQ,&reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1)  //Envoi du message au processus modification.
            {
              perror("Erreur de msgsnd.\n");
              kill(getpid(), SIGINT);
              exit(1);
            }
      break;

      case LOGIN_ADMIN :  //OK
            //Construction de la réponse LOGIN_ADMIN.
            reponse.type = m.expediteur;
            reponse.expediteur = getpid();
            reponse.requete = LOGIN_ADMIN;
            fprintf(stderr,"(SERVEUR %d) Requete LOGIN_ADMIN reçue de %d\n",getpid(),m.expediteur);
            
            sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
            if(tab->pidAdmin == 0)
            {
              fprintf(stderr,"(SERVEUR %d) Requete LOGIN_ADMIN de %d reussie\n",getpid(),m.expediteur);
              tab->pidAdmin = m.expediteur;
              sem_signal(1);  //Libération du sémaphore 1 (TAB).
              strcpy(reponse.data1, "OK");

              if(msgsnd(idQ,&reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1)  //Envoi de la reponse requête login.
              {
                perror("Erreur de msgsnd.\n");
                kill(getpid(), SIGINT);
                exit(1);
              }
            }
            else 
            { 
              fprintf(stderr,"(SERVEUR %d) Requete LOGIN_ADMIN %d impossible car il y a deja un admin",getpid(),m.expediteur);
              strcpy(reponse.data1, "KO");

              if(msgsnd(idQ,&reponse, sizeof(MESSAGE)-sizeof(long), 0) == -1)  //Envoi de la reponse requête login.
              {
                perror("Erreur de msgsnd.\n");
                kill(getpid(), SIGINT);
                exit(1);
              }
            }


      break;

      case LOGOUT_ADMIN : //OK
            fprintf(stderr,"(SERVEUR %d) Requete LOGOUT_ADMIN reçue de %d\n",getpid(),m.expediteur);
            sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
            tab->pidAdmin=0;
            sem_signal(1);  //Libération du sémaphore 1 (TAB).
            fprintf(stderr,"(SERVEUR %d) Requete LOGOUT_ADMIN de %d reussie\n",getpid(),m.expediteur);
      break;

      case NEW_USER : //OK
            fprintf(stderr,"(SERVEUR %d) Requete NEW_USER reçue de %d : --%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2);
            sem_wait(0);  //Tentative de prise bloquante du sémphore 0 (BD).
            sprintf(requete,"insert into UNIX_FINAL values (NULL,'%s','%s','---','---');",m.data1,m.data2);
            mysql_query(connexion,requete);
      break;

      case DELETE_USER :  //OK
            fprintf(stderr,"(SERVEUR %d) Requete DELETE_USER reçue de %d : --%s--\n",getpid(),m.expediteur,m.data1);
            sem_wait(0);  //tentative de pris bloquante du sémphore 0 (BD).
            sprintf(requete,"delete from UNIX_FINAL where upper(nom) = upper('%s');",m.data1);  //Requete
            mysql_query(connexion,requete);
            sem_signal(0);
      break;

      case NEW_PUB :  //OK
            fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
            int fd,rc;
            if((fd = open("publicites.dat", O_RDWR|O_APPEND)) == -1)  //On tente d'ouvrir le fichier
            {
              if(errno != ENOENT)  //Si le fichier existe bel et bien
                kill(getpid(), SIGINT);
              else  //Sinon on le crée.
              {
                errno = 0;
                if((fd = open("publicites.dat", O_CREAT, 0777)) == -1)  //On tente de le créer.
                  kill(getpid(), SIGINT);
                
                kill(tab->pidPublicite, SIGUSR1); //On envoie un signal au processus publicite après avoir crée le fichier
              }
            }
            PUBLICITE tmp;
            strcpy(tmp.texte, m.texte);
            tmp.nbSecondes = atoi(m.data1);
            if(write(fd,&tmp,sizeof(PUBLICITE)) != sizeof(PUBLICITE))
            {
              perror("Erreur de write");
              exit(1);
            }
            if(close(fd))
            {
              perror("Erreur de close()");
              kill(getpid(), SIGINT);
              exit(1);
            }
      break;
    }
    //sem_signal(1);
    afficheTab();
  }
}

void afficheTab()
{
  if(sem_wait(1) == 0)
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
  sem_signal(1);
}

void HandlerSIGINT(int numsig)  //OK
{
  sem_wait(1);  //Tentative de prise bloquante du sémaphore 1 (TAB).
  fprintf(stderr, "(SERVEUR %d) Reception d'un signal SIGINT %d\n", getpid(), numsig);
  if((tab->pidServeur1 == 0) || (tab->pidServeur2 == 0))  //Si le serveur est le dernier, il supprime les ressources.
  {
    kill(tab->pidPublicite, SIGINT);
    if((msgctl(idQ, IPC_RMID, NULL) == -1)) //Suppression File de message.
    {
      perror("Erreur de suppression de la file de message. ");
      exit(1);
    }
    if((shmctl(idShm, IPC_RMID, NULL)) == -1) //Suppression Memoire partagee.
    {
      perror("Erreur de suppression de la memoire partagee. ");
      exit(1);
    }
    if((semctl(idSem, 0, IPC_RMID)) == -1) //Suppression Semaphore.
    {
      perror("Erreur de suppression de la semaphore. ");
      exit(1);
    }
    fprintf(stderr, "(SERVEUR %d) Tout a ete supprime.\n", getpid());
  }
  else
  {
    //On s'assure d'effacer le pid du serveur qui se termine.
    if(getpid() == tab->pidServeur1)
      tab->pidServeur1 = 0;
    if(getpid() == tab->pidServeur2)
      tab->pidServeur2 = 0;
  }
  sem_signal(1);  //Libération du sémaphore 1 (TAB).
  mysql_close(connexion); //Fermeture BD
  exit(0);
}

void HandlerSIGCHLD(int numsig) //OK
{
  fprintf(stderr, "(SERVEUR %d) Reception d'un signal SIGCHLD.\n", getpid());
  int ret, statut, i;
  
  if((ret = wait(&statut)) == -1) //Récupération du pid du fils qui vient de se terminer.
  {
    perror("Erreur de wait. ");
    exit(1);
  }
  
  i=0;
  while((i<6) && (tab->connexions[i].pidModification != ret)) //On parcourt le tableau à la recherche de l'entrée du fils qui s'est terminé.
    i++;

  if(i<6) //Si un tel fils existe, on l'efface.
    tab->connexions[i].pidModification = 0;

}






