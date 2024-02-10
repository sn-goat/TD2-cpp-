#pragma once
// Structures mémoires pour une collection de films.

#include <string>

struct Film; struct Acteur; // Permet d'utiliser les types alors qu'ils seront défini après.

class ListeFilms {
public:
    ListeFilms();
    ~ListeFilms();

    int getCapacite() const;
    int getNElements() const;
    Film** getElements() const;


    void setCapacite(int capacite);
    void setElements(Film** elements);
    Acteur* trouverActeurListeFilms(const string& nomActeur);
    void ajouterFilmListeFilms( Film* film);
    void enleverFilmListeFilms( Film* film);
    void afficherListeFilms(const ListeFilms& listeFilms) const;
    void detruireListeFilms(ListeFilms& listeFilms);
    void detruireMemoireFilm(Film* film);
    ListeFilms creerListe(std::string nomFichier);

private:
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





