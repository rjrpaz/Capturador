/**********************************************************\
* Este programa emula el comportamiento de una central NEC *
\**********************************************************/

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <mysql/mysql.h>

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS0"

#define FALSE 0
#define TRUE 1

#define MAX_SEGUNDOS 600.0

#define PORCENTAJE_CELULAR 33
#define PORCENTAJE_BASICA 100
#define PORCENTAJE_INTERURBANA 24
#define PORCENTAJE_URBANA 100

#define HOST "0"
#define DATABASE "Central"
#define USUARIO "nobody"
#define PASSWORD "nobody"

MYSQL mysql;
MYSQL_RES *res;
MYSQL_ROW row;

void exiterr(int exitcode)
{
	fprintf( stderr, "%s\n", mysql_error(&mysql) );
	exit( exitcode );
}


char * destino(char * tipo_llamada, char * lugar_destino)
{
	char host[256], username[256], password[256], database[256], query[256];
	char resultado[256];
	char * resul;
	int i, j, k;

	strcpy(query, "SELECT ");
	if ((strcmp(lugar_destino, "urbana")) == 0)
	{
		strcat(query, "SUBSTRING(bloque, 4) FROM NGA WHERE localidad_ID = 702 ");
	}
	else
	{
		strcat(query, "bloque FROM NGA WHERE localidad_ID != 702 ");
	}
	strcat(query, "AND servicio ");

	if ((strcmp(tipo_llamada, "celular")) == 0)
	{
		strcat(query, "LIKE 'STM' ");
	}
	else
	{
		strcat(query, "LIKE 'SBT' ");
	}
	strcat(query, "ORDER BY RAND() LIMIT 1");

	if (!(mysql_connect(&mysql,HOST,USUARIO,PASSWORD)))
		exiterr(1);

	if (mysql_select_db(&mysql,DATABASE))
		exiterr(2);

	if (mysql_query(&mysql, query))
		exiterr(3);

	if (!(res = mysql_store_result(&mysql)))
		exiterr(4);

	row = mysql_fetch_row(res);

	mysql_free_result(res);
	mysql_close(&mysql);

	if ((strcmp(lugar_destino, "urbana")) == 0)
	{
		if ((strcmp(tipo_llamada, "celular")) == 0)
		{
			j = 9 - strlen(row[0]);
		}
		else
		{
			j = 7 - strlen(row[0]);
		}
	}
	else
	{
		if ((strcmp(tipo_llamada, "celular")) == 0)
		{
			j = 12 - strlen(row[0]);
		}
		else
		{
			j = 10 - strlen(row[0]);
		}
	}
	k = pow(10, j) - 1;
//	printf ("LD: %s, TL: %s, J: %d \n", lugar_destino, tipo_llamada, k);
	i = (int) (((float) k)*rand()/(RAND_MAX+1.0));

	if (j == 4)
	{
		if ((strcmp(lugar_destino, "interurbana")) == 0)
		{
			sprintf (resultado, "0%s%04d", row[0], i);
		}
		else
		{
			sprintf (resultado, "%s%04d", row[0], i);
		}
	}
	else if (j == 3)
	{
		if ((strcmp(lugar_destino, "interurbana")) == 0)
		{
			sprintf (resultado, "0%s%03d", row[0], i);
		}
		else
		{
			sprintf (resultado, "%s%03d", row[0], i);
		}
	}
	else if (j == 2)
	{
		if ((strcmp(lugar_destino, "interurbana")) == 0)
		{
			sprintf (resultado, "0%s%02d", row[0], i);
		}
		else
		{
			sprintf (resultado, "%s%02d", row[0], i);
		}
	}
	else
	{
		if ((strcmp(lugar_destino, "interurbana")) == 0)
		{
			sprintf (resultado, "0%s%d", row[0], i);
		}
		else
		{
			sprintf (resultado, "%s%d", row[0], i);
		}
	}

//	printf ("%s \n", resultado);

	resul = resultado;

	return (resul);
}



int main(int argc, char *argv[])
{
	int fd, c, i, j, contador=0, cantidad_segundos;
	struct termios oldtio,newtio;
	char prueba[256], tipo_llamada[20], lugar_destino[20], nro_destino[32];
	struct tm *tm_ptr;
	time_t tiempo, tiempo_comienzo;

	if (argc != 2)
	{
		printf("Uso:\n\t%s <archivo_dat>\n\n", argv[0]);
		exit(1);
	}
	 
	/***********************************************************\
	* Abre la terminal para lectura/escritura. El flag O_NOCTTY *
	* especifica que no acepte caracteres de control para que   *
	* programa no acabe si se envia CTRL-C.                     *
	\************************************************************/

	fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY);
	if (fd <0) 
	{
		perror(MODEMDEVICE); 
		exit(-1); 
	}

	/***********************************************\
	* guarda la configuracion actual de la terminal *
	\***********************************************/
	tcgetattr(fd,&oldtio);
 
	/***********************\
	* configura la terminal *
	\***********************/
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IXON | IXOFF | IGNBRK | ISTRIP | IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;
 
	newtio.c_cc[VMIN]=1;
	newtio.c_cc[VTIME]=0;
 
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);

	srand(time(0));
	while (TRUE)
	{
		/* STX - Start of Text */
		prueba[0] = 2;

		/* SA - Station Address */
		prueba[1] = '0';

		/* UA - Unit Address */
		prueba[2] = '!';

		/* K - Entry Index */
		prueba[3] = 'K';

		/* A - Type of Record */
		prueba[4] = 'A';

		/* Outgoing Trunk Information */
		prueba[5] = '0';
		prueba[6] = '0';
		prueba[7] = '1';
		prueba[8] = '0';

		i = 1+(int) (29.0*rand()/(RAND_MAX+1.0));
		j = i / 10;
		prueba[9] = j + 48;
		j = i % 10;
		prueba[10] = j + 48;

		/* Calling Party Information */
		prueba[11] = '0';
		prueba[12] = '0';
		prueba[13] = '1';

		i = 701+(int) (200.0*rand()/(RAND_MAX+1.0));
		j = i / 100;
		prueba[14] = j + 48;
		i = i % 100;
		j = i / 10;
		prueba[15] = j + 48;
		j = i % 10;
		prueba[16] = j + 48;

		prueba[17] = ' ';
		prueba[18] = ' ';
		prueba[19] = ' ';

		/* Time of Start Conversation */
		cantidad_segundos = 1+(int) (MAX_SEGUNDOS*rand()/(RAND_MAX+1.0));

		if (time(&tiempo) == (time_t)-1)
		{
			printf ("Error al intentar obtener fecha actual\n");
		}

		tiempo_comienzo = tiempo - cantidad_segundos;
		tm_ptr = localtime(&tiempo_comienzo);
		i = (tm_ptr->tm_mon) + 1;
		j = i / 10;
		prueba[20] = j + 48;
		j = i % 10;
		prueba[21] = j + 48;
		i = tm_ptr->tm_mday;
		j = i / 10;
		prueba[22] = j + 48;
		j = i % 10;
		prueba[23] = j + 48;
		i = tm_ptr->tm_hour;
		j = i / 10;
		prueba[24] = j + 48;
		j = i % 10;
		prueba[25] = j + 48;
		i = tm_ptr->tm_min;
		j = i / 10;
		prueba[26] = j + 48;
		j = i % 10;
		prueba[27] = j + 48;
		i = tm_ptr->tm_sec;
		j = i / 10;
		prueba[28] = j + 48;
		j = i % 10;
		prueba[29] = j + 48;

		/* Time of Call Completion */
		tm_ptr = localtime(&tiempo);
		i = (tm_ptr->tm_mon) + 1;
		j = i / 10;
		prueba[30] = j + 48;
		j = i % 10;
		prueba[31] = j + 48;
		i = tm_ptr->tm_mday;
		j = i / 10;
		prueba[32] = j + 48;
		j = i % 10;
		prueba[33] = j + 48;
		i = tm_ptr->tm_hour;
		j = i / 10;
		prueba[34] = j + 48;
		j = i % 10;
		prueba[35] = j + 48;
		i = tm_ptr->tm_min;
		j = i / 10;
		prueba[36] = j + 48;
		j = i % 10;
		prueba[37] = j + 48;
		i = tm_ptr->tm_sec;
		j = i / 10;
		prueba[38] = j + 48;
		j = i % 10;
		prueba[39] = j + 48;

		/* Account Code */
		i = (int) (9999.0*rand()/(RAND_MAX+1.0));
		j = i / 1000;
		prueba[40] = j + 48;
		i = i % 1000;
		j = i / 100;
		prueba[41] = j + 48;
		i = i % 100;
		j = i / 10;
		prueba[42] = j + 48;
		i = i % 10;
		prueba[43] = j + 48;

		/* Account Code  + 3 Space */
		i = 0;
		while (i<9)
		{
			prueba[44 + i] = ' ';
			i++;
		}

		/* Condition Code = 020 */
		prueba[53] = '0';
		prueba[54] = '2';
		prueba[55] = '0';

		/* Route Advance Information */
		prueba[56] = prueba[5];
		prueba[57] = prueba[6];
		prueba[58] = prueba[7];
		prueba[59] = prueba[5];
		prueba[60] = prueba[6];
		prueba[61] = prueba[7];

		/* Called Number */

		i = (int) (100.0*rand()/(RAND_MAX+1.0));
		if ((i >= 0) && (i <= PORCENTAJE_CELULAR))
		{
			sprintf(tipo_llamada, "%s", "celular");
		}
		else
		{
			sprintf(tipo_llamada, "%s", "basica");
		}

		i = (int) (100.0*rand()/(RAND_MAX+1.0));
		if ((i >= 0) && (i <= PORCENTAJE_INTERURBANA))
		{
			sprintf(lugar_destino, "%s", "interurbana");
		}
		else
		{
			sprintf(lugar_destino, "%s", "urbana");
		}
		strcpy(nro_destino, destino(tipo_llamada, lugar_destino));
//		printf("Nro_destino: %s \n", nro_destino);

		i = 0;
		while (i<strlen(nro_destino))
		{
			prueba[62 + i] = nro_destino[i];
			i++;
		}

		j = 0;
		while ((62 + i + j) <94)
		{
			prueba[62 + i + j] = ' ';
			j++;
		}

		/* Call Metering */
		i = 0;
		while (i<4)
		{
			prueba[94 + i] = '0';
			i++;
		}

		/* Space */
		i = 0;
		while (i<8)
		{
			prueba[98 + i] = ' ';
			i++;
		}

		/* Authorization Code */
		i = 0;
		while (i<10)
		{
			prueba[106 + i] = ' ';
			i++;
		}

		/* Year of Start Conversation */
		prueba[116] = ' ';
		prueba[117] = ' ';

		/* Year of Call Completion */
		prueba[118] = ' ';
		prueba[119] = ' ';

		/* Condition Code for Advice of Charge */
		prueba[120] = '0';

		/* Advice of Charge */
		i = 0;
		while (i<6)
		{
			prueba[121 + i] = ' ';
			i++;
		}

		/* Space */
		i = 0;
		while (i<4)
		{
			prueba[127 + i] = ' ';
			i++;
		}

		/* End of Text */
		prueba[131] = 3;

		prueba[132] = '\0';

		for (i=0; i<132; i++)
		{
			c = prueba[i];
			write(fd,&c,1);
		}

		sleep(1);
	}
	close(fd);

	/* restore old modem setings */
	tcsetattr(fd,TCSANOW,&oldtio);

	exit(0); 
}
