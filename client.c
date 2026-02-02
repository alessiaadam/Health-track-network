#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

extern int errno;
int port;
int sd=-1;
int socket_sd()
{
    return sd;
}

int conectare (const char *adresa, int port_param)
{
    struct sockaddr_in server;
    port=port_param;
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
	perror ("Eroare la socket().\n");
	return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(adresa);
    server.sin_port = htons (port);

    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
	perror ("[client]Eroare la connect().\n");
	close(sd);
	sd=-1;
	return errno;
    }
    printf("[client]Conectat la %s:%d\n",adresa,port);
    return 0;
}

int scrie_comanda(const char *comanda)
{
    char comanda_enter[250];
    strcpy(comanda_enter,comanda);
    strcat(comanda_enter,"\n");
    int lung_comanda=strlen(comanda_enter);
    if(sd>=0)
    {
	if(write(sd,comanda_enter,lung_comanda)<=0)
	{
	    perror("[client]Eroare la trimiterea comenzii catre server.\n");
	    return errno;
	}
    }
    return 0;
}

int citire_raspuns(char *raspuns,int lung_raspuns)
{
    if(sd>=0)
    {
	memset(raspuns,0,lung_raspuns);
	int bytes=read(sd,raspuns,lung_raspuns-1);
	if(bytes<=0)
	{
	    perror("[client]Eroare la citirea raspunsului de la server.\n");
	    return errno;
	}
	printf("[raspuns server] %s\n",raspuns);
	return bytes;
    }
    return -1;
}
void inchidere()
{
    if(sd>=0)
    {
	close (sd);
	printf("Client inchis.\n");
	sd=-1;
    }
}
