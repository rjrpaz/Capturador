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

#define DBUSER "cappolicia"
#define DBPASS "chirolita"
#define DBTABLE "Policia"
#define DBUP "/root/dbup_policia"

MYSQL mysql;

int main(int argc, char *argv[])
{
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	char *spooldir = "/var/spool/capturador", filename[256], buffer[BUFSIZ];
	char tipo_llamada[20];
	char llamante[32], llamado[32];
	char duracion[6], codigo[6];
	char fecha[10], command[256];
	char comentario[256];
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
					if (sscanf (buffer, "%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^|]|%[^\n]", tipo_llamada, codigo, llamante, llamado, fecha, duracion, comentario) != 7)
					{
						perror("sscanf");
					}
					else
					{
						if ((strcmp(tipo_llamada, "entrante")) == 0)
						{
							sprintf (command, "%s -t %s -c %s -A %s -B %s -f %s -d %s -C %s &", DBUP, tipo_llamada, codigo, llamante, llamado, fecha, duracion, comentario);
						}
						else
						{
							sprintf (command, "%s -t %s -c %s -A %s -B %s -f %s -d %s &", DBUP, tipo_llamada, codigo, llamante, llamado, fecha, duracion);
						}
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
