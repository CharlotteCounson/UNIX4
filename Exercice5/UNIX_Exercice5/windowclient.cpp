#include "windowclient.h"
#include "ui_windowclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

extern WindowClient *w;

#include "protocole.h" // contient la cle et la structure d'un message


extern char nomClient[40];
int idQ; // identifiant de la file de message

void HandlerSIGUSR1(int sig);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowClient::WindowClient(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowClient)
{
  struct sigaction  S;
  S.sa_handler=HandlerSIGUSR1;
  sigemptyset(&S.sa_mask);
  S.sa_flags=0;
  ui->setupUi(this);
  setWindowTitle(nomClient);
  
  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(CLIENT %s %d) Recuperation de l'id de la file de messages\n",nomClient,getpid());
  
  // TO DO (etape 2)Récupéré id file message
  if((idQ = msgget(CLE,0))== -1)
  {
    perror("(Client) Erreur de msgget");
    exit(1);  
  }
  
  // Envoi d'une requete d'identification
  // TO DO (etape 5)
  MESSAGE test;
  test.type = 1;
  test.expediteur=getpid();
  if(msgsnd(idQ,&test,sizeof(MESSAGE)-sizeof(long),0) == -1)
  {
    perror("(Client) Erreur d'envoie d'identifiant");
    exit(1);
  }


  // Armement du signal SIGUSR1
  // TO DO (etape 4)

  if(sigaction(SIGUSR1,&S,NULL)==-1)
    {
      perror("Erreur de sigaction\n");
      exit(1);
    }
 
}

WindowClient::~WindowClient()
{
  delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setRecu(const char* Text)
{
  //fprintf(stderr,"---%s---\n",Text);
  if (strlen(Text) == 0 )
  {
    ui->lineEditRecu->clear();
    return;
  }
  ui->lineEditRecu->setText(Text);
}

void WindowClient::setAEnvoyer(const char* Text)
{
  //fprintf(stderr,"---%s---\n",Text);
  if (strlen(Text) == 0 )
  {
    ui->lineEditEnvoyer->clear();
    return;
  }
  ui->lineEditEnvoyer->setText(Text);
}

const char* WindowClient::getAEnvoyer()
{
  if (ui->lineEditEnvoyer->text().size())
  { 
    strcpy(aEnvoyer,ui->lineEditEnvoyer->text().toStdString().c_str());
    return aEnvoyer;
  }
  return NULL;
}

const char* WindowClient::getRecu()
{

  if (ui->lineEditRecu->text().size())
  { 
    strcpy(recu,ui->lineEditRecu->text().toStdString().c_str());
    printf("zzz\n");
    return recu;
  }
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonEnvoyer_clicked()
{
  
  MESSAGE Message/*,MessageRecu*/;
  Message.type = 1;
  Message.expediteur=getpid();
  //MessageRecu.typ

  strcpy(Message.texte,getAEnvoyer());
  printf("Clic sur le bouton Envoyer\n");
  // TO DO (etapes 2, 3, 4)
   printf("Envoie\n");
  if(msgsnd(idQ,&Message,sizeof(MESSAGE)-sizeof(long),0) == -1)
  {
    perror("(Client) Erreur d'ecriture");
    exit(1);
  }
  
  //Recu par le meme client Etape2
  /*if(msgrcv(idQ,&MessageRecu,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
  {
    perror("(Client) Erreur de lecture");
    exit(1);  
  }*/

  //Etape 3
  /*if(msgrcv(idQ,&MessageRecu,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
  {
    perror("(Client) Erreur de lecture");
    exit(1);  
  }

  setRecu(MessageRecu.texte);*/

}

void WindowClient::on_pushButtonQuitter_clicked()
{
  fprintf(stderr,"Clic sur le bouton Quitter\n");
  exit(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TO DO (etape 4)
void HandlerSIGUSR1(int sig)
{
  printf("Reception signal %d",sig);
  MESSAGE MessageRecu;
  MessageRecu.type = 1;
  MessageRecu.expediteur=getpid(); 
  if(msgrcv(idQ,&MessageRecu,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
  {
    perror("(Client) Erreur de lecture");
    exit(1);  
  }
  w->setRecu(MessageRecu.texte);
}