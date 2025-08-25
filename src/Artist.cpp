#include "Artist.h"

// Constructeur : affectation des membres via liste d'initialisation
Artist::Artist(const std::string& name, double streams, double daily, double asLead, double solo, double asFeature)
    : name(name), streams(streams), daily(daily), asLead(asLead), solo(solo), asFeature(asFeature) {}

// Getters : renvoient les valeurs stockées
const std::string& Artist::getName() const {
    return name;
}

double Artist::getStreams() const {
    return streams;
}

double Artist::getDaily() const {
    return daily;
}

double Artist::getAsLead() const {
    return asLead;
}

double Artist::getSolo() const {
    return solo;
}

double Artist::getAsFeature() const {
    return asFeature;
}
