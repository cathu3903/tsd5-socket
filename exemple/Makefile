systeme=`hostname -s`

CC = gcc
CFLAGS = -Wall -g $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_ttf

all: serveur_TCP client_TCP

serveur_TCP: serveur_gomoku.c commun.h
	$(CC) $(CFLAGS) -o serveur_TCP serveur_gomoku.c

client_TCP: client_gomoku.c commun.h
	$(CC) $(CFLAGS) -o client_TCP client_gomoku.c $(LDFLAGS)

clean:
	rm -f serveur_TCP client_TCP *.o
