#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<errno.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<signal.h>
#include<pthread.h>
#include<time.h>

#define PORT 2908
extern int errno;

pthread_mutex_t  mutex;
typedef struct thData{
    int idThread;
    int cl;
    char nume[256];
    int rol;
	int id;
} thData;

int doctori_conectati[100];
int nr_doct=0;
pthread_mutex_t lock_doctor=PTHREAD_MUTEX_INITIALIZER;
void *raspunde(void *arg);
void *treat(void * arg);
void alerta_doctor(char* mesaj);
void fisier(const char* nume, int id, const char* parametru, const char* valoare, const char* diagnostic);
int autentificare(char* rol, char *nume, int id, char *parola);
int main()
{
    struct sockaddr_in server;
    struct sockaddr_in from;
    int nr,sd,pid;
    int i=0;
	signal(SIGPIPE,SIG_IGN);
	if(pthread_mutex_init(&mutex,NULL)!=0)
	{
		printf("\n[server] Eroare la initializarea mutex\n");
		return errno;
	}
    if((sd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
		perror("[server]Eroare la socket().\n");
		return errno;
    }
    
    int on=1;
    setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&on, sizeof(on));
    
    bzero(&server,sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family=AF_INET;
    server.sin_addr.s_addr=INADDR_ANY;
    server.sin_port=htons(PORT);

    if(bind(sd,(struct sockaddr *)&server, sizeof(struct sockaddr))==-1)
    {
	perror("[server] Eroare la bind().\n");
	return errno;
    }

    if(listen(sd,5)==-1)
    {
	perror("[server]Eroare la listen().\n");
	return errno;
    }

    while(1)
    {
		int client;
		thData * td;
		int length=sizeof(from);
		pthread_t th;
	
		printf("[server] Asteptam la portul %d...\n",PORT);
		fflush(stdout);

		if((client=accept(sd,(struct sockaddr *) &from, &length))<0)
		{
	    	perror("[server]Eroare la accept().\n");
	    	continue;
		}

		td=(struct thData*)malloc(sizeof(struct thData));
		td->idThread=i++;
		td->cl=client;

		td->rol=0;
		td->id=0;
		memset(td->nume,0,sizeof(td->nume));
		pthread_create(&th,NULL, &treat,td);
    }
	pthread_mutex_destroy(&mutex);
    close(sd);
    return 0;
}

void *treat(void * arg)
{
    struct thData *tdL=(struct thData*)arg;
	raspunde(tdL);
	if (tdL->rol == 1) {
        pthread_mutex_lock(&lock_doctor);
        for(int i = 0; i < nr_doct; i++) 
		{
            if(doctori_conectati[i] == tdL->cl) 
			{
                for(int j = i; j < nr_doct - 1; j++) 
				{
                    doctori_conectati[j] = doctori_conectati[j + 1];
                }
                nr_doct--;
                printf("[Server] Doctorul %s a fost deconectat.\n", tdL->nume);
                break; 
            }
        }
        pthread_mutex_unlock(&lock_doctor);
    }
    close(tdL->cl);
    free(arg);
    return (NULL);
}
int autentificare(char* rol, char *nume, int id, char *parola)
{
	FILE *f=fopen("parole_login.txt","r");
	if(!f)
	{
		perror("[Server] Eroare la deschiderea fisierului de parole");
		return 0;
	}
	char rol_user[50], nume_user[256],parola_user[100];
	int id_user;
	while(fscanf(f,"%s %s %d %s", rol_user,nume_user,&id_user,parola_user)==4)
	{
		if(strcmp(rol_user,rol)==0 && strcmp(nume_user,nume)==0 && strcmp(parola_user,parola)==0 && id_user==id)
		{
			fclose(f);
			return 1;
		}
	}
	fclose(f);
	return 0;
}

void alerta_doctor(char *mesaj)
{
	pthread_mutex_lock(&lock_doctor);
	for(int i=0;i<nr_doct;i++)
	{
		if(write(doctori_conectati[i],mesaj,strlen(mesaj))<0)
		{
			close(doctori_conectati[i]);
			for(int j=i;j<nr_doct-1;j++)
			{
				doctori_conectati[j]=doctori_conectati[j+1];
			}
			nr_doct--;
			i--;
		}
		else
		{
			write(doctori_conectati[i], "\n", 1);
		}
	}
	pthread_mutex_unlock(&lock_doctor);
}

void fisier(const char* nume, int id, const char* parametru, const char* valoare, const char* diagnostic)
{
	pthread_mutex_lock(&mutex);
	FILE *f= fopen("fisa_medicala.txt", "a");
	if (f!=NULL)
	{
		time_t t=time(NULL);
		struct tm tm = *localtime(&t);
		fprintf(f,"%d-%02d-%02d %02d:%02d %s %d %s %s %s\n", tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour, tm.tm_min,nume,id,parametru,valoare,diagnostic);
		fclose(f);
	}
	else
	{
		perror("[server] Eroare la deschiderea fisierului");
	}
	pthread_mutex_unlock(&mutex);
}
void *raspunde(void * arg)
{
    char comanda[1024],raspuns[2048];
    struct thData *tdL=(struct thData*)arg;
    while(1)
	{
		memset(comanda,0,sizeof(comanda));
		memset(raspuns,0,sizeof(raspuns));
		int bytes=read(tdL->cl,comanda,sizeof(comanda)-1);
		if(bytes<=0)
		{
	    	printf("[Thread %d]\n", tdL->idThread);
	    	perror("Eroare la read() de la client.\n");
	    	break;
		}
		comanda[bytes]='\0';
    	if(strlen(comanda)>0 && comanda[strlen(comanda)-1]=='\n')
    	{
        	comanda[strlen(comanda)-1]='\0';
   		}
		printf("[Thread %d]Mesajul a fost receptionat...%s\n",tdL->idThread,comanda);
		if(strncmp(comanda,"Login",5)==0)
		{
	    	char rol_pozitie[50],nume_user[256], id_unic[100],parola[100];
	    	if(sscanf(comanda,"%*s %49s %255s %99s %99s",rol_pozitie, nume_user,id_unic, parola)==4)
	    	{
				int id1=atoi(id_unic);
				if(autentificare(rol_pozitie, nume_user,id1,parola))
				{
					strncpy(tdL->nume,nume_user, sizeof(tdL->nume)-1);
					tdL->id=id1;
					if(strcmp(rol_pozitie,"Doctor")==0)
					{
		    			tdL->rol=1;
		    			snprintf(raspuns,sizeof(raspuns),"Doctorul %s a fost logat!", nume_user);
						pthread_mutex_lock(&lock_doctor);
						if(nr_doct<100)
						{
							doctori_conectati[nr_doct++]=tdL->cl;
						}
						else
						{
							perror("Prea multi doctori conectati!");
						}
						pthread_mutex_unlock(&lock_doctor);
					}
					else if(strcmp(rol_pozitie,"Pacient")==0)
					{
		    			tdL->rol=2;
		    			snprintf(raspuns,sizeof(raspuns),"Pacientul %s a fost logat!", nume_user);
					}
					else
					{
		    			tdL->rol=0;
		    			strcpy(raspuns,"Nu a fost introdus rolul utilizatorului ca fiind Doctor sau Pacient!");
					}
	   			 }
				 else
				 {
					strcpy(raspuns,"Autentficarea nu a reusit, date incorecte!");
				 }
			}
	   		else
	    	{
				strcpy(raspuns,"Format corect: Login <Doctor|Pacient> <nume> <id> <parola>");
	   		}
		}
		else if(strncmp(comanda,"Istoric",7)==0)
		{
	    	if(tdL->rol==1)
	    	{
				char pacient[200];
				char comanda_select[100]="";
				char data_valoare[100]="";
				char id_pacient[50];
				int nr_argum= sscanf(comanda+7,"%199s %49s %99s %99s",pacient, id_pacient, comanda_select, data_valoare);
				if(nr_argum>=2)
				{
					pthread_mutex_lock(&mutex);
					FILE *f= fopen("fisa_medicala.txt","r"); 
					strcpy(raspuns, "Istoric:\n");
					if(f!=NULL)
					{
						char linie[512];
						int gasit=0;
						while(fgets(linie,sizeof(linie),f))
						{
							if(strstr(linie,pacient)!=NULL && strstr(linie,id_pacient)!=NULL)
							{
								int afis=1;
								if(nr_argum == 4 && strcmp(comanda_select, "Data") == 0)
                       	 		{
									if(strncmp(linie, data_valoare, 10) != 0) 
									{
                                		afis= 0;
                            		}
								}
								if(nr_argum==3 && strstr(linie,comanda_select)==NULL)
								{
									afis=0;
								}
								if(afis)
								{
									if(strlen(raspuns)+strlen(linie)<sizeof(raspuns)-100)
									{
										strcat(raspuns,linie);
										gasit=1;
									}
									else
									{
										strcat(raspuns,"Raspuns prea lung");
										break;
									}
								}
							}	
						}
						if(!gasit)
						{
							strcat(raspuns,"Nu exista un istoric pt acest pacient.");
						}
						fclose(f);
					}
					else
					{
						strcpy(raspuns,"Baza de date goala");
					}
					pthread_mutex_unlock(&mutex);
				}
				else
				{
					strcpy(raspuns, "Precizati numele si id-ul pacientului");
				} 
	   	 	}
	    	else
	    	{
				strcpy(raspuns,"Nu se poate accesa istoricul medical. Doar doctorii il pot accesa.");
	    	}
		}
		else if(strncmp(comanda,"Update",6)==0)
		{	
			char alerta[512]="";
			char diagnostic[100]="";
	    	if(tdL->rol==2)
	    	{
				char parametru[50], valoare[50];
				if (sscanf(comanda+7, "%49s %49s", parametru, valoare)==2)
				{
					if(strcmp(parametru,"Tensiune")==0)
					{
						int param1, param2;
						if(sscanf(valoare, "%d/%d", &param1, &param2)==2)
						{
							if(param1>140 || param2>90)
							{
								strcpy(diagnostic, "Hipertensiune");
								sprintf(raspuns,"Hipertensiune-> %d/%d", param1,param2);
								sprintf(alerta,"ALERTA! Hipertensiune-> %s %d : %d/%d",tdL->nume, tdL->id, param1,param2);
								alerta_doctor(alerta);
							}
							else if(param1<90 || param2<60)
							{
								strcpy(diagnostic, "Hipotensiune");
								sprintf(raspuns,"Hipotensiune> %d/%d", param1,param2);
								sprintf(alerta,"ALERTA! Hipotensiune> %s %d : %d/%d",tdL->nume, tdL->id, param1,param2);
								alerta_doctor(alerta);
							}
							else 
							{
								strcpy(diagnostic, "Normal");
								snprintf(raspuns, sizeof(raspuns),"Tensiunea este normala -> %d/%d", param1,param2);
							}
							fisier(tdL->nume,tdL->id, parametru, valoare,diagnostic);
						}
						else 
						{
							strcpy(raspuns, "Format gresit pt Tensiune");
						}
					}
					else if (strcmp(parametru, "Saturatie")==0)
					{
						int param_ox=atoi(valoare);
						if(param_ox<95)
						{
							strcpy(diagnostic, "Hipoxemie");
							sprintf(raspuns, "Valoare critica. Hipoxemie severa ->%d!", param_ox);
							sprintf(alerta, "ALERTA! Valoare critica. Hipoxemie severa ->%s %d %d!", tdL->nume, tdL->id,param_ox);
							alerta_doctor(alerta);
						}
						else if(param_ox>100)
						{
							strcpy(diagnostic, "Nivel ridica de oxigen");
							sprintf(raspuns, "Nivel ridicat de oxigen. -> %d%%!", param_ox);
							sprintf(alerta, "ALERTA!Nivel ridicat de oxigen. -> %s %d %d!", tdL->nume, tdL->id, param_ox);
							alerta_doctor(alerta);
						}
						else
						{
							strcpy(diagnostic, "Normal");
							snprintf(raspuns,sizeof(raspuns), "Oxigen normal-> %d%%",param_ox);
						}
						fisier(tdL->nume,tdL->id, parametru, valoare,diagnostic);
					}
					else if (strcmp(parametru, "Puls")==0)
					{
						int val_puls=atoi(valoare);
						if(val_puls<60)
						{
							strcpy(diagnostic, "Bradicardie");
							sprintf(raspuns, "Puls scazut. Bradicardie!! -> %d", val_puls);
							sprintf(alerta, "ALERTA! Puls scazut. Bradicardie!! -> %s %d %d!", tdL->nume, tdL->id, val_puls);
							alerta_doctor(alerta);
						}
						else if(val_puls>100)
						{
							strcpy(diagnostic, "Tahicardie");
							sprintf(raspuns, "Puls ridicat. Tahicardie!! -> %d", val_puls);
							sprintf(alerta, "ALERTA! Puls ridicat. Tahicardie!! -> %s %d %d!", tdL->nume, tdL->id, val_puls);
							alerta_doctor(alerta);
						}
						else
						{
							strcpy(diagnostic, "Normal");
							snprintf(raspuns, sizeof(raspuns),"Valoare puls-> %d", val_puls);
						}
						fisier(tdL->nume,tdL->id, parametru, valoare,diagnostic);
					}
					else if (strcmp(parametru, "Temperatura")==0)
					{
						float val_temp=atof(valoare);
						if(val_temp<35)
						{
							strcpy(diagnostic, "Hipotermie");
							sprintf(raspuns, "Temperatura scazuta. Hipotermie!! -> %.1f C", val_temp);
							sprintf(alerta, "ALERTA! Temperatura scazuta. Hipotermie!! -> %s %d %.1f C", tdL->nume, tdL->id, val_temp);
							alerta_doctor(alerta);
						}
						else if(val_temp>38)
						{
							strcpy(diagnostic, "Febra");
							sprintf(raspuns, "Temperatura ridicata. Febraa!! -> %.1f C", val_temp);
							sprintf(alerta, "ALERTA! Temperatura ridicataaa. Febraaa!! -> %s %d %.1f C", tdL->nume, tdL->id, val_temp);
							alerta_doctor(alerta);
						}
						else
						{
							strcpy(diagnostic, "Normal");
							snprintf(raspuns, sizeof(raspuns),"Temperatura -> %.1f C", val_temp);
						}
						fisier(tdL->nume,tdL->id, parametru, valoare,diagnostic);
					}
					else 
					{
    					strcpy(raspuns, "Parametru necunoscut! Folositi: Tensiune, Saturatie, Puls, Temperatura.");
					}				
				}
	    	}
	    	else 
	    	{
				strcpy(raspuns,"Nu se poate face update");
	    	}
		}
		else if(strncmp(comanda,"Logout",6)==0)
		{	
			strcpy(raspuns, "Logout realizat cu succes!");
			write(tdL->cl,raspuns, strlen(raspuns));
			break;
		}	
		else
		{
	    	strcpy(raspuns,"Comanda necunoscuta");
		}

		if(write(tdL->cl,raspuns,strlen(raspuns))<=0)
		{
	    	printf("[Thread %d]", tdL->idThread);
	    	perror("[Thread]Eroare la write() catre client.\n");
			break;
		}
    }
	return NULL;
}
