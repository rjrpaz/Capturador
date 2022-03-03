/***********************************************\
*                                               *
* Gerencia de Telecomunicaciones y Teleprocesos *
*                                               *
* Autor: Roberto J. R. Paz                      *
*                                               *
* Fecha: 23/02/2003                             *
*                                               *
\***********************************************/

/*********************************************************************************\
*                                                                                 * 
* Este programa monitorea que el archivo que captura la informacion por el puerto *
* serial del equipo, varie a lo largo del tiempo ( segun el horario del dia).     *
* Cuando determina que no llega informacion desde la central, este proceso se     *
* muere, situacion que es detectada por el programa de Monitore Big Brother.      *
*                                                                                 * 
/*********************************************************************************/

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <fcntl.h>
#include <time.h>
#include "capturador.h"

int periodo = 10;
int hardware, flowchar[2], parser[2], pid, pid_hijo, poschar=0;

char *logfile="/var/log/capturador.log", *device="/dev/ttyS0", *hardware_control="FALSE", *baudrate="B9600", linea[MAXLINE], linea_enviada[MAXLINE];
char bandera, bandera_standar, bandera_accounting, c, c_ant, registro, llamante[32], llamado[32], codigo[10], fecha[10], cadena[256], partyA[33], partyB[32], pulsos[6], duracion[6];
int mes, dia, hora, minuto, i, j, atoi_duracion;


void usage(char *comando)
{
	printf("\nuso: %s [OPCIONES]\n", comando);
	puts("OPCIONES:");
	puts("\t-l <archivo_log_a_vigilar> (Valor por defecto: /var/log/capturador.log)");
	puts("\t-p <periodo_de_tiempo_de_latencia_en_minutos> (Valor por defecto: 10 minutos)");
	puts("\t-h Presenta este menu de ayuda\n");
}


void procesar_opciones(int argc, char *argv[])
{
	while ((c = getopt(argc, argv, "l:p:h")) != EOF)
	{
		switch (c)
		{
			case 'l':
				logfile = optarg;
				break;
			case 'p':
				periodo = atoi(optarg);
				break;
			case 'h':
			default:
				usage(argv[0]);
				exit(1);
		}
	}
}


int main(int argc, char *argv[])
{
	int fd, ref_no, ref_no_ant=100;
	FILE *logfd;
	struct stat estado;
	time_t tiempo_actual;
	struct tm *tm_ptr;

	procesar_opciones(argc, argv);
	openlog (argv[0], LOG_PID,LOG_DAEMON);
	setlogmask (LOG_UPTO(LOG_ERR));
	syslog (LOG_ALERT, "Process Started");

	periodo = periodo * 60;

	while (1)
	{
		/**********************\
		* Obtiene fecha actual *
		\**********************/
		time(&tiempo_actual);
		tm_ptr = localtime(&tiempo_actual);

		/*********************************************\
		* Obtiene la fecha del log que esta vigilando *
		\*********************************************/
		stat(logfile, &estado);

		/*************************************************************\
		* Solamente se fija que haya actividad en los horarios en que *
		* hay empleados: de 8 a 20 hs.                                *
		\*************************************************************/
		if ((tm_ptr->tm_hour >= 8) && (tm_ptr->tm_hour < 20))
		{
			/***************************\
			* Ignora Sabados y Domingos *
			\***************************/
			if ((tm_ptr->tm_wday > 0) && (tm_ptr->tm_wday < 6))
			{
				/*****************************************************************\
				* Si el proceso esta inactivo mas tiempo de lo debido, el proceso *
				* se termina                                                      *
				\*****************************************************************/
				if ((tiempo_actual - estado.st_mtime) > periodo)
				{
					estado.st_mtime -= 3 * 3600;
					syslog (LOG_ALERT, "PBX inactivity detected at [ %s ]", asctime(gmtime(&estado.st_mtime)));
					syslog (LOG_ALERT, "Process Finished");
					closelog ();
					exit (1);
				}
			}
		}
		sleep(60);
	}
	exit (0);
}
