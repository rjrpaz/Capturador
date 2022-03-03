/************************************************\
*  Gerencia de Telecomunicaciones y Teleprocesos *
\************************************************/

#include <stdlib.h>
#include <malloc.h>
#include <mysql/mysql.h>


#define CR 13
#define LF 10
#define MAXLINE 4096
#define DBHOST "10.250.1.254"
#define DBBASE "Central"

#define TRUE 1
#define FALSE 0

#define LOGERROR "/var/log/capturador_error.log"

/*
#define INSERT_DATA "INSERT INTO %s (personal_id, llamante, llamado, fecha, pulsos, duracion, servicio, prestador_id, localidad_id, costo) VALUES (%d, '%s', '%s', '%s', %d, %d, '%s', %d, %d, %.3f)"
*/

#define INSERT_DATA "INSERT INTO %s (personal_id, llamante, llamado, fecha, pulsos, duracion, servicio, prestador_id, localidad_id, costo) VALUES (%s, '%s', '%s', '%s', %s, %s, %s, %u, %u, %.3f)"

#define OBTENER_LOCALIDAD "SELECT localidad_id, servicio, prestador_id FROM NGA WHERE bloque=%s"

#define OBTENER_CLAVE "SELECT clave FROM Clave WHERE caracteristica like '0%s'"

/*
MYSQL mysql;
MYSQL_RES *res;
MYSQL_ROW row;
*/

/*
int errorfd;
*/

typedef struct
{
	unsigned int localidad_id;
	char servicio[255];
	unsigned int prestador_id;
	char clave[2];
} localidad_reg;


void error_log (char *message, char *linea);

int bps (char *baudrate);


void spool_entry (char *entrada);
