/*
 Este programa envia los datos de un archivo, por un puerto serial
*/

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS0"

#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE; 

int main(int argc, char *argv[])
{
	int fd, filefd, c;
	struct termios oldtio,newtio;

	if (argc != 2)
	{
		printf("Uso:\n\t%s <archivo_dat>\n\n", argv[0]);
		exit(0);
	}
	 
/* 
  Open modem device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
*/

	fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY);
	if (fd <0) 
	{
		perror(MODEMDEVICE); 
		exit(-1); 
	}
 
	filefd = open(argv[1], O_RDONLY);
	if (filefd <0) 
	{
		perror(argv[1]); 
		exit(-1); 
	}

	tcgetattr(fd,&oldtio); /* save current modem settings */
 
/* 
  Set bps rate and hardware flow control and 8n1 (8bit,no parity,1 stopbit).
  Also don't hangup automatically and ignore modem status.
  Finally enable receiving characters.
 newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
*/
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
 
/*
 Ignore bytes with parity errors and make terminal raw and dumb.
 newtio.c_iflag = IGNPAR;
*/
	newtio.c_iflag = IXON | IXOFF | IGNBRK | ISTRIP | IGNPAR;
 
/*
 Raw output.
*/
	newtio.c_oflag = 0;
 
/*
 Don't echo characters because if you connect to a host it or your
 modem will echo characters for you. Don't generate signals.
*/
	newtio.c_lflag = 0;
 
/* blocking read until 1 char arrives */
	newtio.c_cc[VMIN]=1;
	newtio.c_cc[VTIME]=0;
 
/* now clean the modem line and activate the settings for modem */
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);

	STOP = FALSE;
	while (read(filefd,&c,1) > 0)
	{
		write(fd,&c,1);
	}
	close(fd);
	close(filefd);
	tcsetattr(fd,TCSANOW,&oldtio); /* restore old modem setings */
	exit(0); 
}
