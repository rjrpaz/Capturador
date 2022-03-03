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


#define DBUSER "capgobierno"
#define DBPASS "chirolita"
#define DBTABLE "Gobierno"

#define DBUP "/root/dbup_gobierno"

MYSQL mysql;

int main(int argc, char *argv[])
{
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	char *spooldir = "/var/spool/capturador", filename[256], buffer[256];
	char llamante[32], llamado[32];
	char pulsos[6], duracion[6], codigo[6];
	char fecha[10], command[256];
	FILE *fd;

	mysql_init(&mysql);

	if (!mysql_real_connect(&mysql, DBHOST, DBUSER, DBPASS, DBBASE, 0, NULL, 0))
	{
		puts("Problemas para conectarse con base de datos");
		exit (1);
	}
	mysql_close(&mysql);

	if ((dp = opendir(spooldir)) == NULL)
	{
		printf("No puedo acceder a directorio %s\n", spooldir);
		exit (1);
	}
	chdir (spooldir);
	while ((entry = readdir(dp)) != NULL)
	{
		lstat(entry->d_name, &statbuf);
		/*********************************\
		* Solo analiza archivos regulares *
		\*********************************/
		if (S_ISREG(statbuf.st_mode))
		{
			sprintf(filename, "%s/%s", spooldir, entry->d_name);
			fd = fopen(filename, "r");
			
			/*******************************************\
			* Solo procesa los archivos que puede abrir *
			\*******************************************/
			if (fd < 0)
			{
				puts ("Error al intentar abrir archivo");
			}
			else
			{
				while (fgets (buffer, sizeof(buffer), fd) != (char *) NULL)	
				{
					if (sscanf (buffer, "%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^\n]", codigo, llamante, llamado, fecha, pulsos, duracion) != 6)
					{
						perror("sscanf");
					}
					else
					{
						sprintf (command, "%s -c %s -A %s -B %s -f %s -p %s -d %s &", DBUP, codigo, llamante, llamado, fecha, pulsos, duracion);
						system(command);
						sleep(5);
					}
				} /* Fin del while */

			} /* Fin del else */

			fclose (fd);
			sprintf (command, "/bin/rm -f %s", filename);
			system(command);
		}
	}
	closedir(dp);
	exit (0);
}
