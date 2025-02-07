/*	Application MIROIR  : cote client		*/

#include "commun.h"

void viderBuffer()
{
    int c = 0;
    while (c != '\n' && c != EOF)
    {
        c = getchar();
    }
}

int
main (int argc, char **argv)
{

	/*---------------------------------------------- les variables */

	/* pour le client */
	int point_acces_client;	/* le point d'accès */

	/* pour l'expédition */
	struct hostent *adIP_serveur;	/* système où est le serveur */
	struct sockaddr_in adTCP_serveur;	/* pour mettre l'adresse de son point d'accès */

	/* pour l'envoi  */
	char envoyer[100];
	int emis;

	/* pour la réception  */
	char recu[100];
	int recus;

	/* divers */
	int retour;
	char c;

	/*---------------------------------------------- l'algorithme */

	/* vérification des arguments */
	if (argc != 3)
	{
		printf ("Usage : %s nom_systeme port\n", argv[0]);
		exit (-1);
	}
	/* récupération des adresses-IP du système où est le serveur */
	if ((adIP_serveur = gethostbyname (argv[1])) == NULL)
	{
		herror("gethostbyname ");
		exit (-1);
	}
	/* création d'un point d'accès */
	point_acces_client = socket (PF_INET, SOCK_STREAM, 0);
	if (point_acces_client < 0)
	{
		perror ("ERREUR-socket ");
		exit (-1);
	}

	/* préparation de l'adresse destinataire (serveur) */
	bzero (&adTCP_serveur, sizeof (adTCP_serveur));
	adTCP_serveur.sin_family = AF_INET;
	adTCP_serveur.sin_port = htons (atoi (argv[2]));
	memcpy (&adTCP_serveur.sin_addr, adIP_serveur->h_addr, 4);

	/* connexion au serveur */
	retour = connect (point_acces_client,
			  (struct sockaddr *) &adTCP_serveur,
			  sizeof (adTCP_serveur));
	if (retour < 0)
	{
		perror ("ERREUR-connect ");
		printf ("\n");
		exit (-1);
	}
	printf ("CONNEXION etablie\n");


	while(1)
	{
		/* lecture d'une ligne au clavier */


		printf ("Que faut-il envoyer ? \n\t");
		scanf("%99[^\n]",envoyer);
		viderBuffer();

		if(envoyer[0] == 'q' && strlen(envoyer) == 1)
			break;

		/* envoi du bloc d'octets */
		emis = send (point_acces_client, envoyer, strlen (envoyer) + 1, 0);
		if (emis < 0)
		{
			perror ("ERREUR-send ");
			shutdown (point_acces_client, SHUT_RDWR);
			close (point_acces_client);
			exit (-1);
		}
		printf ("ENVOI de %d octets\n", emis);

		/* réception de la réponse */
		recus = recv (point_acces_client, recu, sizeof (recu), 0);
		if (recus < 0)
		{
			perror ("ERREUR-recv ");
			shutdown (point_acces_client, SHUT_RDWR);
			close (point_acces_client);
			exit (-1);
		}
		printf ("RECU  %d octets :", recus);
		printf ("\t%s\n", recu);
		printf("Want to continue?(y / n)\n");
		//getchar();
		scanf("%c", &c);
		if(c == 'n')
			break;
		viderBuffer();


		//while (getchar() != '\n');
	}

	/* fermeture du point d'accès */
	shutdown (point_acces_client, SHUT_RDWR);
	close (point_acces_client);
	printf ("FIN DE CONNEXION\n\n");

	return (0);
}
