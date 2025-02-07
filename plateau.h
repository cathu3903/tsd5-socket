#include "tictactoe.h"

/* pour simplifier les états*/
char gagne[2] = { GAGNE_0, GAGNE_1 };


/*********************************************
initialise les plateau à VIDE
	sortie : plateau
*********************************************/
void
initialiser_plateau (t_plateau plateau)
{
	int ligne, colonne;
	for (ligne = 0; ligne < 3; ligne++)
		for (colonne = 0; colonne < 3; colonne++)
			plateau[ligne][colonne] = VIDE;
}

/*********************************************
calcul l'état du jeu
	entrée : plateau
	sortie : etat
*********************************************/
void
calcul_etat (t_plateau plateau, char *etat)
{
	int ligne, colonne, joueur, i;
	int nl[2][3] = { {0, 0, 0}, {0, 0, 0} };	/* par joueur et par ligne */
	int nc[2][3] = { {0, 0, 0}, {0, 0, 0} };	/* par joueur et par colonne */
	int n[2] = { 0, 0 };	/* par joueur */
	int nd[2][2] = { {0, 0}, {0, 0} };	/* par joueur, les diagonales */

	/* comptage */
	for (joueur = 0; joueur < 2; joueur++)
		for (ligne = 0; ligne < 3; ligne++)
			for (colonne = 0; colonne < 3; colonne++)
				if (plateau[ligne][colonne] == pion[joueur])
				{
					nl[joueur][ligne]++;
					nc[joueur][colonne]++;
					n[joueur]++;
				}
	/* les digonales */
	for (joueur = 0; joueur < 2; joueur++)
	{
		for (ligne = 0; ligne < 3; ligne++)
		{
			if (plateau[ligne][ligne] == pion[joueur])
				nd[joueur][0]++;
			if (plateau[ligne][2 - ligne] == pion[joueur])
				nd[joueur][1]++;
		}
	}

	/* GAGNE ? si un joueur a une ligne ou une colonne à 3 */
	for (joueur = 0; joueur < 2; joueur++)
		for (i = 0; i < 3; i++)
			if (nl[joueur][i] == 3 || nc[joueur][i] == 3
			    || nd[joueur][0] == 3 || nd[joueur][1] == 3)
			{
				*etat = gagne[joueur];
				return;
			}
	/* POSER ou DEPLACER ? */
	/*      POSER si les 2 joueurs n'ont pas encore posé leurs 3 pions */
	*etat = n[0] < 3 || n[1] < 3 ? POSER : DEPLACER;
}

/*********************************************
analyse du jeu
	entrée : plateau, jeu, joueur
	sortie : etat, action
	retour : 1 en cas d'erreur de saisie
*********************************************/

int ligne_de, colonne_de;
int
analyse_jeu (t_plateau plateau, struct t_jeu *jeu, char *etat, char *action,
	     int joueur)
{
	int ligne, colonne;

	/* y a-t-il une erreur de saisie ? */
	if (jeu->ligne > '0' && jeu->ligne < '4')
		ligne = (jeu->ligne & 0x0f) - 1;
	else
		return 1;
	if (jeu->colonne > '0' && jeu->colonne < '4')
		colonne = (jeu->colonne & 0x0f) - 1;
	else
		return 1;

	/* y a-t-il une erreur de case ? */
	if (*action == VERS && plateau[ligne][colonne] != VIDE)
	{
		return 1;
	}
	if ((*action == DE) && plateau[ligne][colonne] != pion[joueur])
	{
		return 1;
	}

	/* action de pose */
	if (*etat == POSER)
	{
		plateau[ligne][colonne] = pion[joueur];
		calcul_etat (plateau, etat);
		if (*etat == DEPLACER)
			*action = DE;
		return 0;
	}
	else
	{
		/* action de déplacement */
		if (*action == DE)
		{
			ligne_de = ligne;
			colonne_de = colonne;
			*action = VERS;
		}
		else
		{
			plateau[ligne_de][colonne_de] = VIDE;
			plateau[ligne][colonne] = pion[joueur];
			*action = DE;
		}
		calcul_etat (plateau, etat);
		return 0;
	}
}
