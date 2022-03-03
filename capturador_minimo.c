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

int hardware, flowchar[2], pid, poschar;
char *logfile="/var/log/capturador.log", *device="/dev/ttyS0", *hardware_control="FALSE", *baudrate="1200", linea[103];
char bandera, c, c_ant, registro, llamante[32], llamado[32], fecha[10], fecha_fin[10], cadena[256], partyA[33], partyB[32], pulsos[6], duracion[6], anio[2];
int mes, dia, hora, minuto, i, j, errorfd;
struct tm *tm_ptr;
time_t tiempo_actual;


void usage(char *comando)
{
	printf("\nuso: %s [OPCIONES]\n", comando);
	puts("OPCIONES:");
	puts("\t-l <archivo_log> (Valor por defecto: /var/log/capturador.log)");
	puts("\t-b {38400 | 19200 | 9600 | 4800 | 2400 | 1200 | 600 | 300}");
	puts("\t\t(Valor por defecto: 1200 bps)");
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
	argc -= optind;
	argv += optind;

	if (argc > 1)
	{
		usage(argv[0]);
		exit(1);
	}
}


main(int argc, char *argv[])
{
	int fd, logfd, ref_no, ref_no_ant;
	struct termios oldtio, newtio, oldstdtio, newstdtio;

	procesar_opciones(argc, argv);

	printf ("Logfile: %s\n", logfile);
	printf ("Device: %s\n", device);
	printf ("bps: %s\n", baudrate);

/**********************************************************************\
* Abre el modem como lectura-escritura, y no como una tty que controla *
* al proceso, de esta forma no se muere si por una linea ruidosa llega *
* una señal equivalente a CTRL-C.                                      *
\**********************************************************************/
	fd = open(device, O_RDWR | O_NOCTTY);
	if (fd <0) 
	{
		perror(device); 
		exit(-1); 
	}
 
/**********************************************************************\
* Abre archivo de log para los errores                                 *
\**********************************************************************/
	errorfd = open(LOGERROR, O_CREAT | O_WRONLY | O_APPEND, 0600);
	if (errorfd <0) 
	{
		perror(LOGERROR); 
		exit(-1); 
	}

/**********************************************************************\
* Abre archivo de log                                                  *
\**********************************************************************/
	logfd = open(logfile, O_CREAT | O_WRONLY | O_APPEND, 0600);
	if (logfd <0) 
	{
		perror(logfile); 
		exit(-1); 
	}

/**********************************************************************\
* Guarda la configuracion previa de la terminal.                       *
\**********************************************************************/
	tcgetattr(fd,&oldtio); 

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
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);

	while (1)
	{
		/* modem */
		read(fd,&c,1);
		/* archivo de log */
		write(logfd,&c,1);
	}
	tcsetattr(fd,TCSANOW,&oldtio);
	close(fd);
	close(logfd);
	close(errorfd);
	exit (0);
}
