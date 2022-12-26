#include "windowadmin.h"
#include "ui_windowadmin.h"
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

extern int idQ;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
WindowAdmin::WindowAdmin(QWidget *parent):QMainWindow(parent),ui(new Ui::WindowAdmin)
{
    ui->setupUi(this);
}

WindowAdmin::~WindowAdmin()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowAdmin::getNom()
{
  strcpy(nom,ui->lineEditNom->text().toStdString().c_str());
  return nom;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowAdmin::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setTexte(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditTexte->clear();
    return;
  }
  ui->lineEditTexte->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowAdmin::getTexte()
{
  strcpy(texte,ui->lineEditTexte->text().toStdString().c_str());
  return texte;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::setNbSecondes(int n)
{
  char Text[10];
  sprintf(Text,"%d",n);
  ui->lineEditNbSecondes->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowAdmin::getNbSecondes()
{
  char tmp[10];
  strcpy(tmp,ui->lineEditNbSecondes->text().toStdString().c_str());
  return atoi(tmp);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowAdmin::on_pushButtonAjouterUtilisateur_clicked()
{
  // TO DO
  fprintf(stderr, "(ADMINISTRATEUR %d) Clique sur boutton Ajouter\n", getpid());
  MESSAGE msg;

  if(strlen(getNom()) <= 0 || strlen(getMotDePasse()) <= 0){
    fprintf(stderr, "(ADMINISTRATEUR %d) Nom utlisateur ou mot de passe non spécifié\n", getpid());
    return;
  }

  msg.type = 1;
  msg.expediteur = getpid();
  msg.requete = NEW_USER;
  strcpy(msg.data1, getNom());
  strcpy(msg.data2, getMotDePasse());

  if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(ADMINISTRATEUR %d) Erreur de msgsnd BTN Ajouter utlisateur\n", getpid());
    exit(1);
  }

}

void WindowAdmin::on_pushButtonSupprimerUtilisateur_clicked()
{
  // TO DO
  fprintf(stderr, "(ADMINISTRATEUR %d) Clique sur boutton Supprimer\n", getpid());
  MESSAGE msg;

  if(strlen(getNom()) <= 0){
    fprintf(stderr, "(ADMINISTRATEUR %d) Nom utlisateur non spécifié\n", getpid());
    return;
  }

  msg.type = 1;
  msg.expediteur = getpid();
  msg.requete = DELETE_USER;
  strcpy(msg.data1, getNom());

  if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(ADMINISTRATEUR %d) Erreur de msgsnd BTN Supprimer utlisateur\n", getpid());
    exit(1);
  }


}

void WindowAdmin::on_pushButtonAjouterPublicite_clicked()
{
  // TO DO
  fprintf(stderr, "(ADMINISTRATEUR %d) Clique sur le boutton Ajout Pub\n", getpid());
  if(strlen(getTexte()) <= 0 || getNbSecondes() <= 0){
    fprintf(stderr, "(ADMINISTRATEUR %d) Encoder une publicite correcte\n", getpid());
    return;
  }

  MESSAGE msg;

  msg.type = 1;
  msg.expediteur = getpid();
  msg.requete = NEW_PUB;
  sprintf(msg.data1, "%d", getNbSecondes());
  strcpy(msg.texte, getTexte());

  if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(ADMINISTRATEUR %d) Erreur de msgsnd BTN Ajout publicite\n", getpid());
    exit(1);
  }

}

void WindowAdmin::on_pushButtonQuitter_clicked()
{
  // TO DO
  fprintf(stderr, "(ADMINISTRATEUR %d) Clique boutton Quitter\n", getpid());



  MESSAGE msg;
  msg.type = 1;
  msg.expediteur = getpid();
  msg.requete = LOGOUT_ADMIN;

  if(msgsnd(idQ, &msg, sizeof(MESSAGE) - sizeof(long), 0) == -1){
    fprintf(stderr, "(ADMINISTRATEUR %d) Erreur msgsnd Quitter\n", getpid());
    exit(1);
  }

  exit(0);
}
