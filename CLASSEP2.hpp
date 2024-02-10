#pragma once
// Structures mémoires pour une collection de films.

#include <string>

struct Film; struct Acteur; // Permet d'utiliser les types alors qu'ils seront défini après.

class ListeFilms {
public:

    //initialisateur et destructeur
    ListeFilms();
    ~ListeFilms();
    
    //trouver chercher les valeurs des elements
    int trouverNElements() const;
    Film** trouverElements() const;

    //Methode de la classe nessessaires pour son fonctionnement
    void afficherFilmographieActeur(const ListeFilms& listeFilms, const std::string& nomActeur);
    void ajouterFilmListeFilms(ListeFilms& listeFilms, Film* film);
    void enleverFilmListeFilms(ListeFilms& listeFilms, Film* film);
    void afficherListeFilms(const ListeFilms& listeFilms) const;
    void detruireListeFilms(ListeFilms& listeFilms);
    void detruireFilm(Film* film, ListeFilms& listeFilms);
    ListeFilms creerListe(std::string nomFichier);

private:
    // attributs privees de la classe
    int capacite_;
    int nElements_;
    Film** elements_ = {};

};

struct ListeActeurs {
    int capacite, nElements;
    Acteur** elements; // Pointeur vers un tableau de Acteur*, chaque Acteur* pointant vers un Acteur.
};

struct Film
{
    std::string titre, realisateur; // Titre et nom du réalisateur (on suppose qu'il n'y a qu'un réalisateur).
    int anneeSortie, recette; // Année de sortie et recette globale du film en millions de dollars
    ListeActeurs acteurs;
};

struct Acteur
{
    std::string nom; int anneeNaissance; char sexe;
    ListeFilms joueDans;
};
