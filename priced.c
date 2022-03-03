#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <netdb.h>
#include <ctype.h>
#include <mysql/mysql.h>
#include "capturador.h"

static int port = 666;

#define CONOCIDOS 12

static char *numeros_conocidos[] = {"100", "101", "102", "103", "105", "107", "108", "110", "112", "113", "114", "121"};
/*
static char *servicios_conocidos[] = {"Bomberos", "Policia", "Ayuda al ni&ntilde;o", "Defensa Civil", "Emergencia Ambiental", "Emergencia M&eacute;dica", "Protecci&oacute;n Ciudadana", "Informaci&oacute;n de Guia", "Gesti&oacute;n Comercial Telecom", "Hora Oficial", "Reparaciones Telecom", "Servicio de Telelectura"};
*/
static int localidad_id_conocidos[] = {9999, 9998, 9997, 9996, 9995, 9994, 9993, 9992, 9991, 9990, 9989, 9988};
static char *claves_conocidas[] = {"99", "99", "99", "99", "99", "99", "99", "0", "99", "0", "99", "0"};

static char *normal[] = {"0.0469", "0.0469", "0.162", "0.180", "0.354", "0.408", "0.498", "0.498", "0.498", "0.498", "0.498", "0.498", "0.498"};
static char *reducida[] = {"0.0469", "0.0469", "0.126", "0.126", "0.228", "0.288", "0.348", "0.348", "0.348", "0.348", "0.348", "0.348", "0.348"};

#define TOTAL_FERIADOS 15
static char *feriados[] = {"0101", "0402", "0413", "0501", "0525", "0610", "0618", "0706", "0709", "0820", "0930", "1008", "1111", "1208", "1225"};

#define MINUTO_CELULAR 0.31
#define IVA 1.21

static MYSQL mysql;
static MYSQL_RES *res;
static MYSQL_ROW row;

static int obtener_localidad (MYSQL mysql, char *nro_llamado, localidad_reg *localidad);

static int dia_de_la_semana (char *fecha);

static float costo (char *clave, char *horario, char *duracion, char *tipo);

int main()
{
	struct sockaddr_in sin;
	struct sockaddr_in pin;
	int sock_descriptor;
	int temp_sock_descriptor;
	int address_size;
	char buf[BUFSIZ], buf_res[10]="OK", delim[1] ="|";
	/*@in@*/
	char *p;
	int resultado, command_lenght, nflds=0, i;
	char *flds[20], *command;
	localidad_reg var_localidad, *ptr_localidad;
	float valor;

	sock_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_descriptor == -1)
	{
		perror("error socket");
		exit(EXIT_FAILURE);
	}
	bzero(&sin, sizeof(sin));
/*
	memset(&sin, 48 ,sizeof(sin));
*/
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	if (bind(sock_descriptor, (struct sockaddr *)&sin, sizeof(sin)) == -1)
	{
		perror("error bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sock_descriptor, 20) == -1)
	{
		perror("error listen");
		exit(EXIT_FAILURE);
	}

	if ((ptr_localidad = malloc(sizeof(localidad_reg))) == NULL)
	{
		perror("Error en la reserva de memoria");
		exit(EXIT_FAILURE);
	}

	while(TRUE)
	{
		temp_sock_descriptor = accept(sock_descriptor, (struct sockaddr *)&pin, &address_size);
		if (temp_sock_descriptor == -1)
		{
			perror("error accept");
			exit(EXIT_FAILURE);
		}

		for (i=0; i<BUFSIZ; i++)
		{
			buf[i] = '\0';
		}

		if (recv(temp_sock_descriptor, buf, BUFSIZ, 0) == -1)
		{
			perror("call to recv");
			exit(EXIT_FAILURE);
		}
		printf("---------------------\nString Recibido: %s\n", buf);

		p = buf;
		nflds = 0;
		while (TRUE)
		{
			flds[nflds] = strtok(p, delim);

			if (flds[nflds] == NULL)
			{ /* no more tokens */
				break;
			}

			printf("Desglose del string nro. %d: %s\n", nflds, flds[nflds]);
			/* we have another token */
			nflds++;		/* increment number of tokens */
			p = NULL;	/* remaining times, pass NULL */
		}

		/* Elimina el CR del final de la linea */
/*
		flds[nflds-1][strlen(flds[nflds-1])-1] = '\0';
*/
		mysql_init(&mysql);
		if (!mysql_real_connect(&mysql, 0, flds[1], flds[2], DBBASE, 0, NULL, 0))
		{
			perror("Error al intentar conectarse a la Base de Datos");
		}

		ptr_localidad = &var_localidad;
		resultado = obtener_localidad(mysql, flds[5], ptr_localidad);
		if (resultado != 1)
		{
			var_localidad.localidad_id = 1;
			strcpy(var_localidad.servicio, "NULL");
			var_localidad.prestador_id = 1;
			valor = 0.0;
		}
		else
		{
			valor = costo(var_localidad.clave, flds[6], flds[8], var_localidad.servicio);
		}

		command_lenght = strlen(INSERT_DATA) + strlen(flds[0]) + strlen(flds[3]) + strlen(flds[4]) + strlen(flds[5]) + strlen(flds[6]) + strlen(flds[7]) + strlen(flds[8]) + sizeof(var_localidad.localidad_id) + sizeof(var_localidad.servicio) + sizeof(var_localidad.prestador_id) + sizeof(valor);
		command = (char *) malloc(command_lenght*sizeof(char));

		sprintf(command, INSERT_DATA, flds[0], flds[3], flds[4], flds[5], flds[6], flds[7], flds[8], var_localidad.servicio, var_localidad.prestador_id, var_localidad.localidad_id, valor);

		printf("Comando: %s\n", command);
		resultado = mysql_query(&mysql, command);

		if (resultado != 0)
		{
			puts("Hubo un error al intentar actualizar la base de datos");
			printf("Error: %s\n", mysql_error(&mysql));
			strcpy(buf_res, "ERROR");
		}

		free(command);

		mysql_close(&mysql);

		if (send(temp_sock_descriptor, buf_res, strlen(buf_res), 0) == -1)
		{
			perror("call to send");
			exit(EXIT_FAILURE);
		}

		close(temp_sock_descriptor);
	}
	free(ptr_localidad);
}



int obtener_localidad (MYSQL mysql, char *nro_llamado, localidad_reg *localidad)
{
	char *command, nro_completo[256], nro_parcial[256];
	unsigned int con=0, largo_numero;

	localidad->localidad_id = 1;
	strcpy(localidad->servicio, "");
	localidad->prestador_id = 1;
	strcpy(localidad->clave, "");

	while (con<CONOCIDOS)
	{
		if (strcmp(numeros_conocidos[con], nro_llamado) == 0)
		{
			localidad->localidad_id = localidad_id_conocidos[con];
			/* Telecom */
			localidad->prestador_id = 323;
			strcpy(localidad->clave, claves_conocidas[con]);
			return (1);
		}
		con++;
	}
	if (strncmp(nro_llamado, "00", 2) == 0)
	{
		localidad->localidad_id = 9900;
		strcpy(localidad->clave, "98");
		return (1);
	}
	else if (strncmp(nro_llamado, "0800", 4) == 0)
	{
		localidad->localidad_id = 9901;
		strcpy(localidad->clave, "99");
		return (1);
	}
	else if (strncmp(nro_llamado, "0810", 4) == 0)
	{
		localidad->localidad_id = 9902;
		strcpy(localidad->clave, "0");
		return (1);
	}
	else if (strncmp(nro_llamado, "0610", 4) == 0)
	{
		localidad->localidad_id = 9903;
		strcpy(localidad->clave, "96");
		return (1);
	}
	else
	{
		largo_numero =  strlen(nro_llamado);
	
		/**********************************************************\ 
		* Si es un numero de Cordoba, le agrego la caracteristica  *
		* correspondiente                                          * 
		\**********************************************************/
		if ((largo_numero == 7) || ((largo_numero == 9) && ((strncmp(nro_llamado, "15", 2)) == 0)))
		{
			sprintf(nro_completo, "351%s", nro_llamado);
			largo_numero +=  3;
		}
		else
		{
			for (con=1; con<strlen(nro_llamado); con++)
			{
				nro_completo[con-1] = nro_llamado[con];
			}
			nro_completo[strlen(nro_llamado)-1] = '\0';
			largo_numero--;
		}

		/* El bloque de numeros asignado mas chico es de 100 numeros */
		largo_numero -=  2;

		command = malloc (strlen (OBTENER_LOCALIDAD) + strlen(nro_completo));
		/* La referencia mas chica a un bloque, segun la tabla, es de
			6 numeros */
		while (largo_numero >  5)
		{
			strncpy(nro_parcial, nro_completo, largo_numero);
			nro_parcial[largo_numero] = '\0';
			sprintf(command, OBTENER_LOCALIDAD, nro_parcial);
/*
			printf("OBTENER_LOCALIDAD: %s\n", command);
*/
			con = mysql_query(&mysql, command);
			if (con != 0)
			{
				printf("Error en el query: %s\n", mysql_error(&mysql));
			}

			res = mysql_store_result(&mysql);
			if ((mysql_num_rows(res)) == 1)
			{
				row = mysql_fetch_row(res);
				localidad->localidad_id =  atoi(row[0]);
				strcpy(localidad->servicio, "'");
				strcat(localidad->servicio, row[1]);
				strcat(localidad->servicio, "'");
				localidad->prestador_id =  atoi(row[2]);
				free (command);
				command = malloc (strlen (OBTENER_CLAVE) + strlen(nro_parcial));
				while (largo_numero > 1)
				{
					strncpy(nro_parcial, nro_completo, largo_numero);
					nro_parcial[largo_numero] = '\0';
					sprintf(command, OBTENER_CLAVE, nro_parcial);
/*
					printf("OBTENER_CLAVE: %s\n", command);
*/
					con = mysql_query(&mysql, command);
					if (con != 0)
					{
						printf("Error en el query: %s\n", mysql_error(&mysql));
					}
					res = mysql_store_result(&mysql);
					if ((mysql_num_rows(res)) == 1)
					{
						row = mysql_fetch_row(res);
						strcpy(localidad->clave, row[0]);
						free (command);
						return (1);
					}
					largo_numero--;
				}
			}
			largo_numero--;
		}
		free (command);
		return (0);
	}
	return (0);
}


int dia_de_la_semana (char *fecha)
{
	struct tm *tm_ptr;
	time_t *tiempo;
	char mes_dia[4];
	int i, weekday;

	sprintf(mes_dia, "%c%c%c%c", fecha[2], fecha[3], fecha[4], fecha[5]);
	
	for (i=0; i<TOTAL_FERIADOS; i++)
	{
		if (strcmp(mes_dia, feriados[i]) == 0)
		{
			/* Trata los feriados como los domingos */
			return(0);
		}
	}

	tm_ptr = malloc(sizeof(struct tm));
	tm_ptr->tm_sec = 0;
	tm_ptr->tm_min = 0;
	tm_ptr->tm_hour = 0;
	tm_ptr->tm_mday = (fecha[4] - 48)*10+(fecha[5] - 48);
	tm_ptr->tm_mon = (fecha[2] - 48)*10+(fecha[3] - 48) - 1;
	tm_ptr->tm_year = (fecha[0] - 48)*10+(fecha[1] - 48) + 100;
	tm_ptr->tm_wday = 0;
	tm_ptr->tm_yday = 0;
	tm_ptr->tm_isdst = 0;

	tiempo = (time_t *) malloc (sizeof (time_t));
	if ((*tiempo = mktime(tm_ptr)) == (time_t)-1)
	{
		perror("mktime");
	}
	weekday = tm_ptr->tm_wday;

	free (tm_ptr);
	free (tiempo);
	return (weekday);

}


float costo (char *clave, char *horario, char *duracion, char *tipo)
{
	static char fecha[7], hora[5];
	static int i, weekday, hora_int, clave_int;
	static float cantidad=0.0;

	if (strcmp(clave, "99") == 0)
	{
		return(0.0);
	}
	else if ((strcmp(clave, "98") == 0) || (strcmp(clave, "") == 0))
	{
		return(0.0);
	}
	
	strncpy(fecha, horario, 6);
	fecha[6] = '\0';

	for (i=6; i<10; i++)
	{
		hora[i-6] = horario[i];
	}
	hora[4] = '\0';
	hora_int = atoi(hora);

	clave_int = atoi(clave);

	weekday = dia_de_la_semana(fecha);

	if ((clave_int == 0) || (clave_int == 1))
	{
		/* Lunes a Viernes */
		if ((weekday>0) && (weekday<6))
		{
			if ((hora_int >= 800) && (hora_int <= 2000))
			{
				cantidad = ceil(atof(duracion) / 120.0);
			}
			else
			{
				cantidad = ceil(atof(duracion) / 240.0);
			}
		}
		/* Sabados */
		else if (weekday == 6)
		{
			if ((hora_int >= 800) && (hora_int <= 1300))
			{
				cantidad = ceil(atof(duracion) / 120.0);
			}
			else
			{
				cantidad = ceil(atof(duracion) / 240.0);
			}
		}
		/* Domingos y Feriados */
		else
		{
			cantidad = ceil(atof(duracion) / 240.0);
		}

		cantidad *= atof(normal[0]);
	}
	else if (clave_int > 0)
	{
		/* Lunes a Viernes */
		if ((weekday>0) && (weekday<6))
		{
			if ((hora_int >= 800) && (hora_int <= 2100))
			{
				cantidad = atof(duracion) * atof(normal[clave_int]);
			}
			else
			{
				cantidad = atof(duracion) * atof(reducida[clave_int]);
			}
		}
		/* Sabados */
		else if (weekday == 6)
		{
			if ((hora_int >= 800) && (hora_int <= 1300))
			{
				cantidad = atof(duracion) * atof(normal[clave_int]);
			}
			else
			{
				cantidad = atof(duracion) * atof(reducida[clave_int]);
			}
		}
		/* Domingos y Feriados */
		else
		{
			cantidad = atof(duracion) * atof(reducida[clave_int]);
		}

		cantidad /= 60.0;
	}


	/* Para llamadas a celulares, le agrego el costo del aire */
	if ((strcmp(tipo, "'STM'") == 0) || (strcmp(tipo, "'PCS'") == 0) || (strcmp(tipo,"'SRCE'") == 0) || (strcmp(tipo, "'SRMC'") == 0) || (strcmp(tipo, "'CPP'") == 0))
	{
		cantidad += MINUTO_CELULAR * ceil (atof(duracion) / 60.0);
	}


	/* IVA */
	cantidad = cantidad * IVA;

	return(cantidad);
}


