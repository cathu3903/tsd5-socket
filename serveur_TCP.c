/*  Application MIROIR  : cote serveur      */

#include "commun.h"


void miroir (char *chaine);

int
main (int argc, char **argv)
{

    /*---------------------------------------------- les variables */
    /* pour le serveur */
    struct sockaddr_in adTCP_serveur;   /* pour mettre l'adresse de son point d'acc�s */
    int point_acces_serveur, guichet;

    /* pour le service */
    struct sockaddr_in adresse_appelant;
#ifdef  __osf__
    int lg_appelant;
#else
    socklen_t lg_appelant;
#endif
    char message[100];
    int recus, emis;

    /* divers */
    int retour;

    /*---------------------------------------------- l'algorithme */

    /* v�rification des arguments */
    if (argc != 2)
    {
        printf ("Usage : %s port\n", argv[0]);
        exit (-1);
    }
    /* cr�ation d'un point d'acc�s */
    point_acces_serveur = socket (PF_INET, SOCK_STREAM, 0);
    if (point_acces_serveur < 0)
    {
        perror ("ERREUR-socket ");
        exit (-1);
    }
    /* pr�paration de l'adresse du point d'acc�s */
    bzero (&adTCP_serveur, sizeof (adTCP_serveur));
    adTCP_serveur.sin_family = AF_INET;
    adTCP_serveur.sin_port = htons (atoi (argv[1]));
    adTCP_serveur.sin_addr.s_addr = INADDR_ANY;

    /* attribution de l'adresse au point d'acc�s */
    retour = bind (point_acces_serveur,
               (struct sockaddr *) &adTCP_serveur,
               sizeof (struct sockaddr_in));
    if (retour < 0)
    {
        perror ("ERREUR-bind ");
        exit (-1);
    }
    /* d�claration de r�pondeur */
    retour = listen (point_acces_serveur, 5);
    if (retour < 0)
    {
        perror ("ERREUR-listen ");
        close (point_acces_serveur);
        exit (1);
    }

    /* attente et traitement d'une demande */
    while (1)
    {
        /* attente d'un appel */
        lg_appelant = sizeof (struct sockaddr_in);
        guichet = accept (point_acces_serveur,
                  (struct sockaddr *) &adresse_appelant,
                  &lg_appelant);
        if (guichet < 0)
        {
            perror ("ERREUR-accept() ");
            continue;
        }
        printf ("---------------------CONNEXION de %s:%d\n",
            inet_ntoa (adresse_appelant.sin_addr),
            ntohs (adresse_appelant.sin_port));

        /* r�ception */
        while(1)
        {
            recus = recv (guichet, message, sizeof (message), 0);
            if (recus < 0)
            {
                perror ("ERREUR-recv ");
                shutdown (guichet, SHUT_RDWR);
                close (guichet);
                continue;
            }
            else
            {
                printf ("RECU %d octets : ", recus);
                printf ("\t%s\n", message);

                /* on calcule la r�ponse */
                miroir (message);

                /* et on lui renvoie */
                emis = send (guichet, message, strlen (message) + 1,
                         0);
                if (emis < 0)
                {
                    perror ("ERREUR-send ");
                }
                else
                {
                    printf ("ENVOI %d octets : ",emis);
                    printf ("\t%s\n", message);
                }
            }
        }

        /*
        printf ("---------------------FIN de CONNEXION de %s:%d\n",
            inet_ntoa (adresse_appelant.sin_addr),
            ntohs (adresse_appelant.sin_port));
        shutdown (guichet, SHUT_RDWR);
        close (guichet);
        */

    }
    shutdown (guichet, SHUT_RDWR);
    close (guichet);
    close (point_acces_serveur);
}

void
miroir (char *chaine)
{
    char chaine_miroir[100];
    int i;

    for (i = 0; i < strlen (chaine); i++)
    {
        chaine_miroir[strlen (chaine) - 1 - i] = chaine[i];
    }
    chaine_miroir[strlen (chaine)] = '\0';
    strcpy (chaine, chaine_miroir);
}
