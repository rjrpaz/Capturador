#!/usr/bin/perl
use strict;

my $contador = 1;
open (ARCHIVO, "./lista.lst");
while (<ARCHIVO>)
{
#	chop;
#	@parametros = split /\|/, $_;
#	$comando = "/root/cap/dbup_muleto.ori -c $parametros[0] -A $parametros[1] -B $parametros[2] -f $parametros[3] -p $parametros[4] -d $parametros[5]";
#	$comando = "/root/cap/dbup_muleto -c $parametros[0] -A $parametros[1] -B $parametros[2] -f $parametros[3] -p $parametros[4] -d $parametros[5] &";
#	print $comando, "\n";
#	$comando = sprintf("%04d", $comando);
#	print $comando,":\t",$_, "\n";
#	system($_);
#	$comando++;
	print `$_`;
	print "$_";
	sleep(1);
}
close (ARCHIVO);
exit (0);
