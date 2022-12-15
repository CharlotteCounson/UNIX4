#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include "protocole.h" // contient la cle et la structure d'un message


int idQ;
int pid1,pid2;
void HandlerSIGINT(int sig);
int main()
{
  MESSAGE requete, reponse;
  pid_t destinataire;

  // Armement du signal SIGINT
  // TO DO (etape 6)
  struct sigaction  Signal;
  Signal.sa_handler=HandlerSIGINT;
  sigemptyset(&Signal.sa_mask);
  Signal.sa_flags=0;
  if(sigaction(SIGINT,&Signal,NULL)==-1)
  {
    perror("Erreur de sigaction\n");
    exit(1);
  }

  // Creation de la file de message
  fprintf(stderr,"(SERVEUR) Creation de la file de messages\n");
  // TO DO (etape 2)
  if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)
  { 
    perror("Err de msgget()");
    exit(1);
  }
  /*
  if(msgrcv(idQ,&requete,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
  {
    perror("(Client) Erreur de lecture");
    msgctl(idQ, IPC_RMID,NULL);
    exit(1);  
  }*/

  //etape2
  /*char tmp[80];
  sprintf(tmp,"(SERVEUR) %s",requete.texte);
  strcpy(requete.texte,tmp);
  requete.type = requete.expediteur;
  if(msgsnd(idQ,&requete,sizeof(MESSAGE)-sizeof(long),0) == -1)
  {
    perror("(Client) Erreur d'ecriture");
    exit(1);
  }*/


  // Attente de connection de 2 clients
  fprintf(stderr,"(SERVEUR) Attente de connection d'un premier client...\n");
  // TO DO (etape 5)
  if (msgrcv(idQ,&requete,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
  { 
    perror("(Serveur)erreur msgrcv");
    exit(1);
  }
  pid1=requete.expediteur;
  fprintf(stderr,"(SERVEUR) Attente de connection d'un second client...\n");
  // TO DO (etape 5)
  if (msgrcv(idQ,&requete,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
  { 
    perror("(Serveur)Erreure msgrcv");
    exit(1);
  }
  pid2=requete.expediteur;
  // TO DO (etapes 3, 4 et 5)
  while(1) 
  {
    
      fprintf(stderr,"(SERVEUR) Attente d'une requete...\n");
      printf("Client1 -> Client2\n");
      if(msgrcv(idQ,&requete,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
      {
        perror("(Serveur) Erreur de lecture");
        msgctl(pid1, IPC_RMID,NULL);
        exit(1);  
      }
     
      fprintf(stderr,"(SERVEUR) Requete recue de %d : --%s--\n",requete.expediteur,requete.texte);
      if(requete.expediteur==pid1)
      {
        destinataire=pid2;
      }
      else{
        destinataire=pid1;
      }

      reponse.type=destinataire;// destinataire de la réponse= ewpediteur de la requete


      fprintf(stderr,"(SERVEUR) Envoi de la reponse a %d\n",destinataire);
      char tmp[80];
      sprintf(tmp,"(SERVEUR) %s",requete.texte);
      strcpy(reponse.texte,tmp);
      if(msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
      {
        perror("(Serveur) Erreur d'ecriture");
        exit(1);
      }
      kill(destinataire,SIGUSR1);
    }
   

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TO DO (etape 6)
void HandlerSIGINT(int sig)
{
  msgctl(idQ, IPC_RMID,NULL);
  exit(1);
}