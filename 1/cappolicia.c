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

#define DBUP "/root/dbup_policia"

int hardware, flowchar[2], parser[2], pid, pid_hijo, poschar=0;
int tarifar_saliente = FALSE;
int tarifar_entrante = FALSE;
int tarifar_internas = FALSE;
int opcion_tarifar_entrante = FALSE;
int opcion_tarifar_internas = FALSE;
char *logfile="/var/log/capturador.log", *device="/dev/ttyS0", *hardware_control="FALSE", *baudrate="9600", linea[MAXLINE], linea_enviada[MAXLINE], linea_control[MAXLINE];
char bandera, c, c_ant_1, c_ant_2, c_ant_3, c_ant_4, registro, llamante[32], llamado[32], codigo[10], fecha[10], cadena[256], partyA[33], partyB[32], pulsos[6], anio[4], duracion[6], comentario[256];
int mes, dia, hora, minuto, i, j, k, l, atoi_duracion;
char * ptrchar;
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
	puts("\t-x Habilita control de flujo por hardware");
	puts("\t-e Tarifar llamadas entrantes");
	puts("\t-i Tarifar llamadas entre internos");
	puts("\t-h Presenta este menu de ayuda\n");
}


void procesar_opciones(int argc, char *argv[])
{
	while ((c = getopt(argc, argv, "l:b:d:xeih")) != EOF)
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
			case 'e':
				opcion_tarifar_entrante = TRUE;
				break;
			case 'i':
				opcion_tarifar_internas = TRUE;
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

	sprintf(linea_control, "%c%c%s", CR, LF, "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ");

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

			while ((read(parser[0],linea,MAXLINE)) > 0)
			{
				bandera = 'y';
				tarifar_saliente = FALSE;
				tarifar_entrante = FALSE;
				tarifar_internas = FALSE;
				/************************************************\ 
				* En el caso de que sea el string de control,    *
				* elimina la marca y deja la linea lista para    *
				* procesar.                                      *
				\************************************************/
				if (strncmp(linea, linea_control, strlen(linea_control)) == 0)
				{
					i = 0;
					while ((linea[83+i]) != CR)
					{
						linea[i] = linea[83+i];
						i++;
					}
					linea[i] = CR;
					linea[i+1] = LF;
					linea[i+2] = '\0';
				}

				/************************************************\ 
				* Chequea que el string tenga el largo adecuado. *
				\************************************************/
				if (bandera != 'n')
				{
					if (strlen(linea) != 123)
					{
						bandera = 'n';
						error_log ("Error en el largo del registro", linea);
					}
				}

				/***************************************************\
				* Chequea que sea una llamada a un nro. externo     *
				* Para ello se fija en el condition code:           *
				* SALIENTES:                                        *
				* 	 ( )  : Llamada saliente                    *
				* 	 (DO) : Llamada saliente de gran duracion   *
				* 	 (DX) : External follow me de gran duracion *
				* 	 (VO) : Outgoing Data Call                  *
				* 	 (L)  : Conference Call                     *
				* 	 (T)  : Transference Call                   *
				* 	 (X)  : External Follow Me                  *
				*	 (M)  : Least cost routing (Incluye las     *
				*               llamadas a los numeros gratuitos de *
				*               Internet)                           *
				* 	 (A)  : Llamada por operador (si tiene      *
				*               informacion de pulso                *
				* 	                                            *
				* ENTRANTES:                                        *
				* 	 (I)  : Llamada entrante                    *
				* 	 (DI) : Llamadas entrante de gran duracion  *
				* 	 (T)  : Transference Call                   *
				* 	 (VI) : Incoming Data Call                  *
				* 	 (NI) : Incoming DID Call when the          *
				*               answering party was not the dialed  *
				*               party                               *
				* 	 (NC) : (?). No figura en el manual de la   *
				* 	        central.                            *
				* 	 (CI) : Abandoned nos-DISA call             *
				* 	                                            *
				* INTERNOS:                                         *
				* 	 (J)  : Llamada interna                     *
				* 	 (DJ) : Llamadas interna de gran duracion   *
				* 	 (VJ) : Internal Data Call                  *
				* 	 (NJ) : Dialed Party is not the answering   *
				*               party on an internal call           *
				* 	 (A)  : Llamada por operador (si no tiene   *
				*               informacion de pulso                *
				* 	 (R)  : Intrusion                           *
				\***************************************************/
				
				if ((linea[23] == ' ') || ((linea[23] == 'D') && (linea[24] == 'O')) || ((linea[23] == 'D') && (linea[24] == 'X')) || ((linea[23] == 'V') && (linea[24] == 'O')) || (linea[23] == 'L') || (linea[23] == 'X') || (linea[23] == 'M'))
				{
					tarifar_saliente = TRUE;
				}
				else if ((linea[23] == 'I') || ((linea[23] == 'D') && (linea[24] == 'I')) || ((linea[23] == 'V') && (linea[24] == 'I')) || ((linea[23] == 'N') && (linea[24] == 'I')) || ((linea[23] == 'N') && (linea[24] == 'C')) || ((linea[23] == 'C') && (linea[24] == 'I')))
				{
					if (opcion_tarifar_entrante == TRUE)
					{
						tarifar_entrante = TRUE;
					}
				}
				else if ((linea[23] == 'J') || ((linea[23] == 'D') && (linea[24] == 'J')) || ((linea[23] == 'V') && (linea[24] == 'J')) || ((linea[23] == 'N') && (linea[24] == 'J')) || (linea[23] == 'R'))
				{
					if (opcion_tarifar_internas == TRUE)
					{
						tarifar_internas = TRUE;
					}
				}
				else if ((linea[23] == 'A') || (linea[23] == 'T'))
				{
					/***********************************************\ 
					* Obtiene los pulsos para ver si es entrante    *
					* o saliente.                                   *
					\***********************************************/
					j=0;
					for (i=0; i<4; i++)
					{
						if (linea[18+i] != ' ')
						{
							pulsos[j] = linea[18+i];
							j++;
						}
					}
					pulsos[j] = '\0';
					if (strcmp(pulsos, "") == 0)
					{
						if (opcion_tarifar_entrante == TRUE)
						{
							tarifar_entrante = TRUE;
						}
					}
					else
					{
						tarifar_saliente = TRUE;
					}
				}
				else
				{
					error_log ("Tipo de llamada no contemplada por el capturador", linea);
					bandera = 'n';
				}

				/**********************************************\ 
				* Chequea el formato de la fecha.              *
				\**********************************************/
				if (bandera != 'n')
				{
					mes = 10*(linea[3] - 48);
					mes += (linea[4] - 48);
					if (mes > 12)
					{
						bandera = 'n';
						error_log ("Mes no cumple con el formato", linea);
					}
				}
				if (bandera != 'n')
				{
					dia = 10*(linea[5] - 48);
					dia += (linea[6] - 48);
					if (dia > 31)
					{
						bandera = 'n';
						error_log ("Dia no cumple con el formato", linea);
					}
				}
				if (bandera != 'n')
				{
					hora = 10*(linea[7] - 48);
					hora += (linea[8] - 48);
					if (hora > 23)
					{
						bandera = 'n';
						error_log ("Hora no cumple con el formato", linea);
					}
				}
				if (bandera != 'n')
				{
					minuto = 10*(linea[9] - 48);
					minuto += (linea[10] - 48);
					if (minuto > 59)
					{
						bandera = 'n';
						error_log ("Minuto no cumple con el formato", linea);
					}
				}

				/***********************************************\ 
				* Obtiene la duracion.                          *
				\***********************************************/
				if (bandera != 'n')
				{
					for (i=0; i<3; i++)
					{
						duracion[i] = linea[12+i];
					}
					duracion[i] = '\0';
					atoi_duracion = (atoi(duracion) * 60) + (6 * (linea[15] - 48));

					/****************************************\
					* Chequea si la llamada tiene duracion 0 *
					\****************************************/
					if (atoi_duracion == 0)
					{
						bandera = 'n';
					}
				}

				/***********************************************\ 
				* Obtiene el numero llamado.                    *
				\***********************************************/
				if (bandera != 'n')
				{
					j=0;
					for (i=0; i<17; i++)
					{
						if ((linea[40+i] > 47) && (linea[40+i] < 58))
						{
							llamado[j] = linea[40+i];
							j++;
						}
					}
					llamado[j] = '\0';

					if (tarifar_saliente)
					{
						/**********************************************************\
						* En algunos casos, cuando la llamada es saliente, pone la *
						* caracteristica de Cordoba en el numero.                  *
						\**********************************************************/
						if ((strncmp(llamado,"0351",4)) == 0)
						{
							for (i=4;i<strlen(llamado); i++)
							{
								llamado[i-4] = llamado[i];
							}
							llamado[i-4] = '\0';
						}
					}
					
					if (strcmp(llamado, "") == 0)
					{
						strcpy(llamado, "0");
					}
				}

				/***********************************************\ 
				* Obtiene el numero llamante.                   *
				\***********************************************/
				if (bandera != 'n')
				{
					k = 58;
					l = 17;
					strcpy (comentario, "\"");
					/*******************************************************************\
					* Si el nro. llamado es el 101, y el llamante empieza con 1, 4 o 5, *
					* entonces hay que sacar el 1er. digito del numero. Esto tambien    *
					* ocurre con las llamadas entrantes tipo "T"                        *
					*                                                                   *
					* El significado de ese digito es:                                  *
					* 1 - Telefonia Comun.                                              *
					* 4 - Telefonia Publica.                                            *
					* 5 - Asistida por Operadora.                                       *
					\*******************************************************************/
					if ((((strcmp(llamado, "0101")) == 0) && ((linea[58] == '1') || (linea[58] == '4') || (linea[58] == '5'))) || ((linea[23] == 'T') && (tarifar_entrante == TRUE)))
					{
						k = 59;
						l = 16;
						if (linea[58] == '1')
						{
							strcat (comentario, "Telefonia comun");
						}
						else if (linea[58] == '4')
						{
							strcat (comentario, "Telefonia publica");
						}
						else if (linea[58] == '5')
						{
							strcat (comentario, "Servicio de Operadora");
						}
					}
					strcat (comentario, "\"");

					j=0;
					for (i=0; i<l; i++)
					{
						if ((linea[k+i] > 47) && (linea[k+i] < 68))
						{
							llamante[j] = linea[k+i];
							j++;
						}
					}
					llamante[j] = '\0';

					if (tarifar_entrante || tarifar_saliente)
					{
						/******************************************************************\
						* En el caso de que las llamadas son entrantes, el numero llamante *
						* aparece en el formato de 10 digitos definido en CNC.             *
						\******************************************************************/
						if ((strlen(llamante)) == 10)
						{
							/*******************************************************\
							* En el caso de que la llamada es local, elimina el 351 *
							\*******************************************************/
							if ((strncmp(llamante, "351", 3)) == 0)
							{
								for (i=3;i<strlen(llamante); i++)
								{
									llamante[i-3] = llamante[i];
								}
								llamante[i-3] = '\0';
							}
							/*******************************************************\
							* Los que comienzan con 70 no los toco hasta no definir *
							* que son.                                              *
							\*******************************************************/
							else if ((strncmp(llamante, "70", 2)) != 0) 
							{
								/***************************************************\
								* En el caso de que incluyan el caracter '?' no los *
								* modifico.                                         *
								\***************************************************/
								ptrchar = strchr(llamante,'?');
								if (ptrchar == NULL)
								{
									llamante[(strlen(llamante))+1] = '\0';
									for (i=strlen(llamante);i>=0; i--)
									{
										llamante[i+1] = llamante[i];
									}
									llamante[0] = '0';
								}
							}
						}
					}

					if (strcmp(llamante, "") == 0)
					{
						strcpy(llamante, "0");
					}
				}

				/***********************************************\
				* Si el condition code es L o T, se fija si el  *
				* numero llamado es de afuera                   *
				\***********************************************/
				/*
				if (((linea[23] == 'L') || (linea[23] == 'T')) && (strlen(llamado) < 6))
				{
					bandera = 'n';
				}
				*/

				/******************************\
				* Calcula la fecha prop. dicha *
				\******************************/
				if (time(&tiempo_actual) == (time_t)-1)
				{
					error_log ("Error al intentar obtener fecha actual", linea);
				}

				tm_ptr = localtime(&tiempo_actual);
				sprintf(anio, "%d", (tm_ptr->tm_year) + 1900);
				fecha[0] = anio[2];
				fecha[1] = anio[3];
				for (i=2; i<10; i++)
				{
					fecha[i] = linea[1+i];
				}
				fecha[i] = '\0';

				if (bandera != 'n')
				{
					/*******************************************\ 
					* Etapa final: Arma la cadena que enviara a *
					* otro proceso, y reinicializa variables.   *
					\*******************************************/ 
					strcpy(codigo, "0");

					sprintf(cadena, "%s", DBUP);
					if (tarifar_entrante == TRUE)
					{
						sprintf(cadena, "%s -t entrante -c %s -A %s -B %s -f %s -d %i -C %s &", cadena, codigo, llamante, llamado, fecha, atoi_duracion, comentario);
/*
						printf ("%s\n", cadena);
*/
						system(cadena);
					}
					else if (tarifar_saliente == TRUE)
					{
						sprintf(cadena, "%s -t saliente -c %s -A %s -B %s -f %s -d %i &", cadena, codigo, llamante, llamado, fecha, atoi_duracion);
/*
						printf ("%s\n", cadena);
*/
						system(cadena);
					}
					else if (tarifar_internas == TRUE)
					{
						sprintf(cadena, "%s -t interna -c %s -A %s -B %s -f %s -d %i &", cadena, codigo, llamante, llamado, fecha, atoi_duracion);
/*
						printf ("%s\n", cadena);
*/
						system(cadena);
					}
					strcpy(codigo, "");
					strcpy(llamante, "");
					strcpy(llamado, "");
					strcpy(fecha, "");
					strcpy(duracion, "");
					strcpy(comentario, "");
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

				if ((c_ant_4 == CR) && (c_ant_3 == LF) && (c_ant_2 == 0) && (c_ant_1 == 0) && (c == 0))
				{
					linea_enviada[poschar+1] = '\0';
					if (write(parser[1], linea_enviada, MAXLINE) == -1)
					{
						perror ("write parser");
					}
					poschar = -1;
				}

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
