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

#define DBUP "/root/dbup_pizzurno"

int hardware, flowchar[2], parser[2], pid, pid_hijo, poschar=0;
char *logfile="/var/log/capturador.log", *device="/dev/ttyS0", *hardware_control="FALSE", *baudrate="9600", linea[MAXLINE], linea_enviada[MAXLINE];
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

			while ((read(parser[0],linea,MAXLINE)) > 0)
			{
				bandera = 'y';		
				/********************************************\ 
				* Chequea que aparezca "*" en la posicion 0. *
				\********************************************/
				if (linea[0] != 42)
				{
					bandera = 'n';
					error_log ("No aparece *", linea);
				}

				/********************************************\ 
				* Chequea que aparezca " " en la posicion 1. *
				\********************************************/
				if ((linea[1] != 32) && (bandera != 'n'))
				{
					bandera = 'n';
					error_log ("No aparece espacio", linea);
				}

				/******************************************\ 
				* Chequea que aparezca # en la posicion 2. *
				\******************************************/
				if ((linea[2] != 35) && (bandera != 'n'))
				{
					bandera = 'n';
					error_log ("No aparece #", linea);
				}

				/*************************************************\ 
				* Chequea que aparezca "01" en la posicion 3 y 4. *
				\*************************************************/
				if ((linea[3] != '0') && (bandera != 'n'))
				{
					bandera = 'n';
					error_log ("No es correcto el numero de unidad", linea);
				}
				if ((linea[4] != '1') && (bandera != 'n'))
				{
					bandera = 'n';
					error_log ("No es correcto el numero de unidad", linea);
				}

				/****************************************************\ 
				* Determina si el tipo de registro es de accounting  *
				* o no en la posicion 7.                             *
				\****************************************************/
				if (bandera != 'n')
				{
					/**************************\ 
					* El registro es estandar. *
					\**************************/
					if (linea[7] == '1')
					{
						bandera_standar = 'y';
						/************************************************\ 
						* Chequea que el string tenga el largo adecuado. *
						\************************************************/

						if (strlen(linea) != 104)
						{
							bandera_standar = 'n';
							error_log ("Error en el largo del registro estandar", linea);
						}


						/***************************************************\ 
						* Guarda el numero de referencia (posicion 5 y 6) y *
						* lo compara con el registro siguiente, para ver si *
						* mantiene la secuencia.                            *
						\***************************************************/
						if (bandera_standar != 'n')
						{
							ref_no_ant = 10 * (linea[5] - 48);
							ref_no_ant += (linea[6] - 48);

							if (ref_no_ant == 99)
							{
								ref_no_ant = -1;
							}
						}


						/*********************\ 
						* Determina la fecha. *
						\*********************/ 
						if (bandera_standar != 'n')
						{
							mes = 10*(linea[10] - 48);
							mes += (linea[11] - 48);
							if (mes > 12)
							{
								bandera_standar = 'n';
								error_log ("Mes no cumple con el formato", linea);
							}
						}
						if (bandera_standar != 'n')
						{
							dia = 10*(linea[12] - 48);
							dia += (linea[13] - 48);
							if (dia > 31)
							{
								bandera_standar = 'n';
								error_log ("Dia no cumple con el formato", linea);
							}
						}
						if (bandera_standar != 'n')
						{
							hora = 10*(linea[14] - 48);
							hora += (linea[15] - 48);
							if (hora > 23)
							{
								bandera_standar = 'n';
								error_log ("Hora no cumple con el formato", linea);
							}
						}
						if (bandera_standar != 'n')
						{
							minuto = 10*(linea[16] - 48);
							minuto += (linea[17] - 48);
							if (minuto > 59)
							{
								bandera_standar = 'n';
								error_log ("Minuto no cumple con el formato", linea);
							}
						}

						/*****************************************\ 
						* Determina el tipo de llamante, para ver *
						* si es un interno (1), o la operadora    *
						* (2).                                    *
						\*****************************************/ 
						if ((linea[82] != '1') && (linea[82] != '2') && (bandera_standar != 'n'))
						{
							bandera_standar = 'n';
						}

						/*****************************************\ 
						* Determina el tipo de llamado, para ver  *
						* si es una llamada hacia afuera (3).     *
						\*****************************************/ 
						if ((linea[83] != '3') && (bandera_standar != 'n'))
						{
							bandera_standar = 'n';
						}

						/*****************************************\ 
						* Determina si la llamada fue contestada. *
						\*****************************************/ 
						if ((linea[90] != '1') && (bandera_standar != 'n'))
						{
							bandera_standar = 'n';
						}

						/********************************************\ 
						* Finalmente obtiene los datos utiles que se *
						* encuentran en un registro estandar.        *
						\********************************************/ 
						if (bandera_standar != 'n')
						{
							for (i=0; i<10; i++)
							{
								fecha[i] = linea[8+i];
							}
							fecha[i] = '\0';
							j=0;
							for (i=0; i<32; i++)
							{
								if ((linea[18+i] != ' ') && (linea[18+i] != '#'))
								{
									codigo[j] = linea[18+i];
									j++;
								}
							}
							codigo[j] = '\0';
						}
					}
					/*******************************\ 
					* El registro es de accounting. *
					\*******************************/
					else if (linea[7] == '2')
					{
					 	bandera_accounting = 'y';
						if (bandera_standar != 'y')
						{
					 		bandera_accounting = 'n';
						}
						/************************************************\ 
						* Chequea que el string tenga el largo adecuado. *
						\************************************************/
						if ((strlen(linea) != 61) && (bandera_accounting != 'n'))
						{
					 		bandera_accounting = 'n';
							error_log ("Error en el largo del registro de accounting", linea);
						}


						/***************************************************\ 
						* Guarda el numero de referencia (posicion 5 y 6) y *
						* lo compara con el registro siguiente, para ver si *
						* mantiene la secuencia.                            *
						\***************************************************/
						if (bandera_accounting != 'n')
						{
							ref_no = 10 * (linea[5] - 48);
							ref_no += (linea[6] - 48);
							if (ref_no != (ref_no_ant + 1))
							{
								bandera_accounting = 'n';
								error_log ("Numero de referencia no respeta la secuencia", linea);
							}
						}

						if (bandera_accounting != 'n')
						{
							for (i=0; i<6; i++)
							{
								duracion[i] = linea[53+i];
							}
							duracion[i] = '\0';

							/****************************************\
							* Chequea si la llamada tiene duracion 0 *
							\****************************************/
							atoi_duracion = atoi(duracion);
							if (atoi_duracion == 0)
							{
								bandera_accounting = 'n';
							}
						}


						/********************************************\ 
						* Finalmente obtiene los datos utiles que se *
						* encuentran en un registro de accounting.   *
						\********************************************/ 
						if (bandera_accounting != 'n')
						{
							j=0;
							if ((linea[8] == '0') && (linea[9] == '4'))
							{
								for (i=0; i<19; i++)
								{
									if (linea[9+i] != ' ')
									{
										llamado[j] = linea[9+i];
										j++;
									}
								}
							}
							else
							{
								for (i=0; i<20; i++)
								{
									if (linea[8+i] != ' ')
									{
										llamado[j] = linea[8+i];
										j++;
									}
								}
							}
							llamado[j] = '\0';
						
							if (strlen(llamado) == 4)
							{
								bandera_accounting = 'n';
							}
	
							if (bandera_accounting != 'n')
							{
								for (i=0; i<6; i++)
								{
									pulsos[i] = linea[47+i];
								}
								pulsos[i] = '\0';

								/*******************************************\ 
								* Etapa final: Arma la cadena que enviara a *
								* otro proceso, y reinicializa variables.   *
								\*******************************************/ 
								strcpy(llamante, "0");
/*
								sprintf(cadena, "codigo: %s, llamante: %s, llamado: %s, fecha: %s, pulsos: %s, duracion: %s", codigo, llamante, llamado, fecha, pulsos, duracion);
								printf ("----------\n%s\n", cadena);
*/

								sprintf(cadena, "%s -c %s -A %s -B %s -f %s -p %s -d %s &", DBUP, codigo, llamante, llamado, fecha, pulsos, duracion);

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
							strcpy(duracion, "");
						}
					}
					else
					{
						error_log ("Registro de tipo desconocido", linea);
						registro = '?';
						bandera = 'n';
					}
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

				if ((c_ant == CR) && (c == LF))
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
