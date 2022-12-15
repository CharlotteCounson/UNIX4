.SILENT:
DDEBUG = #-D DEBUG
PROGRAMS = Administrateur Client Serveur CreationBD BidonFichierPub Publicite Consultation Modification 
COMP = g++ -Wall $(DDEBUG) -W -c -pipe -g -std=gnu++11
OBJS1 = mainAdmin.o windowadmin.o moc_windowadmin.o
OBJS2 = dialogmodification.o mainClient.o windowclient.o moc_dialogmodification.o moc_windowclient.o
LIB_QT = /usr/lib64/libQt5Widgets.so /usr/lib64/libQt5Gui.so /usr/lib64/libQt5Core.so /usr/lib64/libGL.so
QT = -I. -isystem /usr/include/qt5 -isystem /usr/include/qt5/QtWidgets -isystem /usr/include/qt5/QtGui -isystem /usr/include/qt5/QtCore -I. -I. -I/usr/lib64/qt5/mkspecs/linux-g++
D_REENTRANT = -D_REENTRANT -fPIC -DQT_DEPRECATED_WARNINGS -DQT_QML_DEBUG -DQT_WIDGETS_LIB -DQT_GUI_LIB -DQT_CORE_LIB
MYSQL = -I/usr/include/mysql -m64 -L/usr/lib64/mysql -lmysqlclient -lpthread -lz -lm -lrt -lssl -lcrypto -ldl


all: $(PROGRAMS)

Administrateur: $(OBJS1)
	echo Edition de liens "Administrateur" ...
	g++  -o Administrateur $(OBJS1) $(LIB_QT) -lpthread

mainAdmin.o: mainAdmin.cpp
	echo Compilation de "mainAdmin.o" ...
	$(COMP) $(D_REENTRANT) -I../Administrateur $(QT) -o mainAdmin.o mainAdmin.cpp

windowadmin.o: windowadmin.cpp
	echo Compilation de "windowadmin.o" ...
	$(COMP) $(D_REENTRANT) -I../Administrateur $(QT) -o windowadmin.o windowadmin.cpp
	
moc_windowadmin.o: moc_windowadmin.cpp
	echo Compilation de "moc_windowadmin.o" ...
	$(COMP) $(D_REENTRANT) -I../Administrateur $(QT) -o moc_windowadmin.o moc_windowadmin.cpp


Client: $(OBJS2)
	echo Edition de liens de "Client" ...
	g++  -o Client $(OBJS2) $(LIB_QT) -lpthread

dialogmodification.o: dialogmodification.cpp
	echo Compilation de "dialogmodification.o" ... 
	$(COMP) $(D_REENTRANT) -I../UNIX_DOSSIER_FINAL $(QT) -o dialogmodification.o dialogmodification.cpp

mainClient.o: mainClient.cpp
	echo Compilation de "mainClient.o" ...
	$(COMP) $(D_REENTRANT) -I../UNIX_DOSSIER_FINAL $(QT) -o mainClient.o mainClient.cpp

windowclient.o: windowclient.cpp
	echo Compilation de "windowclient.o" ...
	$(COMP) $(D_REENTRANT) -I../UNIX_DOSSIER_FINAL $(QT) -o windowclient.o windowclient.cpp

moc_dialogmodification.o: moc_dialogmodification.cpp
	echo Compilation de "moc_dialogmodification.o" ...
	$(COMP) $(D_REENTRANT) -I../UNIX_DOSSIER_FINAL $(QT) -o moc_dialogmodification.o moc_dialogmodification.cpp

moc_windowclient.o: moc_windowclient.cpp
	echo Compilation de "moc_windowclient.o" ...
	$(COMP) $(D_REENTRANT) -I../UNIX_DOSSIER_FINAL $(QT) -o moc_windowclient.o moc_windowclient.cpp


Serveur: Serveur.cpp MySemaphores.o
	echo Compilation de "Serveur" ...
	g++ -o Serveur Serveur.cpp MySemaphores.o $(MYSQL) 

CreationBD: CreationBD.cpp
	echo Compilation de "CreationBD" ...
	g++ -o CreationBD CreationBD.cpp $(MYSQL)

BidonFichierPub: BidonFichierPub.cpp
	echo Compilation de "BidonFichierPub" ...
	g++ -o BidonFichierPub BidonFichierPub.cpp

Publicite: Publicite.cpp MySemaphores.o
	echo Compilation de "Publicite" ...
	g++ -o Publicite Publicite.cpp MySemaphores.o

Consultation: Consultation.cpp MySemaphores.o
	echo Compilation de "Consultation" ...
	g++ -o Consultation Consultation.cpp MySemaphores.o  $(MYSQL)

Modification: Modification.cpp MySemaphores.o
	echo Compilation de "Modification" ...
	g++ -o Modification Modification.cpp MySemaphores.o $(MYSQL)

MySemaphores.o: MySemaphores.cpp
	echo Compilation de "MySemaphores.o"...
	$(COMP) $(D_REENTRANT) -o MySemaphores.o MySemaphores.cpp



