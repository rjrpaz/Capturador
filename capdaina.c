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

#define DBUP "/root/dbup_daina"

extern char *tzname[2];
long int timezone;

int hardware, flowchar[2], parser[2], pid, pid_hijo, poschar=0;
char *logfile="/var/log/capturador.log", *device="/dev/ttyS0", *hardware_control="FALSE", *baudrate="9600", linea[MAXLINE], linea_enviada[MAXLINE];
char bandera, c, c_ant, registro, llamante[24], llamado[24], codigo[10], fecha[10], fecha_fin[10], cadena[256], pulsos[4], anio[4];
int mes, dia, hora, minuto, segundo, i, j, k;
long int duracion;
struct tm *tm_ptr;
struct tm tiempo_antes, tiempo_despues;
time_t tiempo_antes_epoch, tiempo_despues_epoch;
char * letra;

void usage(char *comando)
{
	printf("\nuso: %s [OPCIONES]\n", comando);
	printf("OPCIONES:\n");
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
				exit(EXIT_SUCCESS);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 1)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
}

/*@null@*/
int main(int argc, char *argv[])
{
	int fd;
	FILE *logfd;
	struct termios oldtio, newtio;

	procesar_opciones(argc, argv);

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
	else if (pid == 0) /* Proceso Hijo */
	{
		close (flowchar[1]);

		if (pipe(parser) == -1)
		{
			perror("pipe");
			exit (EXIT_FAILURE);
		}

		if ((pid_hijo=fork()) == -1)
		{
			perror("fork");
			exit (EXIT_FAILURE);
		}
		else if (pid_hijo == 0) /* Proceso hijo en el hijo */
		{
			close (parser[1]);
			close (flowchar[0]);

			while ((read(parser[0],linea,MAXLINE)) > 0)
			{
/*
				printf ("Linea: %s\n", linea);
*/
				bandera = 'y';		
				/************************************************\ 
				* Chequea que el string tenga el largo adecuado. *
				\************************************************/
				if (strlen(linea) != 132)
				{
					bandera = 'n';
					error_log ("Error en el largo", linea);
				}

				/*****************************************************\ 
				* Chequea que aparezca caracter 02H en la posicion 0. *
				\*****************************************************/
				if ((linea[0] != 2) && (bandera != 'n'))
				{
					bandera = 'n';
					error_log ("No aparece caracter 02H en posicion 0", linea);
				}

				/********************************************\ 
				* Chequea que aparezca 0 en la posicion 1. *
				\********************************************/
				if ((linea[1] != '0') && (bandera != 'n'))
				{
					bandera = 'n';
					error_log ("No aparece 0 en posicion 1", linea);
				}

				/******************************************\ 
				* Chequea que aparezca ! en la posicion 2. *
				\******************************************/
				if ((linea[2] != '!') && (bandera != 'n'))
				{
					bandera = 'n';
					error_log ("No aparece ! en posicion 2", linea);
				}

				/******************************************\ 
				* Chequea que aparezca K en la posicion 3. *
				\******************************************/
				if ((linea[3] != 'K') && (bandera != 'n'))
				{
					bandera = 'n';
					error_log ("No aparece K en posicion 3", linea);
				}

				/************************************************\ 
				* Determina si el tipo de registro es de llamada *
				* saliente.                                      *
				\************************************************/
				if ((linea[4] != 'A') && (bandera != 'n'))
				{
					bandera = 'n';
				}

				/************************************************\ 
				* Determina si hay espacio entre posicion 48 y   *
				* 50.                                            *
				\************************************************/
				for (i=50; i<53; i++)
				{
					if ((linea[i] != 32) && (bandera != 'n'))
					{
						bandera = 'n';
						sprintf(cadena, "No aparece espacio en posicion %d", i -2);
						error_log (cadena, linea);
					}
				}
				
				/************************************************\ 
				* Debe figurar 0 en posicion 51 y 53             *
				\************************************************/
/*
				Aparentemente el primero de los 3 caracteres de estado ya no
				tiene un valor fijo en 0


				if ((linea[53] != '0') && (bandera != 'n'))
				{
					bandera = 'n';
					error_log ("No aparece 0 en posicion 51", linea);
				}
				
				if ((linea[55] != '0') && (bandera != 'n'))
				{
					bandera = 'n';
					error_log ("No aparece 0 en posicion 53", linea);
				}
*/				
				/************************************************\ 
				* Determina si hay espacio entre posicion 84 y   *
				* 91.                                            *
				\************************************************/
				for (i=86; i<94; i++)
				{
					if ((linea[i] != 32) && (bandera != 'n'))
					{
						bandera = 'n';
						sprintf(cadena, "No aparece espacio en posicion %d", i -2);
						error_log (cadena, linea);
					}
				}

				/************************************************\ 
				* Determina si hay espacio entre posicion 96 y   *
				* 103.                                           *
				\************************************************/
				for (i=98; i<106; i++)
				{
					if ((linea[i] != 32) && (bandera != 'n'))
					{
						bandera = 'n';
						sprintf(cadena, "No aparece espacio en posicion %d", i -2);
						error_log (cadena, linea);
					}
				}

				/************************************************\ 
				* Determina si hay espacio entre posicion 114 y  *
				* 117.                                           *
				\************************************************/
/*
				for (i=116; i<120; i++)
				{
					if ((linea[i] != 32) && (bandera != 'n'))
					{
						printf ("%c%c\n", linea[i], linea[i+1]);
						bandera = 'n';
						sprintf(cadena, "No aparece espacio en posicion %d", i -2);
						error_log (cadena, linea);
					}
				}
*/
				/************************************************\ 
				* Determina si hay espacio entre posicion 125 y  *
				* 128.                                           *
				\************************************************/
				for (i=127; i<131; i++)
				{
					if ((linea[i] != 32) && (bandera != 'n'))
					{
						bandera = 'n';
						sprintf(cadena, "No aparece espacio en posicion %d", i -2);
						error_log (cadena, linea);
					}
				}

				/*********************************\ 
				* Chequea el formato de la fecha. *
				\*********************************/ 

				if (bandera != 'n')
				{
					mes = 10*(linea[20] - 48);
					mes += (linea[21] - 48);
					if (mes > 12)
					{
						bandera = 'n';
						error_log ("Mes de inicio no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					mes = 10*(linea[30] - 48);
					mes += (linea[31] - 48);
					if (mes > 12)
					{
						bandera = 'n';
						error_log ("Mes de fin no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					dia = 10*(linea[22] - 48);
					dia += (linea[23] - 48);
					if (dia > 31)
					{
						bandera = 'n';
						error_log ("Dia de inicio no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					dia = 10*(linea[32] - 48);
					dia += (linea[33] - 48);
					if (dia > 31)
					{
						bandera = 'n';
						error_log ("Dia de fin no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					hora = 10*(linea[24] - 48);
					hora += (linea[25] - 48);
					if (hora > 23)
					{
						bandera = 'n';
						error_log ("Hora de inicio no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					hora = 10*(linea[34] - 48);
					hora += (linea[35] - 48);
					if (hora > 23)
					{
						bandera = 'n';
						error_log ("Hora de fin no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					minuto = 10*(linea[26] - 48);
					minuto += (linea[27] - 48);
					if (minuto > 59)
					{
						bandera = 'n';
						error_log ("Minuto de inicio no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					minuto = 10*(linea[36] - 48);
					minuto += (linea[37] - 48);
					if (minuto > 59)
					{
						bandera = 'n';
						error_log ("Minuto de fin no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					segundo = 10*(linea[28] - 48);
					segundo += (linea[29] - 48);
					if (segundo > 59)
					{
						bandera = 'n';
						error_log ("Segundo de inicio no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					segundo = 10*(linea[38] - 48);
					segundo += (linea[39] - 48);
					if (segundo > 59)
					{
						bandera = 'n';
						error_log ("Segundo de fin no cumple con el formato", linea);
					}
				}

				/************************************************\ 
				* Determina si la llamada fue contestada ??????. *
				\************************************************/ 
						
				/********************************************\ 
				* Finalmente obtiene los datos utiles que se *
				* encuentran en el registro.                 *
				\********************************************/ 
				if (bandera != 'n')
				{
					/******************************\
					* Calcula la fecha prop. dicha *
					\******************************/
					/* Anio */
					for (i=0; i<2; i++)
					{
						fecha[i] = linea[116+i];
					}
					/* Mes, dia, hora, minuto */
					for (i=2; i<10; i++)
					{
						fecha[i] = linea[18+i];
					}
					fecha[i] = '\0';

					for (i=0; i<2; i++)
					{
						fecha_fin[i] = linea[118+i];
					}
					for (i=2; i<10; i++)
					{
						fecha_fin[i] = linea[28+i];
					}
					fecha_fin[i] = '\0';
					/*********************\
					* Obtiene la duracion *
					\*********************/


					tiempo_antes.tm_year = (linea[116] - 48)* 10 + (linea[117] - 48) + 100;
					tiempo_despues.tm_year = (linea[118] - 48)* 10 + (linea[119] - 48) + 100;
					tiempo_antes.tm_mon = (linea[20] - 48)* 10 + (linea[21] - 48) - 1;
					tiempo_despues.tm_mon = (linea[30] - 48)* 10 + (linea[31] - 48) - 1;
					tiempo_antes.tm_mday = (linea[22] - 48)* 10 + (linea[23] - 48);
					tiempo_despues.tm_mday = (linea[32] - 48)* 10 + (linea[33] - 48);
					tiempo_antes.tm_hour = (linea[24] - 48)* 10 + (linea[25] - 48);
					tiempo_despues.tm_hour = (linea[34] - 48)* 10 + (linea[35] - 48);
					tiempo_antes.tm_min = (linea[26] - 48)* 10 + (linea[27] - 48);
					tiempo_despues.tm_min = (linea[36] - 48)* 10 + (linea[37] - 48);
					tiempo_antes.tm_sec = (linea[28] - 48)* 10 + (linea[29] - 48);
					tiempo_despues.tm_sec = (linea[38] - 48)* 10 + (linea[39] - 48);
					tiempo_antes.tm_wday = 0;
					tiempo_despues.tm_wday = 0;
					tiempo_antes.tm_yday = 0;
					tiempo_despues.tm_yday = 0;
					tiempo_antes.tm_isdst = 0;
					tiempo_despues.tm_isdst = 0;

					if ((tiempo_antes_epoch = mktime(&tiempo_antes)) == -1)
					{
						error_log ("Error usando mktime", linea);
					}
					if ((tiempo_despues_epoch = mktime(&tiempo_despues)) == -1)
					{
						error_log ("Error usando mktime", linea);
					}

					duracion = difftime(tiempo_despues_epoch, tiempo_antes_epoch);
					j=0;
					for (i=0; i<6; i++)
					{
						if (linea[14+i] != ' ')
						{
							llamante[j] = linea[14+i];
							j++;
						}
					}
					llamante[j] = '\0';

					j=0;
					for (i=0; i<10; i++)
					{
						if (linea[40+i] != ' ')
						{
							codigo[j] = linea[40+i];
							j++;
						}
					}
					codigo[j] = '\0';
					if (strcmp(codigo, "") == 0)
					{
						strcpy(codigo, "0");
					}

					k = 62;
					if (linea[k] == '0')
					{
						k++;
					}

					j=0;
					for (i=0; i<23; i++)
					{
						if (linea[k+i] != ' ')
						{
							llamado[j] = linea[k+i];
							j++;
						}
					}
					llamado[j] = '\0';
					if (strlen(llamado) == 0)
					{
						strcpy(llamado, "0");
					}
							
					for (i=0; i<4; i++)
					{
						pulsos[i] = linea[94+i];
					}
					pulsos[i] = '\0';
					
					/*******************************************\ 
					* Etapa final: Arma la cadena que enviara a *
					* otro proceso, y reinicializa variables.   *
					\*******************************************/ 
/*
					sprintf(cadena, "codigo: %s, llamante: %s, llamado: %s, fecha: %s, pulsos: %s, duracion: %ld", codigo, llamante, llamado, fecha, pulsos, duracion);
					printf ("----------\n%s\n", cadena);
*/

					sprintf(cadena, "%s -c %s -A %s -B %s -f %s -p %s -d %ld &", DBUP, codigo, llamante, llamado, fecha, pulsos, duracion);
/*
					printf ("----------\n%s\n", cadena);
*/

/*
					if (llamado[0] != '0')
					{
						for (i=0; i<(strlen(linea) - 5); i++)
						{
							printf ("%c", linea[5+i]);
						}
						printf (" - ");
					}
					*/
					if (duracion != 0)	
					{
/*
					printf ("%s\n", cadena);
*/
						system(cadena);
					}
					strcpy(codigo, "");
					strcpy(llamante, "");
					strcpy(llamado, "");
					strcpy(fecha, "");
					strcpy(pulsos, "");
					duracion = 0;
				} /* Fin del if */
			} /* Fin de while */
			close(parser[0]);
			exit (0);
		}
		else /* proceso padre en el hijo */
		{
			close(parser[0]);
		
			while (read(flowchar[0],&c,1) > 0)
			{
				linea_enviada[poschar] = c;

				if (c == 3)
				{
					linea_enviada[poschar+1] = '\0';

					if (write(parser[1], linea_enviada, MAXLINE) == -1)
					{
						perror ("write parser");
					}
					poschar = -1;
				}

				c_ant = c;
				poschar++;
			}
			close(parser[1]);
			exit (0);
		}
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
		printf ("Logfile: %s\n", logfile);
		printf ("Dispositivo: %s\n", device);

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
		logfd = fopen(logfile, "a");
		if (logfd <0) 
		{
			perror(logfile); 
			exit(EXIT_FAILURE); 
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

		/* modem */
		while (read(fd,&c,1) > 0)
		{
			/* archivo de log */
			fwrite(&c, sizeof(c), 1, logfd);
			fflush(logfd);

			/* tuberia */
			if (write(flowchar[1],&c,1) != 1)
			{
				perror("write error to flowchar");
			}
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
