#pragma once
#include <string>

/*
  Représente un artiste Spotify et ses métriques agrégées.

  Champs (tous en double pour rester cohérent avec les valeurs décimales du CSV) :
    - name       : nom de l'artiste
    - streams    : total de streams
    - daily      : score journalier
    - asLead     : streams en tant qu'artiste principal (lead)
    - solo       : streams en solo
    - asFeature  : streams en tant qu'artiste invité (feature)
*/
class Artist {
private:
    std::string name;
    double streams;
    double daily;
    double asLead;
    double solo;
    double asFeature;

public:
    // Constructeur : initialise tous les champs
    Artist(const std::string& name, double streams, double daily, double asLead, double solo, double asFeature);

    // Getters (accesseurs en lecture uniquement)
    const std::string& getName() const;
    double getStreams() const;
    double getDaily() const;
    double getAsLead() const;
    double getSolo() const;
    double getAsFeature() const;
};
