/***********************************************\
* Gerencia de Telecomunicaciones y Teleprocesos *
* Segun el port por donde sale la llamada, el   *
* analisis del string debe contemplar los       *
* siguientes aspectos:                          *
* Port 01-04: El 1er. caracter debe ser 0, el   *
* 				cual debe ser eliminado, sino   *
* 				se descarta                     *
* Port 09,10: Son internos, se descartan        *
* Port 13-16: Los numeros se toman tal como     *
* 				figuran (valijas de celulares)  *
* Port 17-20: Idem 01-04                        *
\***********************************************/

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "capturador.h"

#define DBUP "/root/dbup_deportes"

extern char *tzname[2];
long int timezone;

int hardware, flowchar[2], parser[2], pid, pid_hijo, poschar=0;
char *logfile="/var/log/capturador.log", *device="/dev/ttyS0", *hardware_control="FALSE", *baudrate="9600", linea[MAXLINE], linea_enviada[MAXLINE];
char bandera, c, c_ant, registro, llamante[24], llamado[24], codigo[10], fecha[10], fecha_fin[10], cadena[256], pulsos[4], anio[4];
int mes, dia, hora, minuto, segundo, port, i, j, k;
long int duracion;
struct tm *tm_ptr, *tiempo_antes_ptr, *tiempo_despues_ptr;
time_t tiempo_actual, *tiempo_antes, *tiempo_despues;

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
		close(flowchar[1]);

		if (pipe(parser) == -1)
		{
			perror("pipe");
			exit(EXIT_FAILURE);
		}
	
		if ((pid_hijo=fork()) == -1)
		{
			perror("fork");
			exit (EXIT_FAILURE);
		}
		else if (pid_hijo == 0) /* Proceso hijo en el hijo */
		{
			close(parser[1]);
			close(flowchar[0]);

			while ((read(parser[0],linea,MAXLINE)) > 0)
			{
/*
				printf ("Linea: %s\n", linea);
*/
				bandera = 'y';
				/************************************************\ 
				* Chequea que el string tenga el largo adecuado. *
				\************************************************/
				if (strlen(linea) != 82)
				{
					bandera = 'n';
					error_log ("Error en el largo", linea);
				}

				/*******************************************\ 
				* Chequea si la linea es de las especiales. *
				\*******************************************/ 

				if ((linea[0] == ' ') || (linea[0] == '-'))
				{
						bandera = 'n';
				}

				/*********************************\ 
				* Chequea el formato de la fecha. *
				\*********************************/ 

				if (bandera != 'n')
				{
					mes = 10*(linea[0] - 48);
					mes += (linea[1] - 48);
					if (mes > 12)
					{
						bandera = 'n';
						error_log ("Mes de inicio no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					dia = 10*(linea[3] - 48);
					dia += (linea[4] - 48);
					if (dia > 31)
					{
						bandera = 'n';
						error_log ("Dia de inicio no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					hora = 10*(linea[9] - 48);
					hora += (linea[10] - 48);
					if ((linea[14] == 'A') && (hora == 12))
					{
						hora = 0;
					}
					if ((linea[14] == 'P') && (hora != 12))
					{
						hora += 12;
					}
					if (hora > 23)
					{
						bandera = 'n';
						error_log ("Hora de inicio no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					minuto = 10*(linea[12] - 48);
					minuto += (linea[13] - 48);
					if (minuto > 59)
					{
						bandera = 'n';
						error_log ("Minuto de inicio no cumple con el formato", linea);
					}
				}

				/***********************************************\ 
				* Determina el puerto por donde sale la llamada *
				\***********************************************/ 
				if (bandera != 'n')
				{
					port = 10*(linea[23] - 48);
					port += (linea[24] - 48);
					if ((port>8) && (port<11))
					{
						bandera = 'n';
					}
				}

				/********************************************\ 
				* Finalmente obtiene los datos utiles que se *
				* encuentran en el registro.                 *
				\********************************************/ 
				if (bandera != 'n')
				{
					/******************************\
					* Calcula la fecha prop. dicha *
					\******************************/
					fecha[0] = linea[6];
					fecha[1] = linea[7];
					fecha[2] = linea[0];
					fecha[3] = linea[1];
					fecha[4] = linea[3];
					fecha[5] = linea[4];
					fecha[6] = '\0';
					sprintf(fecha, "%s%02d%02d", fecha, hora, minuto);
/*
					printf ("Fecha: %s\n", fecha);
					exit;
*/

					/*********************\
					* Obtiene la duracion *
					\*********************/
					hora = 10*(linea[57] - 48);
					hora += (linea[58] - 48);
					minuto = 10*(linea[60] - 48);
					minuto += (linea[61] - 48);
					segundo = 10*(linea[63] - 48);
					segundo += (linea[64] - 48);

					duracion = 3600*hora + 60*minuto + segundo;
			
					if (duracion == 0)
					{
						bandera = 'n';
					}

					/*********************\
					* Obtiene el llamante *
					\*********************/
					j=0;
					for (i=0; i<3; i++)
					{
						if (linea[19+i] != ' ')
						{
							llamante[j] = linea[19+i];
							j++;
						}
					}
					llamante[j] = '\0';

					/*******************\
					* Obtiene el codigo *
					\*******************/
					j=0;
					for (i=0; i<5; i++)
					{
						if (linea[66+i] != ' ')
						{
							codigo[j] = linea[66+i];
							j++;
						}
					}
					codigo[j] = '\0';
					if (strcmp(codigo, "") == 0)
					{
						strcpy(codigo, "0");
					}
							
					/********************\
					* Obtiene el llamado *
					\********************/
					if (port == 1)
					{
						bandera = 'n';
					}
					if ((port>12) && (port<17))
					{
						k=26;
					}
					else
					{
						if (linea[26] == '0')
						{
							k=27;
						}
						else
						{
							k=26;
						}
					}

					j=0;
					for (i=0; i<30; i++)
					{
						if (((linea[k+i] < 48) || (linea[k+i] > 57)) && (linea[k+i] != ' '))
						{
							bandera = 'n';
						}
						if (linea[k+i] != ' ')
						{
							llamado[j] = linea[k+i];
							j++;
						}
					}
					llamado[j] = '\0';
					if ((strcmp(llamado, ""))==0)
					{
						strcpy(llamado, "0");
					}
/*
					for (i=0; i<4; i++)
					{
						pulsos[i] = linea[94+i];
					}
					pulsos[i] = '\0';
*/
					strcpy(pulsos, "0");
					
					/*******************************************\ 
					* Etapa final: Arma la cadena que enviara a *
					* otro proceso, y reinicializa variables.   *
					\*******************************************/ 

					if (bandera != 'n')
					{
/*
						sprintf(cadena, "codigo: %s, llamante: %s, llamado: %s, fecha: %s, pulsos: %s, duracion: %ld", codigo, llamante, llamado, fecha, pulsos, duracion);
*/
/*
						sprintf(cadena, "%s -c %s -A %s -B %s -f %s -p %s -d %ld &", DBUP, codigo, llamante, llamado, fecha, pulsos, duracion);
*/
						sprintf(cadena, "%s|%s|%s|%s|%s|%ld", codigo, llamante, llamado, fecha, pulsos, duracion);

						printf ("%s\n", linea);
						printf ("%s\n", cadena);

/*
						system(cadena);
*/
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
		else
		{ /* proceso padre en el hijo */
			close(parser[0]);

			while (read(flowchar[0],&c,1) > 0)
			{
				linea_enviada[poschar] = c;

				if ((c_ant == CR) && (c == LF))
				{
					linea_enviada[poschar+1] = '\0';

					if (write(parser[1], linea_enviada, MAXLINE) == -1)
					{
						perror("write parser");
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
		exit (EXIT_SUCCESS);
	}
}
