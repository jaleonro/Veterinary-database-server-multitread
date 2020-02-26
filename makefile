all:  ejecutarS ejecutarC1 generarOL clear 

ejecutarS: generarOS
	reset	
	gnome-terminal -e ./p3-dogServer

generarOS: p3-dogServer.c
	gcc p3-dogServer.c -o p3-dogServer -lpthread

ejecutarC1: generarOC1
	reset	
	gnome-terminal -e ./p3-dogClient
	gnome-terminal -e ./p3-dogClient
	gnome-terminal -e ./p3-dogClient
	gnome-terminal -e ./p3-dogClient

generarOC1: p3-dogClient.c
	gcc p3-dogClient.c -o p3-dogClient -lpthread

generarOL: consultarLogs.c
	gcc consultarLogs.c -o consultarLogs 

clear:
	rm -f generarOS
	rm -f generarOC1
	rm -f generarOL

