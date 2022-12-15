#include "windowclient.h"
#include "ui_windowclient.h"
#include <QMessageBox>
#include "dialogmodification.h"
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
#include <setjmp.h>
#include "protocole.h"

#define TIME_OUT 120

extern WindowClient *w;
int idQ, idShm;
int timeOut = TIME_OUT;
key_t key = CLE;//
TAB_CONNEXIONS* tab;

void HandlerSIGUSR1(int sig);
void HandlerSIGUSR2(int sig);
void HandlerSIGALRM(int sig);
void ResetALRM(void);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowClient::WindowClient(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowClient)
{
    ui->setupUi(this);
    logoutOK();
    MESSAGE msg_connect; //Structure message pour la connexion.

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la file de messages\n",getpid());
    if((idQ=msgget(key,0))==-1)
    {
        perror("Erreur de msgget");
        exit(0);
    }
    printf("idQ = %d\n",idQ);


/*********************************************************/
    // Mémoire partagée
/*********************************************************/
    // Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la mémoire partagée\n",getpid());
    if ((idShm = shmget(key,0,0)) == -1)
    {
      perror("Erreur de shmget");
      exit(1);
    }
    fprintf(stderr,"idShm = %d\n",idShm);

    
      // Attachement à la mémoire partagée
    if ((tab = (TAB_CONNEXIONS*)shmat(idShm,NULL,SHM_RDONLY)) == (TAB_CONNEXIONS*)-1)
    {
      perror("Erreur de shmat");
      exit(1);
    }
    

/*********************************************************/
    // Armement des signaux
/*********************************************************/
    // Signal SIGUSR1
    struct sigaction a;
    a.sa_flags = 0;
	  a.sa_handler = HandlerSIGUSR1;
	  if(sigemptyset(&a.sa_mask) == -1)
	  {
	  	perror("(ARMEMENT SIGUSR1) : Erreur de sigemptyset.");
	  	exit(2);
	  }
	  if(sigaction(SIGUSR1, &a, NULL) == -1)
	  {
	  	perror("(ARMEMENT SIGUSR1) : Erreur de sigaction.");
	  	exit(3);
	  }


    // Signal SIGUSR2
    struct sigaction a2;
    a2.sa_flags = 0;
	  a2.sa_handler = HandlerSIGUSR2;
	  if(sigemptyset(&a2.sa_mask) == -1)
	  {
	  	perror("(ARMEMENT SIGUSR2) : Erreur de sigemptyset.");
	  	exit(2);
	  }
	  if(sigaction(SIGUSR2, &a2, NULL) == -1)
	  {
	  	perror("(ARMEMENT SIGUSR2) : Erreur de sigaction.");
	  	exit(3);
	  }


    // Signal SIGALRM
    struct sigaction b;
    b.sa_flags = 0;
    b.sa_handler = HandlerSIGALRM;
	  if(sigemptyset(&b.sa_mask) == -1)
	  {
	  	perror("(ARMEMENT SIGALRM) : Erreur de sigemptyset.");
	  	exit(2);
	  }
    if (sigaction(SIGALRM,&b,NULL) == -1)
    {
      perror("(ARMEMENT SIGALRM) : Erreur de sigaction");
      exit(1);
    }

    // Envoi d'une requete de connexion au serveur
    msg_connect.type = 1;
    msg_connect.expediteur = getpid();
    msg_connect.requete = CONNECT;
    if (msgsnd(idQ,&msg_connect,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1)
    {
        perror("Erreur de msgsnd");
        exit(0);
    }

}

WindowClient::~WindowClient()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNom()
{
  strcpy(connectes[0],ui->lineEditNom->text().toStdString().c_str());
  return connectes[0];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPublicite(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditPublicite->clear();
    return;
  }
  ui->lineEditPublicite->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setTimeOut(int nb)
{
  ui->lcdNumberTimeOut->display(nb);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setAEnvoyer(const char* Text)
{
  //fprintf(stderr,"---%s---\n",Text);
  if (strlen(Text) == 0 )
  {
    ui->lineEditAEnvoyer->clear();
    return;
  }
  ui->lineEditAEnvoyer->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getAEnvoyer()
{
  strcpy(aEnvoyer,ui->lineEditAEnvoyer->text().toStdString().c_str());
  return aEnvoyer;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPersonneConnectee(int i,const char* Text)
{
  if (strlen(Text) == 0 )
  {
    switch(i)
    {
        case 1 : ui->lineEditConnecte1->clear(); break;
        case 2 : ui->lineEditConnecte2->clear(); break;
        case 3 : ui->lineEditConnecte3->clear(); break;
        case 4 : ui->lineEditConnecte4->clear(); break;
        case 5 : ui->lineEditConnecte5->clear(); break;
    }
    return;
  }
  switch(i)
  {
      case 1 : ui->lineEditConnecte1->setText(Text); break;
      case 2 : ui->lineEditConnecte2->setText(Text); break;
      case 3 : ui->lineEditConnecte3->setText(Text); break;
      case 4 : ui->lineEditConnecte4->setText(Text); break;
      case 5 : ui->lineEditConnecte5->setText(Text); break;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getPersonneConnectee(int i)
{
  QLineEdit *tmp;
  switch(i)
  {
    case 1 : tmp = ui->lineEditConnecte1; break;
    case 2 : tmp = ui->lineEditConnecte2; break;
    case 3 : tmp = ui->lineEditConnecte3; break;
    case 4 : tmp = ui->lineEditConnecte4; break;
    case 5 : tmp = ui->lineEditConnecte5; break;
    default : return NULL;
  }

  strcpy(connectes[i],tmp->text().toStdString().c_str());
  return connectes[i];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::ajouteMessage(const char* personne,const char* message)
{
  // Choix de la couleur en fonction de la position
  int i=1;
  bool trouve=false;
  while (i<=5 && !trouve)
  {
      if (getPersonneConnectee(i) != NULL && strcmp(getPersonneConnectee(i),personne) == 0) trouve = true;
      else i++;
  }
  char couleur[40];
  if (trouve)
  {
      switch(i)
      {
        case 1 : strcpy(couleur,"<font color=\"red\">"); break;
        case 2 : strcpy(couleur,"<font color=\"blue\">"); break;
        case 3 : strcpy(couleur,"<font color=\"green\">"); break;
        case 4 : strcpy(couleur,"<font color=\"darkcyan\">"); break;
        case 5 : strcpy(couleur,"<font color=\"orange\">"); break;
      }
  }
  else strcpy(couleur,"<font color=\"black\">");
  if (strcmp(getNom(),personne) == 0) strcpy(couleur,"<font color=\"purple\">");

  // ajout du message dans la conversation
  char buffer[300];
  sprintf(buffer,"%s(%s)</font> %s",couleur,personne,message);
  ui->textEditConversations->append(buffer);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNomRenseignements(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNomRenseignements->clear();
    return;
  }
  ui->lineEditNomRenseignements->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNomRenseignements()
{
  strcpy(nomR,ui->lineEditNomRenseignements->text().toStdString().c_str());
  return nomR;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setGsm(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditGsm->clear();
    return;
  }
  ui->lineEditGsm->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setEmail(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditEmail->clear();
    return;
  }
  ui->lineEditEmail->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setCheckbox(int i,bool b)
{
  QCheckBox *tmp;
  switch(i)
  {
    case 1 : tmp = ui->checkBox1; break;
    case 2 : tmp = ui->checkBox2; break;
    case 3 : tmp = ui->checkBox3; break;
    case 4 : tmp = ui->checkBox4; break;
    case 5 : tmp = ui->checkBox5; break;
    default : return;
  }
  tmp->setChecked(b);
  if (b) tmp->setText("Accepté");
  else tmp->setText("Refusé");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::loginOK()
{
  ui->pushButtonLogin->setEnabled(false);
  ui->pushButtonLogout->setEnabled(true);
  ui->lineEditNom->setReadOnly(true);
  ui->lineEditMotDePasse->setReadOnly(true);
  ui->pushButtonEnvoyer->setEnabled(true);
  ui->pushButtonConsulter->setEnabled(true);
  ui->pushButtonModifier->setEnabled(true);
  ui->checkBox1->setEnabled(true);
  ui->checkBox2->setEnabled(true);
  ui->checkBox3->setEnabled(true);
  ui->checkBox4->setEnabled(true);
  ui->checkBox5->setEnabled(true);
  ui->lineEditAEnvoyer->setEnabled(true);
  ui->lineEditNomRenseignements->setEnabled(true);
  setTimeOut(TIME_OUT);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::logoutOK()
{
  ui->pushButtonLogin->setEnabled(true);
  ui->pushButtonLogout->setEnabled(false);
  ui->lineEditNom->setReadOnly(false);
  ui->lineEditNom->setText("");
  ui->lineEditMotDePasse->setReadOnly(false);
  ui->lineEditMotDePasse->setText("");
  ui->pushButtonEnvoyer->setEnabled(false);
  ui->pushButtonConsulter->setEnabled(false);
  ui->pushButtonModifier->setEnabled(false);
  for (int i=1 ; i<=5 ; i++)
  {
      setCheckbox(i,false);
      setPersonneConnectee(i,"");
  }
  ui->checkBox1->setEnabled(false);
  ui->checkBox2->setEnabled(false);
  ui->checkBox3->setEnabled(false);
  ui->checkBox4->setEnabled(false);
  ui->checkBox5->setEnabled(false);
  setNomRenseignements("");
  setGsm("");
  setEmail("");
  ui->textEditConversations->clear();
  setAEnvoyer("");
  ui->lineEditAEnvoyer->setEnabled(false);
  ui->lineEditNomRenseignements->setEnabled(false);
  setTimeOut(TIME_OUT);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogin_clicked() //OK.
{
  // TO DO
  fprintf(stderr,"Bouton LOGIN clique.\n");

  MESSAGE msg_login;
  msg_login.type = 1;    //Type 1 signifie que les message est destiné au serveur.
  msg_login.expediteur = getpid();
  msg_login.requete = LOGIN;  //Message de type "LOGIN".
  strcpy(msg_login.data1, getNom());
  strcpy(msg_login.data2, getMotDePasse());
  
  if(msgsnd(idQ,&msg_login,sizeof(MESSAGE)-sizeof(long), IPC_NOWAIT) == -1) //Envoi d'une requete LOGIN.
  {
      perror("Erreur de msgsnd");
      exit(0);
  }
}

void WindowClient::on_pushButtonLogout_clicked()  //OK.
{
  // TO DO
  logoutOK();
  fprintf(stderr,"Bouton LOGOUT clique.\n");

  MESSAGE msg_logout;
  msg_logout.type = 1;    //Type 1 signifie que les message est destiné au serveur.
  msg_logout.expediteur = getpid();
  msg_logout.requete = LOGOUT;  //Message de type "LOGOUT".
  
  if(msgsnd(idQ,&msg_logout,sizeof(MESSAGE)-sizeof(long), IPC_NOWAIT) == -1) //Envoi de la requête "LOGOUT".
  {
      perror("Erreur de msgsnd");
      exit(0);
  }
  fprintf(stderr,"Message envoye. Sortie de la méthode.\n");
  alarm(0);
  timeOut=TIME_OUT;
  setTimeOut(timeOut);
}

void WindowClient::on_pushButtonQuitter_clicked() //OK
{
  // TO DO
  fprintf(stderr,"Bouton Quitter clique.\n");
  MESSAGE msg_quit;
  msg_quit.type = 1;
  msg_quit.expediteur = getpid();
  msg_quit.requete = LOGOUT;  //Message de type "LOGOUT" en premier lieu. 
  fprintf(stderr,"(CLIENT %d) Envoi d'un message LOGOUT.\n",getpid());
  if(msgsnd(idQ,&msg_quit,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "LOGOUT".
  {
    perror("Erreur de msgsnd");
    exit(0);
  }

  msg_quit.requete = DECONNECT;
  fprintf(stderr,"(CLIENT %d) Envoi d'un message DECONNECT.\n",getpid());
  if(msgsnd(idQ,&msg_quit,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "DECONNECT".
  {
    perror("Erreur de msgsnd");
    exit(0);
  }
  // Detachement de la memoire partagee
  if(shmdt(tab) == -1)
  {
    perror("Erreur de shmdt");
    exit(1);
  }
  exit(0);
}

void WindowClient::on_pushButtonEnvoyer_clicked()
{
  MESSAGE m;
  if((strlen(w->getAEnvoyer()) > 0))
  {
    m.type = 1;
    m.expediteur = getpid();
    m.requete = SEND; 
    strcpy(m.data1, getNom());
    strcpy(m.texte, w->getAEnvoyer());
    ajouteMessage(m.data1, m.texte);
    if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0) == -1) //Envoi de la requête "SEND".
    {
      perror("Erreur de msgsnd");
      exit(0);
    }
    w->setAEnvoyer("");
  }
  ResetALRM();
}

void WindowClient::on_pushButtonConsulter_clicked()
{
  fprintf(stderr,"Bouton Consulter clique.\n");
  ResetALRM();
  MESSAGE msg_consult;
  msg_consult.type = 1;
  msg_consult.expediteur = getpid();
  msg_consult.requete = CONSULT;  //Message de type "CONSULT" en premier lieu. 
  strcpy(msg_consult.data1,getNomRenseignements());
  fprintf(stderr,"(CLIENT %d) Envoi d'un message CONSULT.\n",getpid());
  if(msgsnd(idQ,&msg_consult,sizeof(MESSAGE)-sizeof(long), 0) == -1) //Envoi de la requête "CONSULT".
  {
    perror("Erreur de msgsnd");
    exit(0);
  }
  w->setGsm("EN ATTENTE...");
  w->setEmail("EN ATTENTE...");
}

void WindowClient::on_pushButtonModifier_clicked()
{
  // TO DO
  ResetALRM();  //réinitialisation du compteur.
  
  //construction du message MODIF1.
  MESSAGE m;
  m.type = 1;
  m.expediteur = getpid();
  m.requete = MODIF1;
  fprintf(stderr, "(CLIENT %d) Envoi d'un message MODIF1", getpid());
  if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0) == -1) //Envoi de la requête
  {
    perror("Erreur de msgsnd");
    exit(0);
  }

  sigset_t mask, oldmask;
  sigfillset(&mask);
  sigprocmask(SIG_SETMASK, &mask, &oldmask);  //Section critique debut.
  // Attente d'une reponse en provenance de Modification
  fprintf(stderr,"(CLIENT %d) Attente reponse MODIF1\n",getpid());
  if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
  {
    perror("Erreur de msgrcv");
    exit(1);
  }
  sigprocmask(SIG_SETMASK, &oldmask, NULL);
  fprintf(stderr, "Fin section critique.\n");

  // Verification si la modification est possible
  if (strcmp(m.data1,"KO") == 0 && strcmp(m.data2,"KO") == 0 && strcmp(m.texte,"KO") == 0)
  {
    QMessageBox::critical(w,"Problème...","Modification déjà en cours...");
    return;
  }

  // Modification des données par utilisateur
  DialogModification dialogue(this,getNom(),m.data1,m.data2,m.texte);
  dialogue.exec();
  char motDePasse[40];
  char gsm[40];
  char email[40];
  strcpy(motDePasse,dialogue.getMotDePasse());
  strcpy(gsm,dialogue.getGsm());
  strcpy(email,dialogue.getEmail());

  // Envoi des données modifiées au serveur
  //Construction du message.
  m.type = 1;
  m.expediteur = getpid();
  m.requete = MODIF2;
  strcpy(m.data1, motDePasse);
  strcpy(m.data2, gsm);
  strcpy(m.texte, email);
  
  if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0) == -1) //Envoi de la requête
  {
    perror("Erreur de msgsnd");
    exit(0);
  }
  
  ResetALRM();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les checkbox ///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_checkBox1_clicked(bool checked)
{
  MESSAGE msg;
  msg.type = 1;
  msg.expediteur = getpid();
  strcpy(msg.data1, getPersonneConnectee(1));
  if(checked)
  {
    if(strlen(msg.data1) > 0)
    {
      ui->checkBox1->setText("Accepté");
      msg.requete = ACCEPT_USER; 
      if(msgsnd(idQ,&msg,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "ACCEPT_USER".
      {
        perror("Erreur de msgsnd");
        exit(0);
      }
    }
    else
      ui->checkBox1->setChecked(false);
  }
  else
  {
    ui->checkBox1->setText("Refusé");
    msg.requete = REFUSE_USER;
    if(msgsnd(idQ,&msg,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "REFUSE_USER".
    {
      perror("Erreur de msgsnd");
      exit(0);//EXACTEMENT LA MEME PUR TOUTES LES CHECKBOX
    }
  }
  ResetALRM();
}

void WindowClient::on_checkBox2_clicked(bool checked)
{

  MESSAGE msg;
  msg.type = 1;
  msg.expediteur = getpid();
  strcpy(msg.data1, getPersonneConnectee(2));
  if(checked)
  {
    if(strlen(msg.data1) > 0)
    {
      ui->checkBox2->setText("Accepté");
      msg.requete = ACCEPT_USER; 
      if(msgsnd(idQ,&msg,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "ACCEPT_USER".
      {
        perror("Erreur de msgsnd");
        exit(0);
      }
    }
    else
      ui->checkBox2->setChecked(false);
  }
  else
  {
    ui->checkBox2->setText("Refusé");
    msg.requete = REFUSE_USER;
    if(msgsnd(idQ,&msg,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "REFUSE_USER".
    {
      perror("Erreur de msgsnd");
      exit(0);//EXACTEMENT LA MEME PUR TOUTES LES CHECKBOX
    }
  }
  ResetALRM();
}

void WindowClient::on_checkBox3_clicked(bool checked)
{
  MESSAGE msg;
  msg.type = 1;
  msg.expediteur = getpid();
  strcpy(msg.data1, getPersonneConnectee(3));
  if(checked)
  {
    if(strlen(msg.data1) > 0)
    {
      ui->checkBox3->setText("Accepté");
      msg.requete = ACCEPT_USER; 
      if(msgsnd(idQ,&msg,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "ACCEPT_USER".
      {
        perror("Erreur de msgsnd");
        exit(0);
      }
    }
    else
      ui->checkBox3->setChecked(false);
  }
  else
  {
    ui->checkBox3->setText("Refusé");
    msg.requete = REFUSE_USER;
    if(msgsnd(idQ,&msg,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "REFUSE_USER".
    {
      perror("Erreur de msgsnd");
      exit(0);//EXACTEMENT LA MEME PUR TOUTES LES CHECKBOX
    }
  }
  ResetALRM();
}

void WindowClient::on_checkBox4_clicked(bool checked)
{
  MESSAGE msg;
  msg.type = 1;
  msg.expediteur = getpid();
  strcpy(msg.data1, getPersonneConnectee(4));
  if(checked)
  {
    if(strlen(msg.data1) > 0)
    {
      ui->checkBox4->setText("Accepté");
      msg.requete = ACCEPT_USER; 
      if(msgsnd(idQ,&msg,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "ACCEPT_USER".
      {
        perror("Erreur de msgsnd");
        exit(0);
      }
    }
    else
      ui->checkBox4->setChecked(false);
  }
  else
  {
    ui->checkBox4->setText("Refusé");
    msg.requete = REFUSE_USER;
    if(msgsnd(idQ,&msg,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "REFUSE_USER".
    {
      perror("Erreur de msgsnd");
      exit(0);//EXACTEMENT LA MEME PUR TOUTES LES CHECKBOX
    }
  }
  ResetALRM();
}

void WindowClient::on_checkBox5_clicked(bool checked)
{
  MESSAGE msg;
  msg.type = 1;
  msg.expediteur = getpid();
  strcpy(msg.data1, getPersonneConnectee(5));
  if(checked)
  {
    if(strlen(msg.data1) > 0)
    {
      ui->checkBox5->setText("Accepté"); 
      msg.requete = ACCEPT_USER; 
      if(msgsnd(idQ,&msg,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "ACCEPT_USER".
      {
        perror("Erreur de msgsnd");
        exit(0);
      }
    }
    else
      ui->checkBox5->setChecked(false);
  }
  else
  {
    ui->checkBox5->setText("Refusé");
    msg.requete = REFUSE_USER;
    if(msgsnd(idQ,&msg,sizeof(MESSAGE)-sizeof(long),IPC_NOWAIT) == -1) //Envoi de la requête "REFUSE_USER".
    {
      perror("Erreur de msgsnd");
      exit(0);//EXACTEMENT LA MEME PUR TOUTES LES CHECKBOX
    }
  }
  ResetALRM();
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions pour améliorer la visibilité ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////


void ResetALRM(void)
{
  alarm(0);
  timeOut=TIME_OUT;
  w->setTimeOut(timeOut);
  alarm(1);
  timeOut--;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ///////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HandlerSIGUSR1(int sig)  //
{
    fprintf(stderr, "(CLIENT %d) Reception d'un signal SIGUSR1 (%d).\n", getpid(), sig);
    MESSAGE m;
    int i;

    // ...msgrcv(idQ,&m,...)
    if(msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1) //Reception du message venant du serveur.
    {
      perror("(CLIENT) Erreur de msgrcv : ");
      exit(1);
    }
    
      switch(m.requete)
      {
        case LOGIN :  //OK
              if (strcmp(m.data1,"OK") == 0)  //Si contenu de la reponse = "OK"
              {
                fprintf(stderr,"(CLIENT %d) Login OK\n",getpid());
                w->loginOK(); //Changement des boutons (voir GUI).
                w->setNomRenseignements(w->getNom());
                QMessageBox::information(w,"Login...","Login Ok ! Bienvenue...");
                timeOut=TIME_OUT;
                w->setTimeOut(timeOut);
                alarm(1);
                timeOut--;
              }
              else 
                QMessageBox::critical(w,"Login...","Erreur de login...");
        break;

        case ADD_USER : //OK
              // TO DO;
              fprintf(stderr, "Reception d'une requete ADD_USER contenant %s.\n", m.data1);
              i=1;
              while((i<6) && (strlen(w->getPersonneConnectee(i)) != 0))
              {
                fprintf(stderr, "Il y'a deja %s connecte en i = %d.\n", w->getPersonneConnectee(i), i);
                i++;
              }
              if(i<6)
                w->setPersonneConnectee(i,m.data1);
              else
                fprintf(stderr,"erreur du (CLIENT) , on dépasse le nb d'utilisateur connectable.\n");
        break;

        case REMOVE_USER :  //OK
              i=1;
              while (i<6 && (strcmp(w->getPersonneConnectee(i),m.data1) != 0))
                i++;
              if(i<6)
                w->setPersonneConnectee(i,"");
              else
                fprintf(stderr, "erreur du (CLIENT) , on dépasse le nb d'utilisateur max\n");
        break;

        case SEND :   //OK
              fprintf(stderr, "Reception d'un message SEND de %s\n", m.data1);
              w->ajouteMessage(m.data1, m.texte);
              
        break;

        case CONSULT :  //OK
              fprintf(stderr, "Reception d'un message CONSULT %s \n", m.data1);
              if(strcmp(m.data1, "OK") == 0)
              {
                w->setGsm(m.data2);
                w->setEmail(m.texte);
              }
              else
              {
                w->setGsm("NON TROUVE");
                w->setEmail("NON TROUVE");
              }
        break;
      }
}
void HandlerSIGUSR2(int sig)  //
{
  fprintf(stderr, "(CLIENT %d) Reception d'un signal SIGUSR2 (%d).\n", getpid(), sig);    
  //Affichage dans l'interface graphique de la pub.
  w->setPublicite(tab->publicite);
  fprintf(stderr, "(CLIENT %d) Fin handler SIGUSR2.\n", getpid());
}


void HandlerSIGALRM(int sig)  //
{
  if(timeOut == 0)
  {
    //Envoi de message LOGOUT.
    fprintf(stderr, "(CLIENT %d) Timeout (%d).\n", getpid(), sig);
    w->logoutOK();
    MESSAGE msg_logout;
    msg_logout.type = 1;    //Type 1 signifie que les message est destiné au serveur.
    msg_logout.expediteur = getpid();
    msg_logout.requete = LOGOUT;  //Message de type "LOGOUT".
    if(msgsnd(idQ,&msg_logout,sizeof(MESSAGE)-sizeof(long), IPC_NOWAIT) == -1) //Envoi de la requête "LOGOUT".
    {
      perror("(TimeOut) : Erreur de msgsnd.");
      exit(0);
    }
    fprintf(stderr,"(TimeOut) : Message envoye.\n");
    timeOut = TIME_OUT;
  }
  else
  {
    w->setTimeOut(timeOut);
    timeOut--;
    alarm(1);
  }
}