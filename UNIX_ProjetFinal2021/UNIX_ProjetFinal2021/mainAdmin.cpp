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
    if( (idQ = msgget(CLE, 0)) == -1){
        fprintf(stderr, "(ADMINISTRATEUR %d) Erreur de msgget\n", getpid());
        exit(1);
    }

    // Envoi d'une requete de connexion au serveur
    MESSAGE m;
    m.type = 1;
    m.expediteur = getpid();
    m.requete = LOGIN_ADMIN;

    if(msgsnd(idQ, &m, sizeof(MESSAGE) - sizeof(long), 0) == -1){
        fprintf(stderr, "(ADMINISTRATEUR %d) Erreur de msgsnd LOGIN_ADMIN\n", getpid());
        exit(1);
    }

    // Attente de la réponse
    fprintf(stderr,"(ADMINISTRATEUR %d) Attente reponse\n",getpid());

    if(msgrcv(idQ, &m, sizeof(MESSAGE) - sizeof(long), getpid(), 0) == -1){
        fprintf(stderr, "(ADMINISTRATEUR %d) Erreur de msgrcv LOGIN_ADMIN\n", getpid());
        exit(1);
    }

    if(strcmp(m.data1, "KO") == 0){
        fprintf(stderr, "(ADMINISTRATEUR %d) Un processus admin est deja lance\n", getpid());
        exit(0);
    }


    QApplication a(argc, argv);
    WindowAdmin w;
    w.show();
    return a.exec();
}
