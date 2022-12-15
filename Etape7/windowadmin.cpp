#include "windowadmin.h"
#include "ui_windowadmin.h"
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

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
void WindowAdmin::on_pushButtonAjouterUtilisateur_clicked() //OK
{
  // TO DO
  char tmpNom[50], tmpMdp[50];

  strcpy(tmpNom, getNom());
  strcpy(tmpMdp, getMotDePasse());
  if((strlen(tmpNom) != 0) && (strlen(tmpMdp) != 0))  //Si les champ nom et mdp ne sont pas vides.
  {
    MESSAGE m;
    m.type = 1;
    m.expediteur = getpid();
    m.requete = NEW_USER;
    strcpy(m.data1, tmpNom);
    strcpy(m.data2, tmpMdp);
    if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0) == -1) //Envoi du message au serveur.
    {
        perror("Erreur de msgsnd");
        exit(1);
    }
    setNom("");
    setMotDePasse("");
  }
}

void WindowAdmin::on_pushButtonSupprimerUtilisateur_clicked() //OK
{
  // TO DO
  char tmpNom[50];
  strcpy(tmpNom, getNom());
  if(strlen(tmpNom) != 0) //Si le champ nom n'est pas vide.
  {
    MESSAGE m;
    m.type = 1;
    m.expediteur = getpid();
    m.requete = DELETE_USER;
    strcpy(m.data1, tmpNom);
    if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0) == -1) //Envoi du message au serveur.
    {
        perror("Erreur de msgsnd");
        exit(1);
    }
    setNom("");
    setMotDePasse("");
  }
}

void WindowAdmin::on_pushButtonAjouterPublicite_clicked() //OK
{
  // TO DO
  char tmpTxt[100];
  int tmpSecondes;
  strcpy(tmpTxt, getTexte());
  tmpSecondes = getNbSecondes();
  if((strlen(tmpTxt) !=0) && (tmpSecondes > 0))
  {
    fprintf(stderr, "Nouvelle pub valide !\n");
    MESSAGE m;
    m.type = 1;
    m.expediteur = getpid();
    m.requete = NEW_PUB;
    strcpy(m.texte, tmpTxt);
    sprintf(m.data1,"%d", tmpSecondes);
    if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0) == -1) //Envoi du message au serveur.
    {
        perror("Erreur de msgsnd");
        exit(1);
    }
    setTexte("");
    ui->lineEditNbSecondes->clear();
  } 
  fprintf(stderr, "Fin bouton AjouterPub.\n");
}

void WindowAdmin::on_pushButtonQuitter_clicked()  //OK
{
  // TO DO
    MESSAGE m;
    m.type = 1;
    m.expediteur = getpid();
    m.requete = LOGOUT_ADMIN;
    if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0) == -1) //Envoi du message au serveur.
    {
        perror("Erreur de msgsnd");
        exit(0);
    }
  exit(0);
}
