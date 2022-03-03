/************************************************\
*  Gerencia de Telecomunicaciones y Teleprocesos *
\************************************************/

#include <stdio.h>
#include <fcntl.h>
#define MAXLINE 256

int main(int argc, char *argv[])
{
	int n, fd[2], entrada, salida;
	int pid;
	char line[MAXLINE], c;

	if (pipe(fd) < 0)
	{
		perror("pipe error");
		exit (-1);
	}

	if ((entrada=open(argv[1], O_RDONLY)) < 0)
	{
		perror("open archivo entrada");
		exit (-1);
	}

	if ((salida=open(argv[2], O_CREAT | O_WRONLY | O_APPEND)) < 0)
	{
		perror("open archivo salida");
		exit (-1);
	}

	if ((pid=fork()) < -1)
	{
		perror("fork error");
		exit (-1);
	}
	else if (pid == 0)
	{
		close (fd[1]);
		while (read (fd[0], &c, 1) > 0)
		{
			write (salida, &c, 1);
		}
		close(salida);
	}
	else
	{
		close (fd[0]);
		while (read(entrada,&c,1) > 0)
		{
			write(fd[1],&c,1);
		}
		close(entrada);
	}
	exit (0);
}
