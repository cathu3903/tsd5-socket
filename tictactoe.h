#ifndef _TICTACTOE_H
#define _TICTACTOE_H

/*********************************************
les fichiers include
*********************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/*********************************************
ce qui concerne le jeu
*********************************************/
/*lesjoueurs*/
#define JOUEUR_0	0
#define JOUEUR_1	1

/*le plateau*/
typedef char t_plateau[3][3];
/* les valeurs des places de la grille*/
char pion[2] = { 'O', 'X' };	/* selon le joueur */

#define VIDE '.'

/* le jeu saisi*/
typedef struct t_jeu
{
	char ligne;
	char colonne;
} t_jeu;

/* les valeurs d'état du jeu*/
#define POSER		'p'
#define DEPLACER	'd'
#define GAGNE_0		'G'
#define GAGNE_1		'g'

/* les actions possibles*/
#define DE		'D'
#define VERS	'V'

/*********************************************
ce qui concerne les PDU
**********************************************/
#define MAX_MESSAGE	254

/* PDU serveur-->client*/
typedef struct t_pdu_sc
{
	t_plateau plateau;
	char etat;
	char message[MAX_MESSAGE + 1];
} t_pdu_sc;

/* PDU client-->serveur*/
typedef struct t_pdu_cs
{
	struct t_jeu jeu;
} t_pdu_cs;


/*********************************************
affichage du plateau
	entrée : plateau
*********************************************/
void
afficher_plateau (t_plateau plateau)
{
	printf ("1 ->  %c  %c  %c\n", plateau[0][0], plateau[0][1],
		plateau[0][2]);
	printf ("2 ->  %c  %c  %c\n", plateau[1][0], plateau[1][1],
		plateau[1][2]);
	printf ("3 ->  %c  %c  %c\n", plateau[2][0], plateau[2][1],
		plateau[2][2]);
	printf ("      1   2   3\n\n");
}

#endif
