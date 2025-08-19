#pragma once
#include "Artist.h"
#include <vector>
#include <string>

class SpotifyDataset {
private:
    std::vector<Artist> artists;

    // Outils de parsing
    static std::string trim(const std::string& s);
    static std::string normalizeKey(const std::string& s); // "As lead" -> "aslead"
    static std::vector<std::string> parseCSVLine(const std::string& line); // RFC 4180

    struct ColMap {
        int artist = -1;
        int streams = -1;
        int daily = -1;
        int asLead = -1;
        int solo = -1;
        int asFeature = -1;
        bool complete() const {
            return artist>=0 && streams>=0 && daily>=0 && asLead>=0 && solo>=0 && asFeature>=0;
        }
    };
    // Déduit le mapping depuis la première ligne; isHeader=true si noms reconnus
    static ColMap buildColumnMap(const std::vector<std::string>& firstRow, bool& isHeader);

    // Conversion texte -> double avec gestion milliers et virgule décimale
    double parseNumber(const std::string& str, int linenumber) const;

public:
    // Charge les données depuis un CSV. Renvoie true si le fichier s'ouvre (même si des lignes sont ignorées).
    bool loadFromCSV(const std::string& filename);

    const std::vector<Artist>& getArtists() const;

    // "streams", "daily", "solo", "aslead"/"as_lead", "asfeature"/"as_feature"
    std::vector<double> getAttribute(const std::string& attr) const;
};