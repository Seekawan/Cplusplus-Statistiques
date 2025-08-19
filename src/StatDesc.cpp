#include "StatDesc.h"
#include <algorithm>
#include <map>
#include <cmath>
#include <iostream>
#include <iomanip>

// --- MOYENNE ---
// Somme / n, renvoie 0.0 si data est vide.
double StatDesc::mean(const std::vector<double>& data) {
    double sum = 0.0;
    for (double x : data) sum += x;
    return data.empty() ? 0.0 : sum / data.size();
}

// --- MEDIANE ---
// Trie une copie du vecteur puis renvoie l'élément central (ou moyenne des deux centraux)
double StatDesc::median(std::vector<double> data) {
    if (data.empty()) return 0.0;
    std::sort(data.begin(), data.end());
    size_t n = data.size();
    if (n % 2 == 0) return (data[n/2 - 1] + data[n/2]) / 2.0;
    else return data[n/2];
}

// --- MODE ---
// Compte les fréquences avec std::map, puis récupère la ou les valeurs de fréquence max
std::vector<double> StatDesc::mode(const std::vector<double>& data) {
    std::map<double, int> freq;
    for (double x : data) freq[x]++;
    int maxFreq = 0;
    for (const auto& p : freq)
        if (p.second > maxFreq) maxFreq = p.second;

    std::vector<double> modes;
    for (const auto& p : freq)
        if (p.second == maxFreq) modes.push_back(p.first);

    return modes;
}

// --- MIN ---
double StatDesc::min(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    return *std::min_element(data.begin(), data.end());
}

// --- MAX ---
double StatDesc::max(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    return *std::max_element(data.begin(), data.end());
}

// --- AMPLITUDE ---
double StatDesc::amplitude(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    auto mm = std::minmax_element(data.begin(), data.end());
    return *mm.second - *mm.first;
}

// --- VARIANCE ---
// Somme des (x-m)^2, divisée par (n-1) si sample=true, sinon par n.
// Si data.size()<2 -> 0.0
double StatDesc::variance(const std::vector<double>& data, bool sample) {
    if (data.size() < 2) return 0.0;
    double m = mean(data);
    double var = 0.0;
    for (double x : data) var += (x-m)*(x-m);
    return var / (data.size() - (sample ? 1 : 0));
}

// --- ECART-TYPE ---
// Racine carrée de la variance
double StatDesc::stddev(const std::vector<double>& data, bool sample) {
    return std::sqrt(variance(data, sample));
}

// --- TOP N ---
// Copie le vecteur d'artistes, trie selon l'attribut demandé (ordre décroissant), puis prend les N premiers.
// Si attr est inconnu, av et bv restent à 0 -> l'ordre résultant sera arbitraire.
std::vector<Artist> StatDesc::topN(const std::vector<Artist>& artists, int n, const std::string& attr) {
    std::vector<Artist> sorted = artists;
    std::sort(sorted.begin(), sorted.end(), [attr](const Artist& a, const Artist& b) {
        double av = 0, bv = 0;
        if      (attr == "streams")    { av=a.getStreams();    bv=b.getStreams();}
        else if (attr == "daily")      { av=a.getDaily();      bv=b.getDaily();}
        else if (attr == "solo")       { av=a.getSolo();       bv=b.getSolo();}
        else if (attr == "aslead" || attr == "as_lead")   { av=a.getAsLead();     bv=b.getAsLead();}
        else if (attr == "asfeature" || attr == "as_feature") { av=a.getAsFeature(); bv=b.getAsFeature();}
        return av > bv; // Trie du plus grand au plus petit
    });
    if (n > (int)sorted.size()) n = sorted.size();
    return std::vector<Artist>(sorted.begin(), sorted.begin() + n);
}

// --- TOP GAP LEAD/FEATURE ---
// Classe les artistes selon l'écart absolu entre asLead et asFeature
std::vector<Artist> StatDesc::topGapLeadFeature(const std::vector<Artist>& artists, int n) {
    std::vector<std::pair<double, const Artist*>> byGap;
    for(const Artist& a : artists)
        byGap.push_back({std::abs(a.getAsLead()-a.getAsFeature()), &a});
    std::sort(byGap.begin(), byGap.end(), [](const auto& x, const auto& y){
        return x.first > y.first;
    });
    std::vector<Artist> res;
    for(int i=0; i<n && i<(int)byGap.size(); ++i) res.push_back(*byGap[i].second);
    return res;
}

// --- AFFICHAGE : ratio solo/feature par artiste ---
// Pour chaque artiste : affiche %solo et %feature, basés sur le total de streams de l'artiste.
void StatDesc::printSoloFeatureRatio(const std::vector<Artist>& artists) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Artiste                  %solo   %feature\n";
    std::cout << "------------------------------------------------\n";
    for (const Artist& a : artists) {
        double total = a.getStreams();
        if (total == 0.0) continue; // éviter division par 0
        double psolo = 100.0 * a.getSolo() / total;
        double pfeat = 100.0 * a.getAsFeature() / total;
        std::cout << std::setw(22) << std::left << a.getName()
                  << std::setw(8) << psolo 
                  << std::setw(8) << pfeat << '\n';
    }
}

// --- AFFICHAGE : répartition globale ---
// Calcule les % sur la somme globale des streams (solo, feature, autre = reste)
void StatDesc::printGlobalSoloFeatureRatio(const std::vector<Artist>& artists) {
    double total = 0.0, solo = 0.0, feature = 0.0;
    for(const Artist& a : artists) {
        total += a.getStreams();
        solo += a.getSolo();
        feature += a.getAsFeature();
    }
    if (total == 0.0) {
        std::cout << "Aucune donnée.\n"; return;
    }
    double psolo = 100.0 * solo / total;
    double pfeat = 100.0 * feature / total;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Répartition globale des streams:\n";
    std::cout << "% solo: " << psolo << "\n";
    std::cout << "% feature: " << pfeat << "\n";
    std::cout << "Autre: " << (100.0 - psolo - pfeat) << "\n";
}
