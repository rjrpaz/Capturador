TARGET =	capturador.o \
			centralemu \
			capturador_minimo spooler \
			caprentas caprentas-muleto dbup_rentas spooler_rentas \
			capgobierno capgobierno-muleto dbup_gobierno spooler_gobierno \
			capgobierno-9370 capgobierno-sumario capgobierno-sumario2 \
			capfinanzas capfinanzas-muleto dbup_finanzas spooler_finanzas \
			cappizzurno cappizzurno-muleto dbup_pizzurno spooler_pizzurno \
			capdeportes capdeportes-muleto dbup_deportes spooler_deportes \
			capreggral dbup_reggral spooler_reggral \
			capeducacion capeducacion-muleto dbup_educacion spooler_educacion \
			capdre capdre-muleto dbup_dre spooler_dre \
			cappolicia cappolicia-muleto cappolicia-internet dbup_policia spooler_policia \
			barrer_policia \
			capipam dbup_ipam spooler_ipam \
			capmuleto capmuleto2 dbup_muleto prueba_mail \
			capjusticia dbup_justicia spooler_justicia \
			capdaina dbup_daina spooler_daina \
			emu_nec capdemo dbup_demo \
			prueba_conexion \
			actcen \
			priced

RM = /bin/rm -f
#MYSQLINCDIR=/usr/local/include/mysql
MYSQLINCDIR=/usr/local/mysql/include
#MYSQLLIBDIR=/usr/local/lib/mysql/lib
MYSQLLIBDIR=/usr/local/mysql/lib/mysql
#MYSQLLIBDIR=/usr/lib/


DEBUG=1
ifdef DEBUG
	CFLAGS = -g -O3 -Wall -L$(MYSQLLIBDIR) -lmysqlclient
else
	CFLAGS = -L$(MYSQLLIBDIR) -lmysqlclient
endif

all: $(TARGET)

.c.o:
	$(CC) -L$(MYSQLLIBDIR) -I$(MYSQLINCDIR) -c $<

capturador_minimo: capturador_minimo.o capturador.o
	$(CC) -o capturador_minimo $(CFLAGS) capturador.o capturador_minimo.o
caprentas: caprentas.o capturador.o
	$(CC) -o caprentas $(CFLAGS) capturador.o caprentas.o
dbup_rentas: dbup_rentas.o capturador.o
	$(CC) -o dbup_rentas $(CFLAGS) capturador.o dbup_rentas.o
capgobierno: capgobierno.o capturador.o
	$(CC) -o capgobierno $(CFLAGS) capturador.o capgobierno.o
capgobierno-muleto: capgobierno-muleto.o capturador.o
	$(CC) -o capgobierno-muleto $(CFLAGS) capturador.o capgobierno-muleto.o
capgobierno-9370: capgobierno-9370.o capturador.o
	$(CC) -o capgobierno-9370 $(CFLAGS) capturador.o capgobierno-9370.o
capgobierno-sumario: capgobierno-sumario.o capturador.o
	$(CC) -o capgobierno-sumario $(CFLAGS) capturador.o capgobierno-sumario.o
capgobierno-sumario2: capgobierno-sumario2.o capturador.o
	$(CC) -o capgobierno-sumario2 $(CFLAGS) capturador.o capgobierno-sumario2.o
dbup_gobierno: dbup_gobierno.o capturador.o
	$(CC) -o dbup_gobierno $(CFLAGS) capturador.o dbup_gobierno.o
capfinanzas: capfinanzas.o capturador.o
	$(CC) -o capfinanzas $(CFLAGS) capturador.o capfinanzas.o
capfinanzas-muleto: capfinanzas-muleto.o capturador.o
	$(CC) -o capfinanzas-muleto $(CFLAGS) capturador.o capfinanzas-muleto.o
dbup_finanzas: dbup_finanzas.o capturador.o
	$(CC) -o dbup_finanzas $(CFLAGS) capturador.o dbup_finanzas.o
cappizzurno: cappizzurno.o capturador.o
	$(CC) -o cappizzurno $(CFLAGS) capturador.o cappizzurno.o
cappizzurno-muleto: cappizzurno-muleto.o capturador.o
	$(CC) -o cappizzurno-muleto $(CFLAGS) capturador.o cappizzurno-muleto.o
dbup_pizzurno: dbup_pizzurno.o capturador.o
	$(CC) -o dbup_pizzurno $(CFLAGS) capturador.o dbup_pizzurno.o
capdeportes: capdeportes.o capturador.o
	$(CC) -o capdeportes $(CFLAGS) capturador.o capdeportes.o
dbup_deportes: dbup_deportes.o capturador.o
	$(CC) -o dbup_deportes $(CFLAGS) capturador.o dbup_deportes.o
dbup_deportes-muleto: dbup_deportes-muleto.o capturador.o
	$(CC) -o dbup_deportes-muleto $(CFLAGS) capturador.o dbup_deportes-muleto.o
capreggral: capreggral.o capturador.o
	$(CC) -o capreggral $(CFLAGS) capturador.o capreggral.o
dbup_reggral: dbup_reggral.o capturador.o
	$(CC) -o dbup_reggral $(CFLAGS) capturador.o dbup_reggral.o
capeducacion: capeducacion.o capturador.o
	$(CC) -o capeducacion $(CFLAGS) capturador.o capeducacion.o
dbup_educacion: dbup_educacion.o capturador.o
	$(CC) -o dbup_educacion $(CFLAGS) capturador.o dbup_educacion.o
capdeportes-muleto: capdeportes-muleto.o capturador.o
	$(CC) -o capdeportes-muleto $(CFLAGS) capturador.o capdeportes-muleto.o
caprentas-muleto: caprentas-muleto.o capturador.o
	$(CC) -o caprentas-muleto $(CFLAGS) capturador.o caprentas-muleto.o
capeducacion-muleto: capeducacion-muleto.o capturador.o
	$(CC) -o capeducacion-muleto $(CFLAGS) capturador.o capeducacion-muleto.o
capdre: capdre.o capturador.o
	$(CC) -o capdre $(CFLAGS) capturador.o capdre.o
capdre-muleto: capdre-muleto.o capturador.o
	$(CC) -o capdre-muleto $(CFLAGS) capturador.o capdre-muleto.o
dbup_dre: dbup_dre.o capturador.o
	$(CC) -o dbup_dre $(CFLAGS) capturador.o dbup_dre.o
cappolicia: cappolicia.o capturador.o
	$(CC) -o cappolicia $(CFLAGS) capturador.o cappolicia.o
cappolicia-muleto: cappolicia-muleto.o capturador.o
	$(CC) -o cappolicia-muleto $(CFLAGS) capturador.o cappolicia-muleto.o
cappolicia-internet: cappolicia-internet.o capturador.o
	$(CC) -o cappolicia-internet $(CFLAGS) capturador.o cappolicia-internet.o
dbup_policia: dbup_policia.o capturador.o
	$(CC) -o dbup_policia $(CFLAGS) capturador.o dbup_policia.o
dbup_policia-muleto: dbup_policia-muleto.o capturador.o
	$(CC) -o dbup_policia-muleto $(CFLAGS) capturador.o dbup_policia-muleto.o
spooler_policia: spooler_policia.o capturador.o
	$(CC) -o spooler_policia $(CFLAGS) capturador.o spooler_policia.o
barrer_policia: barrer_policia.o capturador.o
	$(CC) -o barrer_policia $(CFLAGS) capturador.o barrer_policia.o
capipam: capipam.o capturador.o
	$(CC) -o capipam $(CFLAGS) capturador.o capipam.o
dbup_ipam: dbup_ipam.o capturador.o
	$(CC) -o dbup_ipam $(CFLAGS) capturador.o dbup_ipam.o
capjusticia: capjusticia.o capturador.o
	$(CC) -o capjusticia $(CFLAGS) capturador.o capjusticia.o
dbup_justicia: dbup_justicia.o capturador.o
	$(CC) -o dbup_justicia $(CFLAGS) capturador.o dbup_justicia.o
capdaina: capdaina.o capturador.o
	$(CC) -o capdaina $(CFLAGS) capturador.o capdaina.o
dbup_daina: dbup_daina.o capturador.o
	$(CC) -o dbup_daina $(CFLAGS) capturador.o dbup_daina.o
capmuleto: capmuleto.o capturador.o
	$(CC) -o capmuleto $(CFLAGS) capturador.o capmuleto.o
capmuleto2: capmuleto2.o capturador.o
	$(CC) -o capmuleto2 $(CFLAGS) capturador.o capmuleto2.o
dbup_muleto: dbup_muleto.o capturador.o
	$(CC) -o dbup_muleto $(CFLAGS) capturador.o dbup_muleto.o
priced: priced.o capturador.o
	$(CC) -o priced $(CFLAGS) -D_GNU_SOURCE capturador.o priced.o
emu_nec: emu_nec.o
	$(CC) -o emu_nec $(CFLAGS) emu_nec.o
capdemo: capdemo.o capturador.o
	$(CC) -o capdemo $(CFLAGS) capturador.o capdemo.o
dbup_demo: dbup_demo.o capturador.o
	$(CC) -o dbup_demo $(CFLAGS) capturador.o dbup_demo.o
actcen: actcen.o capturador.o
	$(CC) -o actcen $(CFLAGS) capturador.o actcen.o
dgramclnt: dgramclnt.o
	$(CC) -o dgramclnt $(CFLAGS) -D_GNU_SOURCE dgramclnt.o

clean: 
	$(RM) $(TARGET) *.o

install-rentas:
	scp caprentas dbup_rentas caprentas:/root/

install-gobierno:
	scp capgobierno dbup_gobierno capgobierno:/root/

install-finanzas:
	scp capfinanzas dbup_finanzas capfinanzas:/root/

install-pizzurno:
	scp cappizzurno dbup_pizzurno cappizzurno:/root/

install-deportes:
	scp capdeportes dbup_deportes capdeportes:/root/

install-priced:
	scp priced d250lx05:/usr/sbin/

install:
install-muleto:
	cp -p capmuleto capmuleto2 dbup_muleto /root/

