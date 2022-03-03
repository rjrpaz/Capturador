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

#define DBUSER "cappolicia"
#define DBPASS "chirolita"
#define DBTABLE "Policia"

int port = 666;
static int duracion, codigo;
static char fecha[11], c, tipo_llamada[20], buffer[BUFSIZ], buf_res[BUFSIZ], llamante[24], llamado[24], comentario[256];

static time_t tiempo_actual;

static void usage(char *comando)
{
	printf("\nuso: %s [OPCIONES]\n", comando);
	puts("OPCIONES:");
	puts("\t-t <tipo_llamada> (entrante|saliente|interna)");
	puts("\t-c <codigo_personal>");
	puts("\t-A <numero_llamante>");
	puts("\t-B <numero_llamado>");
	puts("\t-f <fecha> (formato yymmddhhmm)");
	puts("\t-d <duracion> (en segundos)");
	puts("\t-C \"Comentario\" (entre comillas - solo entrantes.)");
	puts("\t-h Presenta este menu de ayuda\n");
}


static void procesar_opciones(int argc, char *argv[])
{

	if ((argc != 13) && (argc != 15))
	{
		usage(argv[0]);
		exit(EXIT_SUCCESS);
	}

	while ((c = getopt(argc, argv, "t:c:A:B:f:d:C:")) != EOF)
	{
		switch (c)
		{
			case 't':
				strcpy(tipo_llamada, optarg);
				if (((strcmp(tipo_llamada,"saliente")) != 0) && ((strcmp(tipo_llamada,"entrante")) != 0) && ((strcmp(tipo_llamada,"interna")) != 0))
				{
					puts("Ese tipo de llamada no es valido");
					exit(EXIT_SUCCESS);
				}
				break;
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
			case 'd':
				duracion = atoi(optarg);
				break;
			case 'C':
				strcpy(comentario, optarg);
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
	char *str="";
	int socket_descriptor;
	struct sockaddr_in pin;
	struct hostent *server_host_name;

	procesar_opciones(argc, argv);

	if ((strcmp(tipo_llamada, "entrante")) == 0)
	{
		sprintf(buf, "%s|%s|%s|%s|%d|%s|%s|%s|%d|\"%s\"\n", tipo_llamada, DBTABLE, DBUSER, DBPASS, codigo, llamante, llamado, fecha, duracion, comentario);
	}
	else
	{
		sprintf(buf, "%s|%s|%s|%s|%d|%s|%s|%s|%d\n", tipo_llamada, DBTABLE, DBUSER, DBPASS, codigo, llamante, llamado, fecha, duracion);
	}
/*
	printf("Cadena: %s", buf);
*/
	str = buf;

	if ((server_host_name = gethostbyname(DBHOST)) == 0)
	{
		perror("Error al intentar resolver nombre del host");
		sprintf(buf, "%s|%d|%s|%s|%s|%d|\"%s\"\n", tipo_llamada, codigo, llamante, llamado, fecha, duracion, comentario);
		str = buf;
		spool_entry(str);
		exit(EXIT_FAILURE);
	}

	bzero(&pin, sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = htonl(INADDR_ANY);
	pin.sin_addr.s_addr = ((struct in_addr *)(server_host_name->h_addr))->s_addr;
	pin.sin_port = htons(port);

	if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Error al abrir el socket");
		sprintf(buf, "%s|%d|%s|%s|%s|%d|\"%s\"\n", tipo_llamada, codigo, llamante, llamado, fecha, duracion, comentario);
		str = buf;
		spool_entry(str);
		exit(EXIT_FAILURE);
	}

	if (connect(socket_descriptor, (void *)&pin, sizeof(pin)) == -1)
	{
		perror("Error al conectarse al socket");
		sprintf(buf, "%s|%d|%s|%s|%s|%d|\"%s\"\n", tipo_llamada, codigo, llamante, llamado, fecha, duracion, comentario);
		str = buf;
		spool_entry(str);
		close(socket_descriptor);
		exit(EXIT_FAILURE);
	}

	if (send(socket_descriptor, str, strlen(str), 0) == -1)
	{
		perror("Error al enviar los datos");
		sprintf(buf, "%s|%d|%s|%s|%s|%d|\"%s\"\n", tipo_llamada, codigo, llamante, llamado, fecha, duracion, comentario);
		str = buf;
		spool_entry(str);
		close(socket_descriptor);
		exit(EXIT_FAILURE);
	}

	if (recv(socket_descriptor, buf_res, sizeof(buf_res), 0) == -1)
	{
		perror("Error al recibir respuesta del server");
		sprintf(buf, "%s|%d|%s|%s|%s|%d|\"%s\"\n", tipo_llamada, codigo, llamante, llamado, fecha, duracion, comentario);
		str = buf;
		spool_entry(str);
		close(socket_descriptor);
		exit(EXIT_FAILURE);
	}
	else
	{
		if (strcmp(buf_res, "OK") != 0)
		{
			if ((strcmp(buf_res, "ERRCONNDB") == 0) || (strcmp(buf_res, "") == 0))
			{
				perror("Error en el server, al intentar conectarse a la base de datos");
				sprintf(buf, "%s|%d|%s|%s|%s|%d|\"%s\"\n", tipo_llamada, codigo, llamante, llamado, fecha, duracion, comentario);
				str = buf;
				spool_entry(str);
				close(socket_descriptor);
				exit(EXIT_FAILURE);
			}
			else
			{
				sprintf(comando, "(echo \"Subject: CAPTURADOR - ERROR\"\necho \"To:root\"\necho\"\"\necho \"Mensaje de Error:%s\"\necho \"%s\"\n )|/usr/bin/sendmail root", buf_res, buf);
				system(comando);
				close(socket_descriptor);
				exit(EXIT_FAILURE);
			}
		}
	}

	close(socket_descriptor);
}
