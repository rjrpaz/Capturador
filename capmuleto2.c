/************************************************\
*  Gerencia de Telecomunicaciones y Teleprocesos *
\************************************************/

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "capturador.h"

#define DBUP "/root/dbup_muleto"

int hardware, flowchar[2], pid, poschar;
char *logfile="/var/log/capturador.log", *device="/dev/ttyS0", *hardware_control="FALSE", *baudrate="9600", linea[103];
char bandera, bandera_standar, bandera_accounting, c, c_ant, registro, llamante[32], llamado[32], codigo[10], fecha[10], cadena[256], partyA[33], partyB[32], pulsos[6], duracion[6];
int mes, dia, hora, minuto, i, j, atoi_duracion;


void usage(char *comando)
{
	printf("\nuso: %s [OPCIONES]\n", comando);
	puts("OPCIONES:");
	puts("\t-l <archivo_log> (Valor por defecto: /var/log/capturador.log)");
	puts("\t-b {38400 | 19200 | 9600 | 4800 | 2400 | 1200 | 600 | 300}");
	puts("\t\t(Valor por defecto: 9600 bps)");
	puts("\t-d <device_capturado> (Valor por defecto: /dev/ttyS0)");
	puts("\t-x (Habilita control de flujo por hardware)");
	puts("\t-h Presenta este menu de ayuda\n");
}


void procesar_opciones(int argc, char *argv[])
{
	while ((c = getopt(argc, argv, "l:b:d:x:h")) != EOF)
	{
		switch (c)
		{
			case 'l':
				logfile = optarg;
				break;
			case 'b':
				baudrate = optarg;
				break;
			case 'd':
				device = optarg;
				break;
			case 'x':
				hardware_control = "TRUE";
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
	int fd, /* logfd, */ref_no, ref_no_ant=100;
	FILE *logfd, *logfd2;
	struct termios oldtio, newtio;

	procesar_opciones(argc, argv);

	printf ("Logfile: %s\n", logfile);
	printf ("Device: %s\n", device);

/**********************************************************************\
* Abre el modem como lectura-escritura, y no como una tty que controla *
* al proceso, de esta forma no se muere si por una linea ruidosa llega *
* una señal equivalente a CTRL-C.                                      *
\**********************************************************************/
	fd = open(device, O_RDWR | O_NOCTTY);
	if (fd <0) 
	{
		perror(device); 
		exit(EXIT_FAILURE); 
	}
 
/**********************************************************************\
* Abre archivo de log                                                  *
\**********************************************************************/
/*
	logfd = open(logfile, O_CREAT | O_WRONLY | O_APPEND, 0600);
*/
	logfd = fopen(logfile, "a");
	if (logfd <0) 
	{
		perror(logfile); 
		exit(EXIT_FAILURE); 
	}

	logfd2 = fopen("/root/resultado.lst", "a");
	if (logfd2 <0) 
	{
		perror("resultado.lst"); 
		exit(EXIT_FAILURE); 
	}



/**********************************************************************\
* Creacion de la tuberia que envia caracteres al proceso que genera    *
* el socket                                                            *
\**********************************************************************/
	if (pipe(flowchar) == -1)
	{
		perror("pipe");
		exit (EXIT_FAILURE);
	}


/**********************************************************************\
* Si el archivo es una terminal, la configura, si es un archivo        *
* regular, simplemente lo lee .                                        *
\**********************************************************************/
	if (isatty(fd) == 1)
	{
/**********************************************************************\
* Guarda la configuracion previa de la terminal.                       *
\**********************************************************************/
		if (tcgetattr(fd,&oldtio) < 0)
		{
			perror("tcgetattr"); 
			exit (EXIT_FAILURE);
		}
 
/**********************************************************************\
* Configura la velocidad en bps, control de flujo desde el hardware, y *
* 8N1 (8 bit, sin paridad, 1 bit de stop). Tambien no se cuelga        *
* automaticamente e ignora estado del modem. Finalmente, habilita la   *
* terminal para recibir caracteres.                                    *
* newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;          *
* CRTSCTS habilita control de flujo desde hardware, que puede ir o no. *
\**********************************************************************/
		newtio.c_cflag = bps(baudrate) | CS8 | CLOCAL | CREAD;
		if (strcmp(hardware_control, "TRUE") == 0)
		{
			newtio.c_cflag |= CRTSCTS;
		}
 
/**********************************************************************\
* Ignora CR en la entrada                                              *
	newtio.c_iflag = IXON | IXOFF | IGNBRK | ISTRIP | IGNPAR;
* newtio.c_iflag = IGNPAR;                                             *
	newtio.c_iflag = IGNCR;
\**********************************************************************/
		newtio.c_iflag = 0;
 
/**********************************************************************\
* salida "raw".                                                        *
\**********************************************************************/
		newtio.c_oflag = 0;
 
/**********************************************************************\
* No hace echo de caracteres y no genera señales.                      *
\**********************************************************************/
		newtio.c_lflag = 0;
 
/**********************************************************************\
* Bloquea la lectura hasta que un caracter llega por la linea.         *
\**********************************************************************/
		newtio.c_cc[VMIN]=1;
		newtio.c_cc[VTIME]=0;
 
/**********************************************************************\
* Limpia la terminal y activa el seteo de la misma.                    *
\**********************************************************************/
		if (tcflush(fd, TCIFLUSH) < 0)
		{
		  	perror("tcflush");
			exit (EXIT_FAILURE);
		}
		
		if (tcsetattr(fd,TCSANOW,&newtio) < 0)
		{
			perror("tcsetattr");
			exit (EXIT_FAILURE);
		}
	}
 
/**********************************************************************\
* Creacion del proceso hijo que analiza el string, y lo envia si es    *
* una llamada saliente.                                                *
\**********************************************************************/
	if ((pid=fork()) == -1)
	{
		perror("fork");
		exit (EXIT_FAILURE);
	}
/**********************************************************************\
* Proceso hijo: este proceso se encarga de leer un caracter de la      *
* tuberia y lo va guardando en un string hasta que recibe la secuencia *
* de caracteres que denota el fin del string. Aqui analiza si es una   *
* llamada saliente, y si es el caso, y ademas el formato es el         *
* correcto, genera un nuevo proceso hijo que se encarga de generar un  *
* socket que actualice la Base de Datos.                               *
\**********************************************************************/
	else if (pid == 0)
	{
		close(flowchar[1]);
		while ((i=read(flowchar[0],&c,1)) > 0)
		{
			fwrite(&c, sizeof(c), 1, logfd2);
		} /* Fin de while */

		close(flowchar[0]);
		exit (0);
	}
/**********************************************************************\
* Proceso padre: este proceso es el encargado de capturar el puerto    *
* serial, escribirlo en el log, y escribirlo en la tuberia para que lo *
* reciba el proceso hijo, para que realice el analisis descripto       *
* arriba.                                                              *
\**********************************************************************/
	else
	{
		close(flowchar[0]);
		fclose(logfd2);
		/* modem */
		while (read(fd,&c,1) > 0)
		{
			/* archivo de log */
			fwrite(&c, sizeof(c), 1, logfd);
			/* tuberia */
			write(flowchar[1],&c,1);
		}
		if (isatty(fd) == 1)
		{
			tcsetattr(fd,TCSANOW,&oldtio);
		}
		close(fd);
		close(flowchar[1]);
		fclose(logfd);
		exit (0);
	}
}
