#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>
#include <span>

#include "cppitertools/range.hpp"
//#include "gsl/span"

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.
#include "debogage_memoire.hpp"        // Ajout des numéros de ligne des "new" dans le rapport de fuites.  Doit être après les include du système, qui peuvent utiliser des "placement new" (non supporté par notre ajout de numéros de lignes).

using namespace std;
using namespace iter;
//using namespace gsl;

#pragma endregion//}

typedef uint8_t UInt8;
typedef uint16_t UInt16;

#pragma region "Fonctions de base pour lire le fichier binaire"//{
template <typename T>
T lireType(istream& fichier)
{
    T valeur{};
    fichier.read(reinterpret_cast<char*>(&valeur), sizeof(valeur));
    return valeur;
}
#define erreurFataleAssert(message) assert(false&&(message)),terminate()
static const uint8_t enteteTailleVariableDeBase = 0xA0;
size_t lireUintTailleVariable(istream& fichier)
{
    uint8_t entete = lireType<uint8_t>(fichier);
    switch (entete) {
    case enteteTailleVariableDeBase + 0: return lireType<uint8_t>(fichier);
    case enteteTailleVariableDeBase + 1: return lireType<uint16_t>(fichier);
    case enteteTailleVariableDeBase + 2: return lireType<uint32_t>(fichier);
    default:
        erreurFataleAssert("Tentative de lire un entier de taille variable alors que le fichier contient autre chose à cet emplacement.");
    }
}

string lireString(istream& fichier)
{
    string texte;
    texte.resize(lireUintTailleVariable(fichier));
    fichier.read((char*)&texte[0], streamsize(sizeof(texte[0])) * texte.length());
    return texte;
}

#pragma endregion//}

void ajouterFilmListeFilms(ListeFilms& listeFilms, Film* film) {
    if (listeFilms.capacite <= (listeFilms.nElements + 1)) {
        int nouvelleCapacite = (listeFilms.capacite == 0) ? 1 : 2 * listeFilms.capacite;
        Film** tableauDouble = new Film * [nouvelleCapacite];

        if (listeFilms.nElements != 0) {
            for (int i = 0; i < listeFilms.nElements; i++) {
                tableauDouble[i] = listeFilms.elements[i];
            }
        }
        delete[] listeFilms.elements;
        listeFilms.elements = tableauDouble;
        listeFilms.capacite = nouvelleCapacite;
    }
    listeFilms.elements[listeFilms.nElements] = film;
    listeFilms.nElements++;
}

void enleverFilmListeFilms(ListeFilms& listeFilms, Film* film) {
    for (int i = 0; i < listeFilms.nElements; ++i) {
        if (listeFilms.elements[i] == film) {
            for (int j = i; j < listeFilms.nElements - 1; ++j) {
                listeFilms.elements[j] = listeFilms.elements[j + 1];
            }
        }
    }
    listeFilms.nElements--;

}

Acteur* trouverActeurListeFilms(const ListeFilms& listeFilms, const string& nomActeur) {
    for (auto* filmDansListe : span(listeFilms.elements, listeFilms.nElements)) {
        for (int valeur : range(filmDansListe->acteurs.nElements)) {
            bool acteurTrouve = filmDansListe->acteurs.elements[valeur]->nom == nomActeur;
            if (acteurTrouve) {
                return filmDansListe->acteurs.elements[valeur];
            }
            //else a tout casse
        }
    }
    return nullptr;
}

Acteur* lireActeur(istream& fichier, const  ListeFilms& listeFilms)
{
    Acteur acteur = {};
    acteur.nom = lireString(fichier);
    acteur.anneeNaissance = int(lireUintTailleVariable(fichier));
    acteur.sexe = char(lireUintTailleVariable(fichier));

    Acteur* acteurExistant = trouverActeurListeFilms(listeFilms, acteur.nom);

    bool acteurEstPtrNull = acteurExistant == nullptr;
    if (acteurEstPtrNull) {
        cout << "Nom de l'acteur ajouté: " << acteur.nom << " " << acteur.anneeNaissance << '\n';
        Acteur* acteurCree = new Acteur;
        *acteurCree = acteur;
        return acteurCree;
    }
    else {
        return acteurExistant;
    }
}

Film* lireFilm(istream& fichier, ListeFilms& listeFilms)
{
    Film film = {};
    film.titre = lireString(fichier);
    film.realisateur = lireString(fichier);
    film.anneeSortie = int(lireUintTailleVariable(fichier));
    film.recette = int(lireUintTailleVariable(fichier));
    film.acteurs.nElements = int(lireUintTailleVariable(fichier));


    film.acteurs.elements = new Acteur * [film.acteurs.nElements];

    Film* newFilm = new Film(film);

    for (int i = 0; i < film.acteurs.nElements; i++) {
        Acteur* ptrActeur = lireActeur(fichier, listeFilms);
        //Placer l'acteur au bon endroit dans les acteurs du film.
        film.acteurs.elements[i] = ptrActeur;
        //Ajouter le film à la liste des films dans lesquels l'acteur joue.
        ajouterFilmListeFilms(ptrActeur->joueDans, newFilm);

    }
    return newFilm;
}

ListeFilms creerListe(string nomFichier)
{
    ifstream fichier(nomFichier, ios::binary);
    fichier.exceptions(ios::failbit);

    int nElements = int(lireUintTailleVariable(fichier));

    ListeFilms listeFilms{};
    for (int i = 0; i < nElements; i++) {
        ajouterFilmListeFilms(listeFilms, lireFilm(fichier, listeFilms));
    }

    return listeFilms;
}

void detruireFilm(Film* film, ListeFilms& listeFilms) {
    enleverFilmListeFilms(listeFilms, film);
    for (Acteur* acteur : span(film->acteurs.elements, film->acteurs.nElements)) {
        enleverFilmListeFilms(acteur->joueDans, film);
        if (acteur->joueDans.nElements == 0) {
            cout << "Destruction de l'acteur " << acteur->nom << endl;
            delete[] acteur->joueDans.elements;
            delete acteur;
        }
    }
    delete[] film->acteurs.elements;
    delete film;

}

void detruireListeFilms(ListeFilms& listeFilms) {
    for (int index : range(0, listeFilms.nElements)) {
        detruireFilm(listeFilms.elements[0], listeFilms);
    }
    delete[] listeFilms.elements;
}

void afficherActeur(const Acteur& acteur)
{
    cout << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}

void afficherListeFilms(const ListeFilms& listeFilms) {
    static const string ligneDeSeparation = "----------------------------------------\n";
    cout << ligneDeSeparation;

    for (Film* film : span(listeFilms.elements, listeFilms.nElements)) {
        cout << "  " << film->titre << ", Realise par " << film->realisateur << ", sorti en " << film->anneeSortie << ", recettes : " << film->recette << ", Acteurs: " << endl;
        for (Acteur* acteur : span(film->acteurs.elements, film->acteurs.nElements)) {
            afficherActeur(*acteur);
        }

        cout << ligneDeSeparation;
    }
}

void afficherFilmographieActeur(const ListeFilms& listeFilms, const string& nomActeur)
{
    const Acteur* acteur = trouverActeurListeFilms(listeFilms, nomActeur);
    if (acteur == nullptr) {
        cout << "Aucun acteur de ce nom" << endl;
    }
    else {
        afficherListeFilms(acteur->joueDans);
    }
}

int main()
{
    bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.

    static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";


    //TODO: La ligne suivante devrait lire le fichier binaire en allouant la mémoire nécessaire.
    // Devrait afficher les noms de 20 acteurs sans doublons
    // (par l'affichage pour fins de débogage dans votre fonction lireActeur).
    ListeFilms listeFilms = creerListe("films.bin");


    cout << ligneDeSeparation << "Le premier film de la liste est:" << endl;
    //TODO: Afficher le premier film de la liste.  Devrait être Alien.
    Film* film = listeFilms.elements[0];
    cout << "  " << film->titre << ", Realise par " << film->realisateur << ", sorti en " << film->anneeSortie << ", recettes : " << film->recette << endl;

    cout << ligneDeSeparation << "Les films sont:" << endl;
    //TODO: Afficher la liste des films.  Il devrait y en avoir 7.
    afficherListeFilms(listeFilms);

    //TODO: Modifier l'année de naissance de Benedict Cumberbatch pour être 1976 (elle était 0 dans les données lues du fichier).  Vous ne pouvez pas supposer l'ordre des films et des acteurs dans les listes, il faut y aller par son nom.
    Acteur* benedictCumberbatch = trouverActeurListeFilms(listeFilms, "Benedict Cumberbatch");
    benedictCumberbatch-> anneeNaissance = 1976;


    cout << benedictCumberbatch->nom << " " << benedictCumberbatch->anneeNaissance << endl;
    cout << ligneDeSeparation << "Liste des films où Benedict Cumberbatch joue sont:" << endl;

    //TODO: Afficher la liste des films où Benedict Cumberbatch joue.  Il devrait y avoir Le Hobbit et Le jeu de l'imitation.

    afficherFilmographieActeur(listeFilms, benedictCumberbatch->nom);

    //TODO: Détruire et enlever le premier film de la liste (Alien).  Ceci devrait "automatiquement" (par ce que font vos fonctions) détruire les acteurs Tom Skerritt et John Hurt, mais pas Sigourney Weaver puisqu'elle joue aussi dans Avatar.
    detruireFilm(listeFilms.elements[0], listeFilms);
    cout << ligneDeSeparation << "Les films sont maintenant:" << endl;
    //TODO: Afficher la liste des films.
    afficherListeFilms(listeFilms);

    //TODO: Faire les appels qui manquent pour avoir 0% de lignes non exécutées dans le programme
    // (aucune ligne rouge dans la couverture de code; c'est normal que les lignes de "new"
    // et "delete" soient jaunes).  Vous avez aussi le droit d'effacer les lignes du programmes
    // qui ne sont pas exécutée, si finalement vous pensez qu'elle ne sont pas utiles.

    //TODO: Détruire tout avant de terminer le programme.
    // La bibliothèque de verification_allocation devrait afficher "Aucune fuite detectee."
    // a la sortie du programme; il affichera "Fuite detectee:" avec la liste des blocs, s'il manque
    // des delete.
    detruireListeFilms(listeFilms);
}
