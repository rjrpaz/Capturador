#!/bin/sh

# Como corre el dia 1 de cada mes, se fija en la fecha
# del mes anterior
fecha=`/bin/date --date='1 day ago' +%y%m`

# El proceso que debe reiniciar es un argumento del
# comando
programa_que_captura=capturador_minimo
if [ "$1" != "" ]
then
programa_que_captura=$1
fi

# El archivo que debe renombrar puede ser un argumento
# de entrada
logfile=/var/log/capturador.log
logfile_sugerido=$2
if [ "$logfile_sugerido" != "" ]
then
logfile=$logfile_sugerido
fi

# Determina como renombrara el archivo de captura.
filename=`echo $logfile | cut -d"." -f 1`
log_rotado=$filename"_"$fecha".log"

# Determina los nombres de los logs de error.
logerror=$filename"_error.log"
logerror_rotado=$filename"_error_$fecha.log"

killall -9 $programa_que_captura

# Los archivos sobre los cuales va a escribir no deberian
# existir
if [ -f $log_rotado ]
then
echo "$log_rotado ya existe"
/usr/bin/nohup /root/$programa_que_captura &
exit
fi

if [ -f $logerror_rotado ]
then
echo "$logerror_rotado ya existe"
/usr/bin/nohup /root/$programa_que_captura &
exit
fi

# Si hasta ahora no hubo problemas, mueve los logs.
if [ -f $logfile ]
then
/bin/mv $logfile $log_rotado
fi
if [ -f $logerror ]
then
/bin/mv $logerror $logerror_rotado
fi

/usr/bin/nohup /root/$programa_que_captura &

if [ -f $log_rotado.gz ]
then
exit
else
/bin/gzip $log_rotado 2>/dev/null
fi

if [ -f $logerror_rotado.gz ]
then
exit
else
/bin/gzip $logerror_rotado 2>/dev/null
fi

exit

