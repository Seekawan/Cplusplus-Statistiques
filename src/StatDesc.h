#pragma once
#include "Artist.h"
#include <vector>
#include <string>

/*
  StatDesc : statistiques descriptives et classements sur des vecteurs numériques
  et sur le vecteur d'artistes.
*/
class StatDesc {
public:
    // Moyenne arithmétique
    static double mean(const std::vector<double>& data);

    // Médiane (copie et tri du vecteur)
    static double median(std::vector<double> data);

    // Mode(s) : renvoie tous les modes (valeurs les plus fréquentes)
    static std::vector<double> mode(const std::vector<double>& data);

    // Minimum/Maximum (0.0 si data vide)
    static double min(const std::vector<double>& data);
    static double max(const std::vector<double>& data);

    //Amplitude
    static double amplitude(const std::vector<double>& data);

    // Variance : si sample=true, divise par (n-1), sinon par n
    static double variance(const std::vector<double>& data, bool sample=true);

    // Ecart-type : racine de la variance
    static double stddev(const std::vector<double>& data, bool sample=true);

    // Retourne les N premiers artistes selon un attribut (ordre décroissant)
    // attr: "streams", "daily", "solo", "aslead"/"as_lead", "asfeature"/"as_feature"
    static std::vector<Artist> topN(const std::vector<Artist>& artists, int n, const std::string& attr);

    // Classement par plus grand écart absolu entre asLead et asFeature
    static std::vector<Artist> topGapLeadFeature(const std::vector<Artist>& artists, int n);

    // Affichage du % de solo et % de feature par artiste (sur le total de l'artiste)
    static void printSoloFeatureRatio(const std::vector<Artist>& artists);

    // Affichage de la répartition globale (sur la somme de tous les streams du dataset)
    static void printGlobalSoloFeatureRatio(const std::vector<Artist>& artists);
};
