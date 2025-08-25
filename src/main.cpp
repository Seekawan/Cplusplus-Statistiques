// Les fichiers .h de mes autres classes
#include "SpotifyDataset.h"
#include "StatDesc.h"
#include "StatInfer.h"

// Les bibliothèques necessaires à la lecture et sauvegarde de fichier + vector
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

// CODES COULEUR ANSI (vert rétro)
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_RESET  "\033[0m"
#define COLOR_BOLD   "\033[1m"

// Garde en mémoire le dernier résultat affiché (pour la commande "save")
std::string lastResult; // Pour la sauvegarde

// ------------------------------------------------------------
// Commandes "desc" : stats descriptives sur un attribut
// Usage : desc [mean|median|mode|min|max|variance|stddev] [attribut]
// ------------------------------------------------------------
void handleDescCommand(const SpotifyDataset& dataset, const std::vector<std::string>& args, std::string& lastResult) {
    if (args.size() != 3) {
        lastResult = "Usage : desc [mean|median|mode|min|max|variance|stddev|amplitude] [attribut]\n";
        std::cout << lastResult;
        return;
    }
    std::ostringstream oss;
    std::string stat = args[1];
    std::string attr = args[2];

    // Récupère toutes les valeurs de l'attribut voulu
    std::vector<double> data = dataset.getAttribute(attr);
    if (data.empty()) {
        lastResult = "Attribut inconnu ou vide.\n";
        std::cout << lastResult;
        return;
    }

    // Applique la statistique demandée
    if (stat == "mean") 
        oss << "Moyenne de " << attr << ": " << StatDesc::mean(data) << '\n';
    else if (stat == "median")
        oss << "Mediane de " << attr << ": " << StatDesc::median(data) << '\n';
    else if (stat == "mode") {
        std::vector<double> modes = StatDesc::mode(data);
        oss << "Mode(s) de " << attr << ": ";
        for (double m : modes) oss << m << " ";
        oss << '\n';
    }
    else if (stat == "min")
        oss << "Minimum de " << attr << ": " << StatDesc::min(data) << '\n';
    else if (stat == "max")
        oss << "Maximum de " << attr << ": " << StatDesc::max(data) << '\n';
    else if (stat == "amplitude")
        oss << "Amplitude de " << attr << ": " << StatDesc::amplitude(data) << '\n';
    else if (stat == "variance")
        oss << "Variance de " << attr << ": " << StatDesc::variance(data) << '\n';
    else if (stat == "stddev" || stat == "ecarttype")
        oss << "Ecart-type de " << attr << ": " << StatDesc::stddev(data) << '\n';
    else
        oss << "Stat inconnue.\n";

    lastResult = oss.str();
    std::cout << lastResult;   
}

// ------------------------------------------------------------
// Commandes "top" : classements
//  - top N [attribut]
//  - top gapleadfeature N
// ------------------------------------------------------------
void handleTopCommand(const SpotifyDataset& dataset, const std::vector<std::string>& args, std::string& lastResult) {
    std::ostringstream oss;
    if (!(args.size() == 3 || args.size() == 4)) {
        oss << "Usage : top 10 [attribut]\n      ou top gapleadfeature 10\n";
        lastResult = oss.str(); std::cout << lastResult; return;
    }
    // Cas "top gapleadfeature N"
    if (args[1] == "gapleadfeature" && args.size() == 3) {
        int n = std::stoi(args[2]);
        auto top = StatDesc::topGapLeadFeature(dataset.getArtists(), n);
        oss << "Top " << n << " ecart |asLead - asFeature|:\n";
        int i = 1;
        for (const auto& a : top)
            oss << i++ << ". " << a.getName()
                      << " (asLead=" << a.getAsLead()
                      << ", asFeature=" << a.getAsFeature()
                      << ", ecart=" << std::abs(a.getAsLead()-a.getAsFeature()) << ")\n";
        lastResult = oss.str(); std::cout << lastResult; return;
    }

    // Cas "top N attribut"
    int n = std::stoi(args[1]);
    std::string attr = args[2];
    auto top = StatDesc::topN(dataset.getArtists(), n, attr);
    oss << "Top " << n << " artistes selon " << attr << " :\n";
    int i = 1;
    for (const auto& a : top){
        oss << i++ << ". " << a.getName() << " (" << attr << " = ";
        if      (attr == "streams")     oss << a.getStreams();
        else if (attr == "daily")       oss << a.getDaily();
        else if (attr == "solo")        oss << a.getSolo();
        else if (attr == "aslead" || attr == "as_lead") oss << a.getAsLead();
        else if (attr == "asfeature" || attr == "as_feature") oss << a.getAsFeature();
        oss << ")\n";
    }
    lastResult = oss.str(); std::cout << lastResult;
}

// ------------------------------------------------------------
// Affichage du ratio solo/feature par artiste
// ------------------------------------------------------------
void handleRepartitionCommand(const SpotifyDataset& dataset, const std::vector<std::string>& args, std::string& lastResult) {
    if(args.size() != 1) {
        std::cout << "Usage : repartition\n";
        return;
    }
    // Affiche directement sur la sortie standard
    StatDesc::printSoloFeatureRatio(dataset.getArtists());
}

// ------------------------------------------------------------
// Affichage de la répartition globale solo/feature
// ------------------------------------------------------------
void handleGlobalRepartitionCommand(const SpotifyDataset& dataset, const std::vector<std::string>& args, std::string& lastResult) {
    if(args.size() != 2 || args[1] != "global") {
        std::cout << "Usage : repartition global\n";
        return;
    }
    // Affiche directement
    StatDesc::printGlobalSoloFeatureRatio(dataset.getArtists());
}

// ------------------------------------------------------------
// IC sur la moyenne d'un attribut (95%)
// ------------------------------------------------------------
void handleICMeanCommand(const SpotifyDataset& dataset, const std::vector<std::string>& args, std::string& lastResult) {
    if (args.size() != 3) {
        std::cout << "Usage : ic mean [attribut]\n";
        return;
    }
    auto data = dataset.getAttribute(args[2]);
    double demiLargeur = StatInfer::intervalleConfianceMoyenne(data); // 95% => z=1,96
    double moyenne = StatDesc::mean(data);
    std::ostringstream oss;
    oss << "IC 95% pour la moyenne de " << args[2] << " : [" 
        << (moyenne-demiLargeur) << " ; " << (moyenne+demiLargeur) << "]\n";
    lastResult = oss.str();
    std::cout << lastResult;
}

// ------------------------------------------------------------
// IC sur une proportion (95%), pour "x > seuil"
// ------------------------------------------------------------
void handleICPropCommand(const SpotifyDataset& dataset, const std::vector<std::string>& args, std::string& lastResult) {
    if (args.size() != 4) {
        std::cout << "Usage : ic prop [attribut] [seuil]\n";
        return;
    }
    double seuil = std::stod(args[3]);
    auto data = dataset.getAttribute(args[2]);
    int nb = 0;
    for(auto x : data) if(x > seuil) nb++;
    int n = data.size();
    double demiLargeur = StatInfer::intervalleConfianceProportion(nb, n);
    double prop = n==0 ? 0 : (nb/(double)n);
    std::ostringstream oss;
    oss << "IC 95% pour la proportion d'artistes avec " << args[2] << " > " << seuil << " : ["
        << (prop - demiLargeur) << " ; " << (prop + demiLargeur) << "]\n";
    lastResult = oss.str();
    std::cout << lastResult;
}

// ------------------------------------------------------------
// Test de proportion (z-test) : H0: p = p0
// ------------------------------------------------------------
void handleTestPropCommand(const SpotifyDataset& dataset, const std::vector<std::string>& args, std::string& lastResult) {
    if (args.size() != 5) {
        std::cout << "Usage : test testprop [attribut] [seuil] [proportion_attendue]\n";
        return;
    }
    std::string attr = args[2];
    double seuil = std::stod(args[3]);
    double p0 = std::stod(args[4]);
    auto data = dataset.getAttribute(attr);
    int nb = 0;
    for(auto x : data) if(x > seuil) nb++;
    int n = data.size();
    double z = StatInfer::testProportion(nb, n, p0);

    std::ostringstream oss;
    oss << "Test de proportion (H0: p = " << p0 << ") :\n";
    oss << "z = " << z << " (>1.96 ou <-1.96 = significatif à 5%)\n";
    lastResult = oss.str();
    std::cout << lastResult;
}

// ------------------------------------------------------------
// Outils de découpage de commande / sauvegarde
// ------------------------------------------------------------
std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

void saveToFile(const std::string& filename, const std::string& content) {
    std::ofstream out(filename);
    if (!out) { std::cout << "Erreur d'ouverture du fichier : " << filename << "\n"; return; }
    out << content;
    out.close();
    std::cout << "Résultat(s) sauvegardé(s) dans " << filename << "\n";
}

// ------------------------------------------------------------
// Menu (affichage console)
// ------------------------------------------------------------
void showMenu() {
    std::cout << COLOR_GREEN;
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "    ANALYSE SPOTIFY - DATA MINING C++   \n";
    std::cout << "========================================\n";
    std::cout << "Commandes disponibles :\n";
    std::cout << " " << COLOR_BOLD << "desc [stat] [attribut]" << COLOR_RESET << COLOR_GREEN << "      (ex: desc mean streams; stats: mean/median/mode/min/max/variance/stddev/amplitude)\n";
    std::cout << " " << COLOR_BOLD << "top N [attribut]" << COLOR_RESET << COLOR_GREEN << "            (ex: top 10 streams)\n";
    std::cout << " " << COLOR_BOLD << "top gapleadfeature N" << COLOR_RESET << COLOR_GREEN << "   (plus grand ecart lead/feature)\n";
    std::cout << " " << COLOR_BOLD << "repartition" << COLOR_RESET << COLOR_GREEN << "                (ratio solo/feature par artiste)\n";
    std::cout << " " << COLOR_BOLD << "repartition global" << COLOR_RESET << COLOR_GREEN << "         (ratio global)\n";
    std::cout << " " << COLOR_BOLD << "proba top N [attr]" << COLOR_RESET << COLOR_GREEN << "        (ex: proba top 10 streams, modele uniforme: n/N)\n";
    std::cout << " " << COLOR_BOLD << "proba solo70" << COLOR_RESET << COLOR_GREEN << "               (proba >70% solo)\n";
    std::cout << " " << COLOR_BOLD << "proba condtop10daily seuil" << COLOR_RESET << COLOR_GREEN << "\n";
    std::cout << " " << COLOR_BOLD << "regression X Y [plot]" << COLOR_RESET << COLOR_GREEN << "             (ex: regression streams solo)\n";
    std::cout << " " << COLOR_BOLD << "correlation X Y" << COLOR_RESET << COLOR_GREEN << "           (ex: correlation solo asfeature)\n";
    std::cout << " " << COLOR_BOLD << "ic mean [attribut]" << COLOR_RESET << COLOR_GREEN << "             (IC sur la moyenne)\n";
    std::cout << " " << COLOR_BOLD << "ic prop [attribut] [seuil]" << COLOR_RESET << COLOR_GREEN << "    (IC sur une proportion)\n";
    std::cout << " " << COLOR_BOLD << "test testprop [attribut] [seuil] [prop]" << COLOR_RESET << COLOR_GREEN << "  (z-test de proportion)\n";
    std::cout << " " << COLOR_BOLD << "test ttestsolofeature" << COLOR_RESET << COLOR_GREEN << "      (test de moyenne)\n";
    std::cout << " " << COLOR_BOLD << "save" << COLOR_RESET << COLOR_GREEN << "                           (sauvegarder dernier affichage)\n";
    std::cout << " " << COLOR_BOLD << "exit | quit" << COLOR_RESET << COLOR_GREEN << "                  (quitter)\n";
    std::cout << "----------------------------------------" << COLOR_RESET << "\n";
}

// ------------------------------------------------------------
// Point d'entrée
// ------------------------------------------------------------
int main() {
    SpotifyDataset data;

    // Ouvre un fichier de log et rediriger std::cerr
    std::ofstream logStream("logs", std::ios::out | std::ios::trunc);
    std::streambuf* oldCerrBuf = nullptr;
    if (logStream) {
    oldCerrBuf = std::cerr.rdbuf(logStream.rdbuf());
    } else {
    std::cerr << "Impossible d'ouvrir le fichier de logs.\n";
    }

    // Chargement du CSV (les messages d'import iront dans 'logs')
    if (!data.loadFromCSV("artists.csv")) {
    std::cerr << "Erreur lors de l'ouverture du CSV\n";
    }

    // Restaurer std::cerr
    if (oldCerrBuf) {
    std::cerr.rdbuf(oldCerrBuf);
    logStream.close();
    }

    std::string command;
    while (true) {
        showMenu();
        std::cout << "\nCommande (ou 'exit' pour quitter) : ";
        std::getline(std::cin, command);

        if (command == "exit" || command == "quit") {
            break;
        }
        auto tokens = split(command);

        if (tokens.empty()) continue;

        // --- Commande "desc" ---
        if (tokens[0] == "desc") {
            handleDescCommand(data, tokens, lastResult);
        }
        // --- Commande "top" ---
        else if(tokens[0]=="top") handleTopCommand(data, tokens, lastResult);
        // --- "repartition global" ---
        else if(tokens[0]=="repartition" && tokens.size()>1 && tokens[1]=="global")
        handleGlobalRepartitionCommand(data, tokens, lastResult);
        // --- "repartition" ---
        else if(tokens[0]=="repartition")
        handleRepartitionCommand(data, tokens, lastResult);
        // --- "proba top N attr" ---
        else if (tokens[0] == "proba" && tokens.size() == 4 && tokens[1] == "top") {
            int n = std::stoi(tokens[2]);
            double proba = StatInfer::probaTopN(data.getArtists(), n, tokens[3]);
            std::ostringstream oss;
            oss << "Proba d'etre dans le top " << n << " de " << tokens[3]
            << " (modele uniforme n/N): " << proba << "\n"; 
            lastResult = oss.str();
            std::cout << lastResult;
        } 
        // --- "proba solo70" ---
        else if (tokens[0] == "proba" && tokens[1] == "solo70") {
            double proba = StatInfer::probaParSoloRatio(data.getArtists(), 0.70);
            std::ostringstream oss;
            oss << "Proba qu'un artiste ait >70% de streams solo: " << proba << "\n";
            lastResult = oss.str();
            std::cout << lastResult;
        }
        // --- "proba condtop10daily seuil" ---
        else if (tokens[0] == "proba" && tokens[1] == "condtop10daily" && tokens.size() == 3) {
            double seuil = std::stod(tokens[2]);
            double proba = StatInfer::probaCondTopNdaily_given_highStreams(data.getArtists(), seuil, 10);
            std::ostringstream oss;
            oss << "Proba(d'etre dans le top10 daily GLOBAL | streams > " << seuil << ") = " << proba << "\n";
            lastResult = oss.str();
            std::cout << lastResult;
        } 
        // --- "regression X Y" (première occurrence) ---
        else if (tokens[0] == "regression" && (tokens.size() == 3 || tokens.size() == 4)) {
            auto x = data.getAttribute(tokens[1]);
            auto y = data.getAttribute(tokens[2]);
            double a, b, r2;
            StatInfer::regressionLineaire(x, y, a, b, r2);
            // Résidus
            std::vector<double> resid;
            resid.reserve(x.size());
            for (size_t i = 0; i < x.size() && i < y.size(); ++i)
                resid.push_back(y[i] - (a * x[i] + b));

            double rmean = StatDesc::mean(resid);
            double rstd  = StatDesc::stddev(resid);
            double rmin  = resid.empty() ? 0.0 : *std::min(resid.begin(), resid.end());
            double rmax  = resid.empty() ? 0.0 : *std::max(resid.begin(), resid.end());

            std::ostringstream oss;
            oss << "Regression " << tokens[1] << " -> " << tokens[2] << "\n"
                << "Y = " << a << " * X + " << b << " ; R^2 = " << r2 << "\n"
                << "Residuals: mean=" << rmean << ", std=" << rstd
                << ", min=" << rmin << ", max=" << rmax << "\n";
            if (tokens.size() == 4 && tokens[3] == "plot") {
                oss << "(Graphe ASCII affiche)\n";
            }
            lastResult = oss.str();
            std::cout << lastResult;

            if (tokens.size() == 4 && tokens[3] == "plot") {
                StatInfer::regressionAsciiPlot(x, y, a, b); // trace sur stdout
            }
        }
        // --- "correlation X Y" ---
        else if (tokens[0] == "correlation" && tokens.size() == 3) {
        auto x = data.getAttribute(tokens[1]);
        auto y = data.getAttribute(tokens[2]);
        double corr = StatInfer::pearson(x, y);
        std::ostringstream oss;
        oss << "Correlation de Pearson entre " << tokens[1] << " et " << tokens[2] << " : " << corr << "\n";
        lastResult = oss.str();
        std::cout << lastResult;
        }
        // --- "test ttestsolofeature" ---
        else if (tokens[0] == "test" && tokens[1] == "ttestsolofeature") {
        auto solo = data.getAttribute("solo");
        auto feat = data.getAttribute("asfeature");
        double tstat = StatInfer::ttest2moyennes(solo, feat);
        std::ostringstream oss;
        oss << "T-statistique pour comparaison des moyennes (solo vs feature) : " << tstat
        << " (|t| >= ~2 => significatif a 5% environ)\n";
        lastResult = oss.str();
        std::cout << lastResult;
        }
        // --- "ic mean attr" ---
        else if (tokens[0] == "ic" && tokens[1] == "mean")
            handleICMeanCommand(data, tokens, lastResult);

        // --- "ic prop attr seuil" ---
        else if (tokens[0] == "ic" && tokens[1] == "prop")
            handleICPropCommand(data, tokens, lastResult);

        // --- "test testprop attr seuil p0" ---
        else if (tokens[0] == "test" && tokens[1] == "testprop")
            handleTestPropCommand(data, tokens, lastResult);

        // --- "save" : sauvegarde lastResult dans un fichier ---
        else if (tokens[0]=="save") {
        std::cout << "Nom du fichier de sortie ? ";
        std::string filename; std::getline(std::cin, filename);
        saveToFile(filename, lastResult);
        }
        else {
            std::cout << "Commande inconnue.\n";
        }
    }
    return 0;
}
