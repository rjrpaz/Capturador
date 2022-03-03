/************************************************\
*  Gerencia de Telecomunicaciones y Teleprocesos *
\************************************************/

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mysql/mysql.h>
#include <termios.h>
#include <stdlib.h>
#include <time.h>
#include "capturador.h"


#define DBUSER "capmuleto"
#define DBPASS "chirolita"
#define DBTABLE "Muleto"

MYSQL mysql;

int main(int argc, char *argv[])
{
	int timeout=50;

	mysql_init(&mysql);
	mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, (char *) &timeout);

	if (!mysql_real_connect(&mysql, DBHOST, DBUSER, DBPASS, DBBASE, 0, NULL, 0))
	{
		printf("Problemas para conectarse con base de datos: %s\n", mysql_error(&mysql));
		exit (1);
	}
	mysql_close(&mysql);

	exit (0);
}
