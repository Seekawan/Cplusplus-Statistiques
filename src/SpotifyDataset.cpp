#include "SpotifyDataset.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

// ----------- Helpers -----------

std::string SpotifyDataset::trim(const std::string& s) {
    auto is_space = [](unsigned char c) {
        return std::isspace(c) || c == '\r' || c == '\n';
    };
    size_t b = 0, e = s.size();
    while (b < e && is_space((unsigned char)s[b])) ++b;
    while (e > b && is_space((unsigned char)s[e-1])) --e;
    return s.substr(b, e - b);
}

std::string SpotifyDataset::normalizeKey(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (std::isalnum(c)) out.push_back((char)std::tolower(c));
        // on ignore espaces, underscores, tirets...
    }
    return out;
}

// Parser CSV conforme RFC 4180 (guillemets, "" -> ").
std::vector<std::string> SpotifyDataset::parseCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string cur;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char ch = line[i];
        if (ch == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                // guillemet échappé
                cur.push_back('"');
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (ch == ',' && !inQuotes) {
            fields.push_back(trim(cur));
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    fields.push_back(trim(cur));
    return fields;
}

// Mappe les colonnes depuis la première ligne si c'est un header.
// Heuristique: si >=3 noms reconnus, on considère que c'est un header.
SpotifyDataset::ColMap SpotifyDataset::buildColumnMap(const std::vector<std::string>& firstRow, bool& isHeader) {
    ColMap map;
    int recognized = 0;
    for (int i = 0; i < (int)firstRow.size(); ++i) {
        std::string key = normalizeKey(firstRow[i]);

        if (key == "artist" || key == "name") { map.artist = i; recognized++; }
        else if (key == "streams" || key == "stream") { map.streams = i; recognized++; }
        else if (key == "daily") { map.daily = i; recognized++; }
        else if (key == "aslead" || key == "lead" || key == "asprincipal") { map.asLead = i; recognized++; }
        else if (key == "solo") { map.solo = i; recognized++; }
        else if (key == "asfeature" || key == "feature" || key == "feat") { map.asFeature = i; recognized++; }
    }
    isHeader = (recognized >= 3);
    return map;
}

// ----------- Parsing numérique robuste -----------

double SpotifyDataset::parseNumber(const std::string& s, int linenumber) const {
    std::string sanitized = trim(s);

    // Valeur vide -> on retourne 0.0 (choix: ignorer la ligne si vous préférez)
    if (sanitized.empty()) return 0.0;

    // Retirer les espaces internes typiques de groupement "60 000"
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
        [](unsigned char c){ return c==' ' || c=='\t'; }), sanitized.end());

    bool hasDot = sanitized.find('.') != std::string::npos;
    bool hasComma = sanitized.find(',') != std::string::npos;

    if (hasComma && !hasDot) {
        // S'il n'y a qu'une virgule et aucun point: on suppose virgule décimale
        if (std::count(sanitized.begin(), sanitized.end(), ',') == 1) {
            std::replace(sanitized.begin(), sanitized.end(), ',', '.');
        } else {
            // Plusieurs virgules: on suppose séparateurs de milliers -> on les enlève
            sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), ','), sanitized.end());
        }
    } else if (hasComma && hasDot) {
        // Les virgules sont probablement des séparateurs de milliers
        sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), ','), sanitized.end());
    }
    // Sinon: ni virgule ni point -> entier "pur"

    try {
        size_t idx = 0;
        double val = std::stod(sanitized, &idx);
        // Vérifier qu'il n'y a pas de traînant non numérique significatif
        if (idx < sanitized.size()) {
            // caractères restants -> log et exception
            std::cerr << "Avertissement ligne " << linenumber
                      << ": caractères inattendus dans '" << s << "'\n";
        }
        return val;
    } catch (const std::exception&) {
        std::cerr << "Erreur à la ligne " << linenumber << ": Valeur non numérique : '" << s << "'\n";
        throw;
    }
}

// ----------- Chargement du CSV -----------

bool SpotifyDataset::loadFromCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    artists.clear();

    std::string line;
    int lineNumber = 0;
    int imported = 0, skipped = 0;

    auto processRow = [&](const std::vector<std::string>& row, int lineno, const ColMap& map) -> bool {
        auto safeGet = [&](int idx) -> std::string {
            return (idx >= 0 && idx < (int)row.size()) ? row[idx] : std::string();
        };

        // Champs requis minimaux: nom + toutes les colonnes numériques
        if (map.artist < 0 || map.streams < 0 || map.daily < 0 ||
            map.asLead < 0 || map.solo < 0 || map.asFeature < 0) {
            std::cerr << "Ligne " << lineno << " ignorée: mapping de colonnes incomplet\n";
            return false;
        }

        try {
            std::string name = trim(safeGet(map.artist));
            if (name.empty()) {
                std::cerr << "Ligne " << lineno << " ignorée: nom d'artiste vide\n";
                return false;
            }

            double streams   = parseNumber(safeGet(map.streams), lineno);
            double daily     = parseNumber(safeGet(map.daily), lineno);
            double asLead    = parseNumber(safeGet(map.asLead), lineno);
            double solo      = parseNumber(safeGet(map.solo), lineno);
            double asFeature = parseNumber(safeGet(map.asFeature), lineno);

            artists.emplace_back(name, streams, daily, asLead, solo, asFeature);
            return true;
        } catch (...) {
            // parseNumber a déjà loggé; on ignore la ligne
            return false;
        }
    };

    // Lire première ligne
    if (!std::getline(file, line)) {
        std::cerr << "Fichier vide.\n";
        return true; // fichier ouvert mais vide
    }
    lineNumber++;
    auto firstRow = parseCSVLine(line);

    bool isHeader = false;
    ColMap map = buildColumnMap(firstRow, isHeader);

    // Fallback: si pas header, mapping par positions fixes
    if (!isHeader) {
        map.artist = 0;
        map.streams = 1;
        map.daily = 2;
        map.asLead = 3;
        map.solo = 4;
        map.asFeature = 5;

        // Traiter la première ligne comme données
        if ((int)firstRow.size() >= 6) {
            if (processRow(firstRow, lineNumber, map)) imported++; else skipped++;
        } else {
            std::cerr << "Ligne " << lineNumber << " ignorée: nombre de colonnes insuffisant (" << firstRow.size() << ")\n";
            skipped++;
        }
    } else {
        // Header: si mapping incomplet, on tentera quand même par positions fixes en secours
        if (!map.complete()) {
            ColMap pos;
            pos.artist = 0; pos.streams = 1; pos.daily = 2; pos.asLead = 3; pos.solo = 4; pos.asFeature = 5;
            // On utilisera 'map' pour l'accès par nom; si une colonne manque (=-1), on essaiera 'pos'
            // en fusionnant dynamiquement dans processRow on garde 'map' tel quel
        }
    }

    // Parcours des lignes restantes
    while (std::getline(file, line)) {
        lineNumber++;
        auto row = parseCSVLine(line);

        // Si la ligne est visiblement vide
        bool allEmpty = true;
        for (auto& f : row) { if (!trim(f).empty()) { allEmpty = false; break; } }
        if (allEmpty) continue;

        if ((int)row.size() < 2) { // moins que 2 colonnes -> clairement corrompue
            std::cerr << "Ligne " << lineNumber << " ignorée: trop peu de colonnes (" << row.size() << ")\n";
            skipped++; continue;
        }

        if (!processRow(row, lineNumber, map)) skipped++; else imported++;
    }

    std::cerr << "Import CSV terminé: " << imported << " ligne(s) importée(s), "
              << skipped << " ignorée(s). Total artistes: " << artists.size() << "\n";
    return true;
}

// ----------- Accès -----------

const std::vector<Artist>& SpotifyDataset::getArtists() const {
    return artists;
}

std::vector<double> SpotifyDataset::getAttribute(const std::string& attr) const {
    std::vector<double> v;
    for (const Artist& a : artists) {
        if      (attr == "streams")                             v.push_back(a.getStreams());
        else if (attr == "daily")                               v.push_back(a.getDaily());
        else if (attr == "solo")                                v.push_back(a.getSolo());
        else if (attr == "aslead" || attr == "as_lead")         v.push_back(a.getAsLead());
        else if (attr == "asfeature" || attr == "as_feature")   v.push_back(a.getAsFeature());
    }
    return v;
}