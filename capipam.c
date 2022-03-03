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

#define DBUP "/root/dbup_ipam"

int hardware, flowchar[2], parser[2], pid, pid_hijo, poschar=0;
char *logfile="/var/log/capturador.log", *device="/dev/ttyS0", *hardware_control="FALSE", *baudrate="4800", linea[MAXLINE], linea_enviada[MAXLINE];
char bandera, c, c_ant_1, c_ant_2, c_ant_3, c_ant_4, c_ant_5, registro, llamante[32], llamado[32], codigo[10], fecha[10], cadena[256], partyA[33], partyB[32], pulsos[6], anio[4], mes_s[2], dia_s[2], duracion[6], mes_texto[3];
int mes, dia, hora, minuto, i, j, atoi_duracion=0;
struct tm *tm_ptr;
time_t tiempo_actual;


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
	int fd, ref_no, ref_no_ant=100;
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
			exit (EXIT_FAILURE);
		}

		if ((pid_hijo=fork()) == -1)
		{
			perror("fork");
			exit (EXIT_FAILURE);
		}
		else if (pid_hijo == 0) /* proceso hijo en el hijo */
		{
			close(parser[1]);
			close(flowchar[0]);

			/****************************************************\
			* Calcula el anio al arrancar el programa, ya que la *
			* central envia esta informacion cada cierto numero  *
			* de lineas, y no en cada una. Idem con mes y dia.   *
			* Ademas, el anio que figura en esta linea especial  *
			* esta mal.                                          *
			\****************************************************/
			if (time(&tiempo_actual) == (time_t)-1)
			{
				error_log ("Error al intentar obtener fecha actual", linea);
			}

			tm_ptr = localtime(&tiempo_actual);
			sprintf(anio, "%d", (tm_ptr->tm_year) + 1900);
			fecha[0] = anio[2];
			fecha[1] = anio[3];
			sprintf(mes_s, "%02d", (tm_ptr->tm_mon) + 1);
			fecha[2] = mes_s[0];
			fecha[3] = mes_s[1];
			sprintf(dia_s, "%02d", tm_ptr->tm_mday);
			fecha[4] = dia_s[0];
			fecha[5] = dia_s[1];

			while ((read(parser[0],linea,MAXLINE)) > 0)
			{
				bandera = 'y';
				/************************************************\ 
				* elimina strings de encabezado.                 *
				\************************************************/

				if (linea[0] == LF)
				{
					bandera = 'n';
				}

				if (linea[0] == CR)
				{
					bandera = 'n';
				}

				/************************************************\ 
				* guion "-".                                     *
				\************************************************/
				if (linea[0] == 95)
				{
					bandera = 'n';
				}

				/************************************************\ 
				* Otros strings de encabezado.                   *
				\************************************************/
				if (strncmp(linea, "END", 3) == 0)
				{
					bandera = 'n';
				}

				if (strncmp(linea, "TOTAL", 5) == 0)
				{
					bandera = 'n';
				}

				if (strncmp(linea, "BEGIN", 5) == 0)
				{
					bandera = 'n';
				}

				if (strncmp(linea, " BEGIN", 6) == 0)
				{
					bandera = 'n';
				}

				if (strncmp(linea, "NUMBER", 6) == 0)
				{
					bandera = 'n';
				}

				if (strncmp(linea, "  SRCE", 6) == 0)
				{
					bandera = 'n';
				}

				if (strncmp(linea, "   ID", 5) == 0)
				{
					bandera = 'n';
				}

				if (strncmp(linea, "DATE", 4) == 0)
				{
					dia = 10*(linea[7] - 48);
					dia += (linea[8] - 48);
					if (dia > 31)
					{
						bandera = 'n';
						error_log ("Dia no cumple con el formato", linea);
					}

					if (bandera != 'n')
					{
						/************************************************\ 
						* Asigna el dia.                                 *
						\************************************************/
						fecha[4] = linea[7];
						fecha[5] = linea[8];

						/************************************************\ 
						* Asigna el mes.                                 *
						\************************************************/
						mes_texto[0] = linea[10];
						mes_texto[1] = linea[11];
						mes_texto[2] = linea[12];
						fecha[2] = '0';
						if (strcmp(mes_texto, "ENE") == 0)
						{
							fecha[3] = '1';
						}
						else if (strcmp(mes_texto, "FEB") == 0)
						{
							fecha[3] = '2';
						}
						else if (strcmp(mes_texto, "MAR") == 0)
						{
							fecha[3] = '3';
						}
						else if (strcmp(mes_texto, "ABR") == 0)
						{
							fecha[3] = '4';
						}
						else if (strcmp(mes_texto, "MAY") == 0)
						{
							fecha[3] = '5';
						}
						else if (strcmp(mes_texto, "JUN") == 0)
						{
							fecha[3] = '6';
						}
						else if (strcmp(mes_texto, "JUL") == 0)
						{
							fecha[3] = '7';
						}
						else if (strcmp(mes_texto, "AGO") == 0)
						{
							fecha[3] = '8';
						}
						else if (strcmp(mes_texto, "SEP") == 0)
						{
							fecha[3] = '9';
						}
						else if (strcmp(mes_texto, "OCT") == 0)
						{
							fecha[2] = '1';
							fecha[3] = '0';
						}
						else if (strcmp(mes_texto, "NOV") == 0)
						{
							fecha[2] = '1';
							fecha[3] = '1';
						}
						else if (strcmp(mes_texto, "DIC") == 0)
						{
							fecha[2] = '1';
							fecha[3] = '2';
						}
						else
						{
							bandera = 'n';
							error_log ("Mes no cumple con el formato", linea);
						}
					}
					bandera = 'n';
				}

				/************************************************\ 
				* Chequea que el string tenga el largo adecuado. *
				\************************************************/
				if (bandera != 'n')
				{
					if (strlen(linea) != 81)
					{
						bandera = 'n';
						error_log ("Error en el largo del registro", linea);
					}
				}

				/************************************************\ 
				* Chequea aparezca * en la primera linea.        *
				\************************************************/
				if ((bandera != 'n') && (linea[0] != 42))
				{
					bandera = 'n';
					error_log ("No aparece * al comienzo de la linea", linea);
				}

				/************************************************\ 
				* Obtiene el numero llamante, y de paso controla *
				* si la llamada es saliente.                     *
				\************************************************/
				if (bandera != 'n')
				{
					j=0;
					for (i=0; i<7; i++)
					{
						/************************************************\ 
						* Si aparece "/", es llamada saliente.           *
						\************************************************/
						if (linea[1+i] == 47)
						{
							bandera = 'n';
							break;
						}

						if ((linea[1+i] > 47) && (linea[1+i] < 58))
						{
							llamante[j] = linea[1+i];
							j++;
						}
					}
					llamante[j] = '\0';
					if (strcmp(llamante, "") == 0)
					{
						strcpy(llamante, "0");
					}
				}

				/**********************************************\ 
				* Chequea el formato del resto de la fecha.    *
				\**********************************************/
				if (bandera != 'n')
				{
					hora = 10*(linea[46] - 48);
					hora += (linea[47] - 48);
					if (hora > 23)
					{
						bandera = 'n';
						error_log ("Hora no cumple con el formato", linea);
					}
				}
				if (bandera != 'n')
				{
					minuto = 10*(linea[49] - 48);
					minuto += (linea[50] - 48);
					if (minuto > 59)
					{
						bandera = 'n';
						error_log ("Minuto no cumple con el formato", linea);
					}
				}

				if (bandera != 'n')
				{
					/***********************************************\ 
					* Obtiene la duracion.                          *
					\***********************************************/
					atoi_duracion = 0;
					if (linea[52] != ' ')
					{
						atoi_duracion += ((linea[52] - 48) * 36000);
					}
					if (linea[53] != ' ')
					{
						atoi_duracion += ((linea[53] - 48) * 3600);
					}
					if (linea[55] != ' ')
					{
						atoi_duracion += ((linea[55] - 48) * 600);
					}
					if (linea[56] != ' ')
					{
						atoi_duracion += ((linea[56] - 48) * 60);
					}
					if (linea[58] != ' ')
					{
						atoi_duracion += ((linea[58] - 48) * 10);
					}
					if (linea[59] != ' ')
					{
						atoi_duracion += (linea[59] - 48);
					}

					/****************************************\
					* Chequea si la llamada tiene duracion 0 *
					\****************************************/
					if (atoi_duracion == 0)
					{
						bandera = 'n';
					}
					else
					{
						sprintf(duracion, "%d", atoi_duracion);
					}
/*					
					for (i=0; i<4; i++)
					{
						duracion[i] = linea[12+i];
					}
					duracion[i] = '\0';
*/
				}

				if (bandera != 'n')
				{
					/************************************************\ 
					* Completa el resto de la fecha.                 *
					\************************************************/
					fecha[6] = linea[46];
					fecha[7] = linea[47];
					fecha[8] = linea[49];
					fecha[9] = linea[50];
					fecha[10] = '\0';

					/***********************************************\ 
					* Obtiene el numero llamado.                    *
					\***********************************************/
					j=0;
					for (i=0; i<16; i++)
					{
						if ((linea[24+i] > 47) && (linea[24+i] < 58))
						{
							llamado[j] = linea[24+i];
							j++;
						}
					}
					llamado[j] = '\0';
					if (strcmp(llamado, "") == 0)
					{
						strcpy(llamado, "0");
					}

					/************************************************\ 
					* Obtiene el codigo personal.                    *
					\************************************************/
					j=0;
					for (i=0; i<4; i++)
					{
						if (linea[41+i] != ' ')
						{
							codigo[j] = linea[41+i];
							j++;
						}
					}
					codigo[j] = '\0';
					if (strcmp(codigo, "") == 0)
					{
						strcpy(codigo, "0");
					}

					strcpy(pulsos,"0");

					/*******************************************\ 
					* Etapa final: Arma la cadena que enviara a *
					* otro proceso, y reinicializa variables.   *
					\*******************************************/ 

/*
					sprintf(cadena, "codigo: %s, llamante: %s, llamado: %s, fecha: %s, pulsos: %s, duracion: %s", codigo, llamante, llamado, fecha, pulsos, duracion);
					printf ("----------\n%s\n", cadena);
*/
					sprintf(cadena, "%s -c %s -A %s -B %s -f %s -p %s -d %s &", DBUP, codigo, llamante, llamado, fecha, pulsos, duracion);
					printf ("%s\n", cadena);
					system(cadena);
					strcpy(codigo, "");
					strcpy(llamante, "");
					strcpy(llamado, "");
/*
					strcpy(fecha, "");
*/
					strcpy(pulsos, "");
					strcpy(duracion, "");
				}
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

/*
				if ((c_ant_5 == LF) && (c_ant_4 == 0) && (c_ant_3 == 0) && (c_ant_2 == 0) && (c_ant_1 == 0) && (c == LF))
*/
				if ((c_ant_4 == LF) && (c_ant_3 == 0) && (c_ant_2 == 0) && (c_ant_1 == 0) && (c == 0))
				{
					linea_enviada[poschar+1] = '\0';
					if (write(parser[1], linea_enviada, MAXLINE) == -1)
					{
						perror ("write parser");
					}
					poschar = -1;
				}

				c_ant_5 = c_ant_4;
				c_ant_4 = c_ant_3;
				c_ant_3 = c_ant_2;
				c_ant_2 = c_ant_1;
				c_ant_1 = c;
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
/*
		printf ("Logfile: %s\n", logfile);
		printf ("Device: %s\n", device);
*/

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
*                                                                      *
* En particular, los parametros de las Centrales Siemmens son:         *
* 4800, 7, E, 2                                                        *
\**********************************************************************/
			newtio.c_cflag = bps(baudrate) | CS7 | CSTOPB | PARENB | CLOCAL | CREAD;
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
