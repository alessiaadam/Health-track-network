#ifndef CLIENT_H
#define CLIENT_H

int socket_sd();
int conectare(const char *adresa, int port);
int scrie_comanda(const char *comanda);
int citire_raspuns(char *raspuns, int size);
void inchidere();

#endif