#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

//#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.
#include "classe.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>
#include <span>

#include "include/cppitertools/range.hpp"


#include "include/bibliotheque_cours.hpp"
#include "include/verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.
#include "include/debogage_memoire.hpp"        // Ajout des numéros de ligne des "new" dans le rapport de fuites.  Doit être après les include du système, qui peuvent utiliser des "placement new" (non supporté par notre ajout de numéros de lignes).

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


ListeFilms::ListeFilms()
    : capacite_(0),
    nElements_(0),
    elements_(nullptr) {}


int ListeFilms::getCapacite() const {
    return capacite_;

}

ListeFilms::~ListeFilms() {
    for (int i = 0; i < nElements_; ++i) {
        if (!elements_[i]->titre.empty()) {
            detruireMemoireFilm(elements_[i]);
        }
    }
    delete[] elements_;
    setElements(nullptr);
}

int ListeFilms::getNElements() const {
    return nElements_;
}

Film** ListeFilms::getElements()  const {
    return elements_;
}
void ListeFilms::setCapacite(int capacite) {
    capacite_ = capacite;
}
void ListeFilms::setElements(Film** elements) {
    elements_ = elements;
}

void ListeFilms::ajouterFilmListeFilms(ListeFilms& listeFilms, Film* film) {
    bool estCapaciteInsuffisante = nElements_ == capacite_;
    if (estCapaciteInsuffisante) {
        setCapacite((capacite_ == 0) ? 1 : capacite_ * 2);

        Film** nouvelleListeFilms = new Film * [capacite_];
        copy(elements_, elements_ + nElements_, nouvelleListeFilms);

        delete[] elements_;
        setElements(nouvelleListeFilms);
    }
    elements_[nElements_] = film;
    ++nElements_;
}

void ListeFilms::enleverFilmListeFilms(ListeFilms& listeFilms, Film* film) {
    const string nomFilm = film->titre;
    for (int i = 0; i < nElements_; ++i) {
        bool filmTrouve = nomFilm == elements_[i]->titre;
        if (filmTrouve) {
            delete film;
            film = nullptr;

            for (int j = i; j < nElements_ - 1; ++j) {
                elements_[j] = elements_[j + 1];
            }
            --nElements_;
            break;
        }
    }
}
Acteur* trouverActeurListeFilms(const ListeFilms& listeFilms, const string& nomActeur) {
    for (auto* filmDansListe : span(listeFilms.getElements(), listeFilms.getNElements())) {
        for (int valeur : range(filmDansListe->acteurs.nElements)) {
            bool acteurTrouve = filmDansListe->acteurs.elements[valeur]->nom == nomActeur;
            if (acteurTrouve) {
                return filmDansListe->acteurs.elements[valeur];
            }
        }
    }
    return nullptr;
}

Acteur* lireActeur(istream& fichier, ListeFilms& listeFilms)
{
    Acteur acteur = {};
    acteur.nom = lireString(fichier);
    acteur.anneeNaissance = int(lireUintTailleVariable(fichier));
    acteur.sexe = char(lireUintTailleVariable(fichier));

    Acteur* acteurExistant = trouverActeurListeFilms(listeFilms, acteur.nom);

    bool acteurEstNullptr = acteurExistant == nullptr;
    if (acteurEstNullptr) {
        cout << "Nom de l'acteur ajouté: " << acteur.nom << '\n';
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

    film.acteurs.capacite = film.acteurs.nElements;
    Acteur** listeActeurs = new Acteur * [film.acteurs.nElements];
    film.acteurs.elements = listeActeurs;

    Film* newFilm = new Film;
    *newFilm = film;
    for (int i = 0; i < film.acteurs.nElements; i++) {
        newFilm->acteurs.elements[i] = lireActeur(fichier, listeFilms);

        listeFilms.ajouterFilmListeFilms(film.acteurs.elements[i]->joueDans, newFilm);

    }
    return newFilm;
}

ListeFilms  ListeFilms::creerListe(string nomFichier)
{
    ifstream fichier(nomFichier, ios::binary);
    fichier.exceptions(ios::failbit);

    int nElements = int(lireUintTailleVariable(fichier));

    ListeFilms listeFilms{};
    for (int i = 0; i < nElements; i++) {
        listeFilms.ajouterFilmListeFilms(listeFilms, lireFilm(fichier, listeFilms));
    }

    return listeFilms;
}

void ListeFilms::detruireMemoireFilm(Film* film)
{
    for (int i = 0; i < film->acteurs.nElements; ++i) {
        bool acteurPresent = (film->acteurs.elements[i]->sexe == 'M') || (film->acteurs.elements[i]->sexe == 'F');
        if (acteurPresent) {
            cout << "Nom de l'acteur détruit: " << film->acteurs.elements[i]->nom << '\n';
            for (int j = 0; j < film->acteurs.elements[i]->joueDans.getNElements(); ++j) {
                film->acteurs.elements[i]->joueDans.enleverFilmListeFilms(*this, film->acteurs.elements[i]->joueDans.getElements()[j]);
            }
            delete[] film->acteurs.elements[i]->joueDans.getElements();
            film->acteurs.elements[i]->joueDans.setElements(nullptr);
            delete film->acteurs.elements[i];
            film->acteurs.elements[i] = nullptr;
        }
    }
    delete[] film->acteurs.elements;
    film->acteurs.elements = nullptr;
    delete film;
    film = nullptr;
}

void ListeFilms::detruireListeFilms(ListeFilms& listeFilms) {
    for (int i = 0; i < nElements_; ++i) {
        if (!elements_[i]->titre.empty()) {
            detruireMemoireFilm(elements_[i]);
        }
    }
    delete[] elements_;
    setElements(nullptr);
}

void afficherActeur(const Acteur& acteur)
{
    bool acteurPresent = (acteur.sexe == 'M') || (acteur.sexe == 'F');
    if (acteurPresent) {
        cout << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
    }
}

void afficherFilm(const Film& film) {
    cout << film.titre << '\n';
    for (auto acteur : span(film.acteurs.elements, film.acteurs.nElements)) {
        afficherActeur(*acteur);
    }

}


void ListeFilms::afficherListeFilms(const ListeFilms& listeFilms) const {
    static const string ligneDeSeparation = "----------------------------------------\n";
    cout << ligneDeSeparation;

    for (int i = 0; i < listeFilms.nElements_; ++i) {
        afficherFilm(*listeFilms.elements_[i]);
        cout << ligneDeSeparation;
    }
}



void afficherFilmographieActeur(const ListeFilms& listeFilms, const string& nomActeur)
{

    const Acteur* acteur = trouverActeurListeFilms(listeFilms, nomActeur);
    if (acteur == nullptr)
        cout << "Aucun acteur de ce nom" << endl;
    else
        listeFilms.afficherListeFilms(acteur->joueDans);
}


int main()
{
    bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.

    static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";


    //TODO: La ligne suivante devrait lire le fichier binaire en allouant la mémoire nécessaire.
    // Devrait afficher les noms de 20 acteurs sans doublons
    // (par l'affichage pour fins de débogage dans votre fonction lireActeur).
    ListeFilms listeFilms = listeFilms.creerListe("films.bin");


    cout << ligneDeSeparation << "Le premier film de la liste est:" << endl;
    //TODO: Afficher le premier film de la liste.  Devrait être Alien.
    afficherFilm(*listeFilms.getElements()[0]);

    cout << ligneDeSeparation << "Les films sont:" << endl;
    //TODO: Afficher la liste des films.  Il devrait y en avoir 7.
    listeFilms.afficherListeFilms(listeFilms);

    //TODO: Modifier l'année de naissance de Benedict Cumberbatch pour être 1976 (elle était 0 dans les données lues du fichier).
    Acteur* benedict_Cumberbacth = trouverActeurListeFilms(listeFilms, "Benedict Cumberbatch");
    benedict_Cumberbacth->anneeNaissance = 1976;

    cout << ligneDeSeparation << "Liste des films où Benedict Cumberbatch joue sont:" << endl;
    //TODO: Afficher la liste des films où Benedict Cumberbatch joue.
    // Il devrait y avoir Le Hobbit et Le jeu de l'imitation.
    afficherFilmographieActeur(listeFilms, benedict_Cumberbacth->nom);

    //TODO: Détruire et enlever le premier film de la liste (Alien).
    // Ceci devrait "automatiquement" (par ce que font vos fonctions) détruire les acteurs
    // Tom Skerritt et John Hurt, mais pas Sigourney Weaver puisqu'elle joue aussi dans Avatar.
    listeFilms.detruireMemoireFilm(listeFilms.getElements()[0]);
    cout << ligneDeSeparation << "Les films sont maintenant:" << endl;
    //TODO: Afficher la liste des films.
    listeFilms.afficherListeFilms(listeFilms);

    //TODO: Faire les appels qui manquent pour avoir 0% de lignes non exécutées dans le programme
    // (aucune ligne rouge dans la couverture de code; c'est normal que les lignes de "new"
    // et "delete" soient jaunes).  Vous avez aussi le droit d'effacer les lignes du programmes
    // qui ne sont pas exécutée, si finalement vous pensez qu'elle ne sont pas utiles.

    //TODO: Détruire tout avant de terminer le programme.
    // La bibliothèque de verification_allocation devrait afficher "Aucune fuite detectee."
    // a la sortie du programme; il affichera "Fuite detectee:" avec la liste des blocs, s'il manque
    // des delete.
    listeFilms.detruireListeFilms(listeFilms);
}
