#include "FichierClient.h"
#include <string.h>
#include <cstdio>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>



int estPresent(const char* nom)
{
  // TO DO
  int fd,x,i=0;
  CLIENT c;
  if ((fd=open(FICHIER_CLIENTS,O_RDONLY))==-1)
  {
   printf("Probleme d'ouverture\n");
    return -1;
  }
    do
    {
        if(read(fd,&c,sizeof(CLIENT))==0)
        {
          printf("Fin de fichier\n");
          return 0;
        }
        else
        {
          x=strcmp(c.nom,nom);
          i++;
        }

    }while(x!=0);
    close(fd);

  return i;
}

////////////////////////////////////////////////////////////////////////////////////
int hash(const char* motDePasse)
{
  // TO DO
  int i,x,total=0;
  for (i=0;i<strlen(motDePasse);i++)
  {
    x=(i+1)*motDePasse[i];
    total+=x;
  }
  total=total%97;
  return (total);


  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
void ajouteClient(const char* nom, const char* motDePasse)
{
  // TO DO
  CLIENT c;
  int fd, rc;

  if ((fd=open(FICHIER_CLIENTS,O_WRONLY|O_APPEND))==-1)
  {
    if((fd=open(FICHIER_CLIENTS,O_WRONLY|O_CREAT,0644))==-1)
    {
      printf("Echec de creation !");
    }   
  }

    strcpy(c.nom,nom);
    c.hash = hash(motDePasse);
    if(write(fd,&c,sizeof(CLIENT))!=sizeof(CLIENT))
    {
      
    }

    close(fd);
}

////////////////////////////////////////////////////////////////////////////////////
int verifieMotDePasse(int pos, const char* motDePasse)
{
  // TO DO
    CLIENT c;
    int fd,h;
    if((fd=open(FICHIER_CLIENTS,O_RDONLY))==-1)
    {
      printf("Echec ouverture !");
      return -1;
    }
    printf("DébutV\n");
    lseek(fd,((pos-1)*sizeof(CLIENT)),SEEK_SET);

    h=hash(motDePasse);

    if(read(fd,&c,sizeof(CLIENT))==0)
    {
      return 0;
    }
    printf("%d\n",c.hash);
    printf("%d\n",h);
    if(h==c.hash)
    {
      printf("youhou c'est les meme");
      return 1;
    }
    printf("Fermeture\n");
    close(fd);
    printf("incorrect\n");
  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
int listeClients(CLIENT *vecteur) // le vecteur doit etre suffisamment grand
{
  // TO DO
  int nbclient=0, fd;
  CLIENT c;
  if ((fd=open(FICHIER_CLIENTS,O_RDONLY))==-1)
  {
   printf("Probleme d'ouverture\n");
    return -1;
  }
  while(read(fd,&vecteur[nbclient],sizeof(CLIENT))!=0)
  {
    nbclient++;
  }
  //vecteur va prendre la taille de nbclient *sizeof client
    close(fd);

  return nbclient;
}
