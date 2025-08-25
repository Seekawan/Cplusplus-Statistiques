#pragma once
#include "Artist.h"
#include <vector>
#include <string>
#include <unordered_set>

/*
  StatInfer : fonctions d'inférence/statistiques (probabilités simples,
  intervalles de confiance, tests, régression, corrélation).
*/
class StatInfer {
public:
    // PROBABILITÉS 
    static double probaTopN(const std::vector<Artist>&, int n, const std::string& attr);
    static double probaParSoloRatio(const std::vector<Artist>&, double seuilRatio);
    static double probaCondTopNdaily_given_highStreams(const std::vector<Artist>&, double seuilStreams, int n);

    // ESTIMATIONS (IC 95% approximatifs)
    static double intervalleConfianceMoyenne(const std::vector<double>&, double alpha=0.05);
    static double intervalleConfianceProportion(int nbSuccess, int nbTotal, double alpha=0.05);

    // TESTS (t-test, test de proportion) – renvoient la statistique de test
    static double ttest2moyennes(const std::vector<double>&, const std::vector<double>&);
    static double testProportion(int nbSuccess, int nbTotal, double prop0);

    // RÉGRESSION LINÉAIRE (Y = aX + b) + coefficient de détermination R²
    static void regressionLineaire(const std::vector<double>& X, const std::vector<double>& Y, double& a, double& b, double& r2);

    // CORRÉLATION DE PEARSON
    static double pearson(const std::vector<double>&, const std::vector<double>&);

    // TRACE ASCII d'une régression (nuage + droite ajustée)
    static void regressionAsciiPlot(const std::vector<double>& X, const std::vector<double>& Y, double a, double b, int width=60, int height=20);
};
