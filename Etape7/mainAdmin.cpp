#include "windowadmin.h"
#include <QApplication>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

int idQ;

int main(int argc, char *argv[])
{
    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(ADMINISTRATEUR %d) Recuperation de l'id de la file de messages\n",getpid());
    if((idQ = msgget(CLE,0))==-1) //Tentative de récupération de l'id de la file de message.
    {
        perror("Erreur de msgget");
        exit(0);
    }
    printf("idQ = %d\n",idQ);


    // Envoi d'une requete de connexion au serveur
    MESSAGE m, reponse;
    m.type = 1;
    m.expediteur = getpid();
    m.requete = LOGIN_ADMIN;
    if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long), 0) == -1) //Envoi du message au serveur.
    {
        perror("Erreur de msgsnd");
        exit(1);
    }

    // Attente de la réponse
    fprintf(stderr,"(ADMINISTRATEUR %d) Attente reponse\n",getpid());
    if(msgrcv(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1) //Reception du message venant du serveur.
    {
      perror("(CLIENT) Erreur de msgrcv : ");
      exit(1);
    }
    if(strcmp(reponse.data1, "OK") != 0)
    {
      exit(1);
    }

    QApplication a(argc, argv);
    WindowAdmin w;
    w.show();
    return a.exec();
}
