/************************************************\
*  Gerencia de Telecomunicaciones y Teleprocesos *
\************************************************/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <mysql/mysql.h>
#include "capturador.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#define DBUSER "capmuleto"
#define DBPASS "chirolita"
#define DBTABLE "Muleto"
#define DBHOST "10.18.12.87"

int port = 666;
static int duracion, pulsos, codigo;
static char fecha[11], c, llamante[24], llamado[24];

static void usage(char *comando)
{
	printf("\nuso: %s [OPCIONES]\n", comando);
	puts("OPCIONES:");
	puts("\t-c <codigo_personal>");
	puts("\t-A <numero_llamante>");
	puts("\t-B <numero_llamado>");
	puts("\t-f <fecha> (formato yymmddhhmm)");
	puts("\t-p <pulsos>");
	puts("\t-d <duracion> (en segundos)");
	puts("\t-h Presenta este menu de ayuda\n");
}



static void procesar_opciones(int argc, char *argv[])
{

	if (argc != 13)
	{
		usage(argv[0]);
		exit(EXIT_SUCCESS);
	}

	while ((c = getopt(argc, argv, "c:A:B:f:p:d:")) != EOF)
	{
		switch (c)
		{
			case 'c':
				codigo = atoi(optarg);
				break;
			case 'A':
				strcpy(llamante, optarg);
				break;
			case 'B':
				strcpy(llamado, optarg);
				break;
			case 'f':
				strcpy(fecha, optarg);
				break;
			case 'p':
				pulsos = atoi(optarg);
				break;
			case 'd':
				duracion = atoi(optarg);
				break;
			case 'h':
			default:
				usage(argv[0]);
				exit(EXIT_SUCCESS);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 1)
	{
		usage(argv[0]);
		exit(EXIT_SUCCESS);
	}
}


int main(int argc, char *argv[])
{
	char buf[BUFSIZ];
	char comando[BUFSIZ];

	procesar_opciones(argc, argv);
	
	sprintf(buf, "%s|%s|%s|%d|%s|%s|%s|%d|%d\n", DBTABLE, DBUSER, DBPASS, codigo, llamante, llamado, fecha, pulsos, duracion);
	printf("Cadena: %s", buf);
	
	sprintf(comando, "(echo \"Subject: Error en alta de llamada\"\necho \"To:root\"\necho\"\"\necho \"%s\"\n )|/usr/bin/sendmail root", buf);
	system(comando);
	exit (0);
}
