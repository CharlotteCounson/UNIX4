#include "windowclient.h"
#include "ui_windowclient.h"
#include <QMessageBox>
#include "dialogmodification.h"
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/shm.h>

extern WindowClient *w;

#include "protocole.h"

int idQ, idShm;
#define TIME_OUT 120
int timeOut = TIME_OUT;

char * pMEM = NULL;

struct sigaction A, B, C;
void handlerSIGUSR1(int sig);
void handlerSIGALRM(int sig);
void handlerSIGUSR2(int sig);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowClient::WindowClient(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowClient)
{
    ui->setupUi(this);
    logoutOK();

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la file de messages\n",getpid());
    if( (idQ = msgget(CLE, 0)) == -1){
      fprintf(stderr, "(CLIENT %d) Erreur de recuperation de la file de messages\n", getpid());
      exit(1);
    }

    // Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la mémoire partagée\n",getpid());
    if( (idShm = shmget(CLE, 0, 0)) == -1){
      fprintf(stderr, "(CLIENT %d) Erreur shmget\n", getpid());
      exit(1);
    }

    // Attachement à la mémoire partagée en lecture seule d'où le SHM_RDONLY
    if( (pMEM = (char*)shmat(idShm, NULL, SHM_RDONLY)) == (char*)-1){
      fprintf(stderr, "(CLIENT %d) Erreur shmat\n", getpid());
      exit(1);
    }

    // Armement des signaux
    // signal pour messages généraux
    A.sa_handler = handlerSIGUSR1;
    sigemptyset(&A.sa_mask);
    A.sa_flags = 0;

    if (sigaction(SIGUSR1,&A,NULL) == -1)
    {
      perror("Erreur de sigaction sur SIGUSR1 CLIENT");
      exit(1);
    }

    // signal pour l'auto-déconnection
    B.sa_handler = handlerSIGALRM;
    sigemptyset(&B.sa_mask);
    A.sa_flags = 0;

    if (sigaction(SIGALRM, &B, NULL) == -1)
    {
      perror("Erreur de sigaction sur SIGALRM CLIENT");
      exit(1);
    }

    // signal pour la publicite
    C.sa_handler = handlerSIGUSR2;
    sigemptyset(&C.sa_mask);
    C.sa_flags = 0;

    if (sigaction(SIGUSR2, &C, NULL) == -1)
    {
      perror("Erreur de sigaction sur SIGUSR2 CLIENT");
      exit(1);
    }



    // Envoi d'une requete de connexion au serveur

    MESSAGE msg;
    memset(&msg, 0, sizeof(MESSAGE));
    msg.type = 1;
    msg.expediteur = getpid();
    msg.requete = CONNECT;

    if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
      fprintf(stderr, "(CLIENT %d) Erreur de msgsnd à la connection\n", getpid());
      exit(1);
    }
    fprintf(stderr, "(CLIENT %d) Connection au serveur avec succes\n", getpid());

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
void WindowClient::on_pushButtonLogin_clicked()
{
  // TO DO
  MESSAGE msg;
  memset(&msg, 0, sizeof(MESSAGE));

  msg.type = 1;
  msg.expediteur = getpid();
  msg.requete = LOGIN;
  strcpy(msg.data1, getNom());
  strcpy(msg.data2, getMotDePasse());

  if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(CLIENT %d) Erreur de msgsnd Login\n", getpid());
    exit(1);
  }

  timeOut = TIME_OUT;
  setTimeOut(timeOut);
  alarm(1);

}

void WindowClient::on_pushButtonLogout_clicked()
{
  // TO DO

  MESSAGE msg;
  memset(&msg, 0, sizeof(MESSAGE));

  msg.type = 1;
  msg.expediteur = getpid();
  msg.requete = LOGOUT;

  if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(CLIENT %d) Erreur msgsnd Logout\n", getpid());
    exit(1);
  }

  logoutOK();

  timeOut = TIME_OUT;
  setTimeOut(timeOut);
  alarm(0);
}

void WindowClient::on_pushButtonQuitter_clicked()
{
  // TO DO
  MESSAGE msg;
  memset(&msg, 0, sizeof(MESSAGE));

  msg.type = 1;
  msg.expediteur = getpid();
  msg.requete = DECONNECT;

  if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(CLIENT %d) Erreur de msgsnd à la déconnection (Quitter)\n", getpid());
    exit(1);
  }
  fprintf(stderr, "(CLIENT %d) Clique sur boutton Quitter\n", getpid());

  exit(0);
}

void WindowClient::on_pushButtonEnvoyer_clicked()
{
  // TO DO
  char txt[200];
  memset(txt, 0, sizeof(txt));
  MESSAGE msg;
  memset(&msg, 0, sizeof(MESSAGE));
  msg.type = 1;
  msg.expediteur = getpid();
  msg.requete = SEND;

  strcpy(msg.texte, getAEnvoyer());

  if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(CLIENT %d) Erreur msgsnd Envoyer\n", getpid());
    exit(1);
  }
  sleep(1);
  fprintf(stderr, "(CLIENT %d) Requete Envoyer --%s--\n", getpid(), msg.texte);

  setAEnvoyer("");

  timeOut = TIME_OUT;
  setTimeOut(timeOut);
  alarm(1);
}

void WindowClient::on_pushButtonConsulter_clicked()
{
  // TO DO
  MESSAGE msg;
  memset(&msg, 0, sizeof(MESSAGE));
  char userConsult[20];
  memset(userConsult, 0, 20);
  strcpy(userConsult, getNomRenseignements());
  fprintf(stderr, "(CLIENT %d) Consultation de %s\n", getpid(), userConsult);
  msg.type = 1;
  msg.expediteur = getpid();
  msg.requete = CONSULT;
  strcpy(msg.data1, userConsult);

  if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(CLIENT %d) Erreur msgsnd Consulter\n", getpid());
    exit(1);
  }
  sleep(1);

  timeOut = TIME_OUT;
  setTimeOut(timeOut);
  alarm(1);
}

void WindowClient::on_pushButtonModifier_clicked()
{
  // TO DO
  // Envoi d'une requete MODIF1 au serveur
  MESSAGE m;
  // ...
  m.type = 1;
  m.expediteur = getpid();
  m.requete = MODIF1;

  if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(CLIENT %d) Erreur de msgsnd MODIF1\n", getpid());
    exit(1);
  }

  // Attente d'une reponse en provenance de Modification
  fprintf(stderr,"(CLIENT %d) Attente reponse MODIF1\n",getpid());
  // ...
  if(msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1){
    fprintf(stderr, "(CLIENT %d) Erreur de msgrcv MODIF1\n", getpid());
    exit(1);
  }

  // Verification si la modification est possible
  // pourrait pas mettre des || ? pour éviter de tous éviter de tous évalué
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
  // ...
  m.type = 1;
  m.expediteur = getpid();
  m.requete = MODIF2;
  strcpy(m.data1, motDePasse);
  strcpy(m.data2, gsm);
  strcpy(m.texte, email);

  if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(CLIENT %d) Erreur de msgsnd MODIF2\n", getpid());
    exit(1);
  }


  timeOut = TIME_OUT;
  setTimeOut(timeOut);
  alarm(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les checkbox ///////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_checkBox1_clicked(bool checked)
{
  short userDefini = 0;
  MESSAGE acceptRefu;
  memset(&acceptRefu, 0, sizeof(MESSAGE));
  acceptRefu.type = 1;
  acceptRefu.expediteur = getpid();
  strcpy(acceptRefu.data1, "");
  if (checked)
  {
      ui->checkBox1->setText("Accepté");
      // TO DO
      if(strcmp(getPersonneConnectee(1), "") != 0) userDefini = 1;
      acceptRefu.requete = ACCEPT_USER;
  }
  else
  {
      ui->checkBox1->setText("Refusé");
      // TO DO
      if(strcmp(getPersonneConnectee(1), "") != 0) userDefini = 1;
      acceptRefu.requete = REFUSE_USER;
  }

  if(userDefini == 1){
    strcpy(acceptRefu.data1, getPersonneConnectee(1));
    if(msgsnd(idQ, &acceptRefu, sizeof(MESSAGE) - sizeof(long), 0) == -1){
      fprintf(stderr, "(CLIENT %d) Erreur msgsnd checkBox1 %s\n", getpid(), acceptRefu.data1);
      exit(1);
    }
    sleep(1);
  }

  timeOut = TIME_OUT;
  setTimeOut(timeOut);
  alarm(1);

}

void WindowClient::on_checkBox2_clicked(bool checked)
{
  short userDefini = 0;
  MESSAGE acceptRefu;
  memset(&acceptRefu, 0, sizeof(MESSAGE));
  acceptRefu.type = 1;
  acceptRefu.expediteur = getpid();
  strcpy(acceptRefu.data1, "");
  if (checked)
  {
      ui->checkBox2->setText("Accepté");
      // TO DO
      if(strcmp(getPersonneConnectee(2), "") != 0) userDefini = 1;
      acceptRefu.requete = ACCEPT_USER;
  }
  else
  {
      ui->checkBox2->setText("Refusé");
      // TO DO
      if(strcmp(getPersonneConnectee(2), "") != 0) userDefini = 1;
      acceptRefu.requete = REFUSE_USER;
  }

  if(userDefini == 1){
    strcpy(acceptRefu.data1, getPersonneConnectee(2));
    if(msgsnd(idQ, &acceptRefu, sizeof(MESSAGE) - sizeof(long), 0) == -1){
      fprintf(stderr, "(CLIENT %d) Erreur msgsnd checkBox2 %s\n", getpid(), acceptRefu.data1);
      exit(1);
    }
    sleep(1);
  }


  timeOut = TIME_OUT;
  setTimeOut(timeOut);
  alarm(1);
}

void WindowClient::on_checkBox3_clicked(bool checked)
{
  short userDefini = 0;
  MESSAGE acceptRefu;
  memset(&acceptRefu, 0, sizeof(MESSAGE));
  acceptRefu.type = 1;
  acceptRefu.expediteur = getpid();
  strcpy(acceptRefu.data1, "");
  if (checked)
  {
      ui->checkBox3->setText("Accepté");
      // TO DO
      if(strcmp(getPersonneConnectee(3), "") != 0) userDefini = 1;
      acceptRefu.requete = ACCEPT_USER;
  }
  else
  {
      ui->checkBox1->setText("Refusé");
      // TO DO
      if(strcmp(getPersonneConnectee(3), "") != 0) userDefini = 1;
      acceptRefu.requete = REFUSE_USER;
  }

  if(userDefini == 1){
    strcpy(acceptRefu.data1, getPersonneConnectee(3));
    if(msgsnd(idQ, &acceptRefu, sizeof(MESSAGE) - sizeof(long), 0) == -1){
      fprintf(stderr, "(CLIENT %d) Erreur msgsnd checkBox3 %s\n", getpid(), acceptRefu.data1);
      exit(1);
    }
    sleep(1);
  }

  timeOut = TIME_OUT;
  setTimeOut(timeOut);
  alarm(1);
}

void WindowClient::on_checkBox4_clicked(bool checked)
{
  short userDefini = 0;
  MESSAGE acceptRefu;
  memset(&acceptRefu, 0, sizeof(MESSAGE));
  acceptRefu.type = 1;
  acceptRefu.expediteur = getpid();
  strcpy(acceptRefu.data1, "");
  if (checked)
  {
      ui->checkBox4->setText("Accepté");
      // TO DO
      if(strcmp(getPersonneConnectee(4), "") != 0) userDefini = 1;
      acceptRefu.requete = ACCEPT_USER;
  }
  else
  {
      ui->checkBox4->setText("Refusé");
      // TO DO
      if(strcmp(getPersonneConnectee(4), "") != 0) userDefini = 1;
      acceptRefu.requete = REFUSE_USER;
  }

  if(userDefini == 1){
    strcpy(acceptRefu.data1, getPersonneConnectee(4));
    if(msgsnd(idQ, &acceptRefu, sizeof(MESSAGE) - sizeof(long), 0) == -1){
      fprintf(stderr, "(CLIENT %d) Erreur msgsnd checkBox4 %s\n", getpid(), acceptRefu.data1);
      exit(1);
    }
    sleep(1);
  }

  timeOut = TIME_OUT;
  setTimeOut(timeOut);
  alarm(1);
}

void WindowClient::on_checkBox5_clicked(bool checked)
{
  short userDefini = 0;
  MESSAGE acceptRefu;
  memset(&acceptRefu, 0, sizeof(MESSAGE));
  acceptRefu.type = 1;
  acceptRefu.expediteur = getpid();
  strcpy(acceptRefu.data1, "");
  if (checked)
  {
      ui->checkBox5->setText("Accepté");
      // TO DO
      if(strcmp(getPersonneConnectee(5), "") != 0) userDefini = 1;
      acceptRefu.requete = ACCEPT_USER;
  }
  else
  {
      ui->checkBox1->setText("Refusé");
      // TO DO
      if(strcmp(getPersonneConnectee(5), "") != 0) userDefini = 1;
      acceptRefu.requete = REFUSE_USER;
  }

  if(userDefini == 1){
    strcpy(acceptRefu.data1, getPersonneConnectee(5));
    if(msgsnd(idQ, &acceptRefu, sizeof(MESSAGE) - sizeof(long), 0) == -1){
      fprintf(stderr, "(CLIENT %d) Erreur msgsnd checkBox5 %s\n", getpid(), acceptRefu.data1);
      exit(1);
    }
    sleep(1);
  }

  timeOut = TIME_OUT;
  setTimeOut(timeOut);
  alarm(1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIGUSR1(int sig)
{
    MESSAGE m;
    memset(&m, 0, sizeof(MESSAGE));
    int i = 0;
    short placeLibre = 0;

    // ...msgrcv(idQ,&m,...)
    if(msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1){
      fprintf(stderr, "(CLIENT %d) Erreur msgrcv handlerSIGUSR1", getpid());
      exit(1);
    }

    
      switch(m.requete)
      {
        case LOGIN :
                    if (strcmp(m.data1,"OK") == 0)
                    {
                      fprintf(stderr,"(CLIENT %d) Login OK\n",getpid());
                      w->loginOK();
                      QMessageBox::information(w,"Login...","Login Ok ! Bienvenue...");
                      // ...
                    }
                    else QMessageBox::critical(w,"Login...","Erreur de login...");
                    break;

        case ADD_USER :
                    // TO DO;
                    fprintf(stderr, "(CLIENT %d) Requete ADD_USER recue %s\n", getpid(), m.data1);
                    placeLibre = 0;

                    i = 1;
                    while(i < NBCONNECTMAX && placeLibre == 0){
                      if(strcmp(w->getPersonneConnectee(i), "") == 0) placeLibre = 1;
                      if(strcmp(w->getPersonneConnectee(i), m.data1) == 0) placeLibre = -1; // utilisateur déjà présent
                      i++;
                    }
                    if(placeLibre == 1){
                      w->setPersonneConnectee((i-1), m.data1);
                      w->ajouteMessage(m.data1, " est connecté!");
                    }

                    break;

        case REMOVE_USER :
                    // TO DO
                    fprintf(stderr, "(CLIENT %d) Requete REMOVE_USER recue %s\n", getpid(), m.data1);
                    placeLibre = 0;

                    i = 1;
                    while(i < NBCONNECTMAX && placeLibre == 0){
                      if(strcmp(w->getPersonneConnectee(i), m.data1) == 0) placeLibre = 1;
                      else i++;
                    }
                    if(placeLibre == 1){
                      w->setPersonneConnectee(i, "");
                      w->setCheckbox(i, false);
                      if(strlen(m.data1) > 0) w->ajouteMessage(m.data1, " est déconnecté!");
                    }

                    break;

        case SEND :
                    // TO DO
                    fprintf(stderr, "(CLIENT %d) Requete SEND recue %s\n", getpid(), m.data1);
                    w->ajouteMessage(m.data1, m.texte);
                    break;

        case CONSULT :
                    // TO
                    fprintf(stderr, "(CLIENT %d) Requete CONSULT recue %s\n", getpid(), m.data1);
                    if(strcmp(m.data1, "OK") == 0){
                      w->setGsm(m.data2);
                      w->setEmail(m.texte);

                      fprintf(stderr, "(CLIENT %d) CONSULT %s %s\n", getpid(), m.data2, m.texte);
                    }
                    else {
                      w->setGsm("");
                      w->setEmail("NON TROUVE");
                    }
                    break;

      }


}

void handlerSIGALRM(int sig)
{
  timeOut--;
  if(timeOut > 0){
    alarm(1);
  }
  else{
    fprintf(stderr, "(CLIENT %d) Deconnection automatique pour inactivite\n", getpid());
    w->on_pushButtonLogout_clicked();
    timeOut = TIME_OUT;
    alarm(0);
  }
  w->setTimeOut(timeOut);
}


void handlerSIGUSR2(int sig)
{
  if(pMEM != NULL) {
    char txt[200];
    memset(txt, 0, 200);
    strcpy(txt, pMEM);
    w->setPublicite(txt);
  }
}
