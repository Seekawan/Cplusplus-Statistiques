#include "StatInfer.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// --- Probabilité d’être dans le top N selon attr ---
// Renvoie n / total, indépendamment de l'attribut.
// Cela représente une proba uniforme si on suppose qu'un artiste choisi au hasard a
// une chance n/total d'être dans le top N.
double StatInfer::probaTopN(const std::vector<Artist>& artists, int n, const std::string& attr) {
    n = std::min(n, (int)artists.size());
    return n / (double)artists.size();
}

// --- Proba qu’un artiste ait un ratio solo > seuil ---
// On parcourt les artistes et on compte ceux dont (solo/streams) > seuilRatio.
double StatInfer::probaParSoloRatio(const std::vector<Artist>& artists, double seuilRatio) {
    int count = 0;
    for(const Artist& a: artists) {
        double soloRatio = (a.getStreams() == 0) ? 0 : (a.getSolo() / a.getStreams());
        if(soloRatio > seuilRatio) count++;
    }
    return artists.empty() ? 0.0 : (count / (double)artists.size());
}

// --- Proba conditionnelle : être top N en daily parmi les artistes dont streams > seuilStreams ---
// Logique : on filtre les artistes (streams > seuil), on trie ce sous-ensemble par daily,
// puis on renvoie n / (taille du sous-ensemble).
double StatInfer::probaCondTopNdaily_given_highStreams(const std::vector<Artist>& artists, double seuilStreams, int n) {
if (artists.empty()) return 0.0;
// 1) Pointeurs vers tous les artistes pour trier par daily (global)
std::vector<const Artist*> all;
all.reserve(artists.size());
for (const Artist& a : artists) all.push_back(&a);

std::sort(all.begin(), all.end(),
          [](const Artist* a, const Artist* b){ return a->getDaily() > b->getDaily(); });

if (n > (int)all.size()) n = (int)all.size();

// 2) Ensemble du top-N global par daily
std::unordered_set<const Artist*> topDaily;
for (int i = 0; i < n; ++i) topDaily.insert(all[i]);

// 3) Filtrer par streams > seuil
std::vector<const Artist*> filtered;
for (const Artist& a : artists)
    if (a.getStreams() > seuilStreams) filtered.push_back(&a);

if (filtered.empty()) return 0.0;

// 4) Proportion des filtrés qui appartiennent au top-N global daily
int countInTop = 0;
for (const Artist* p : filtered)
    if (topDaily.count(p)) countInTop++;

return countInTop / (double)filtered.size();
}

// --- IC sur la moyenne (approx. gaussienne, 95% -> z=1.96) ---
// Renvoie la demi-largeur de l'IC : mean ± demiLargeur
double StatInfer::intervalleConfianceMoyenne(const std::vector<double>& data, double alpha) {
    double m = 0.0, sq = 0.0;
    int n = data.size();
    if(n < 2) return 0.0;
    for(double x : data) m += x; 
    m = m / n;
    for(double x : data) sq += (x-m)*(x-m);
    double s = std::sqrt(sq/(n-1));
    double z = 1.96; // alpha=0.05 (approx). Pour petits n, une loi t serait plus adaptée.
    return z * s / std::sqrt(n); // Demi-largeur
}

// --- IC sur une proportion (approx. normale) ---
// Renvoie la demi-largeur : p̂ ± demiLargeur
double StatInfer::intervalleConfianceProportion(int nbSuccess, int nbTotal, double alpha) {
    if(nbTotal == 0) return 0.0;
    double p = nbSuccess/(double)nbTotal;
    double z = 1.96; // alpha=0.05
    return z * std::sqrt(p * (1 - p) / nbTotal);
}

// --- t-test (deux moyennes, écart-type empirique) ---
// Calcul du t de Welch (sans p-value).
double StatInfer::ttest2moyennes(const std::vector<double>& X, const std::vector<double>& Y) {
    int n1 = X.size(), n2 = Y.size();
    if(n1 < 2 || n2 < 2) return 0.0;
    double m1=0, m2=0, s1=0, s2=0;
    for(double x : X) m1 += x; m1 /= n1;
    for(double y : Y) m2 += y; m2 /= n2;
    for(double x:X) s1+=(x-m1)*(x-m1);
    for(double y:Y) s2+=(y-m2)*(y-m2);
    s1 = std::sqrt(s1/(n1-1));
    s2 = std::sqrt(s2/(n2-1));
    double t = (m1-m2)/std::sqrt((s1*s1)/n1 + (s2*s2)/n2);
    return t; // La p-value n'est pas calculée ici
}

// --- z-test de proportion ---
// Statistique z = (p̂ - p0) / sqrt(p0*(1-p0)/n)
double StatInfer::testProportion(int nbSucc, int nbTotal, double p0) {
    if(nbTotal == 0) return 0.0;
    double phat = nbSucc/(double)nbTotal;
    double z = (phat-p0)/std::sqrt(p0*(1-p0)/nbTotal);
    return z;
}

// --- Régression linéaire simple Y = aX + b, ainsi que R² ---
// a : pente, b : ordonnée à l'origine, r2 : coefficient de détermination.
void StatInfer::regressionLineaire(const std::vector<double>& X, const std::vector<double>& Y, double& a, double& b, double& r2) {
    double mx=0, my=0, sxy=0, sxx=0, syy=0;
    int n = X.size(); if(n==0 || n!=Y.size()) {a=0; b=0; r2=0; return;}
    for(int i=0;i<n;++i){mx+=X[i]; my+=Y[i];}
    mx/=n; my/=n;
    for(int i=0;i<n;++i){
        sxy += (X[i]-mx)*(Y[i]-my);
        sxx += (X[i]-mx)*(X[i]-mx);
        syy += (Y[i]-my)*(Y[i]-my);
    }
    a = (sxx==0) ? 0.0 : sxy/sxx;
    b = my - a*mx;
    double r = (sxx==0||syy==0)?0 : sxy/std::sqrt(sxx*syy);
    r2 = r*r;
}

// --- Corrélation de Pearson ---
// Retourne 0 si tailles incompatibles ou si variance nulle.
double StatInfer::pearson(const std::vector<double>& X, const std::vector<double>& Y) {
    int n = X.size();
    if(n==0 || n!=Y.size()) return 0.0;
    double mx=0, my=0, sx=0, sy=0, num=0;
    for(int i=0;i<n;++i){mx+=X[i];my+=Y[i];}
    mx/=n; my/=n;
    for(int i=0;i<n;++i){
      sx+=(X[i]-mx)*(X[i]-mx);
      sy+=(Y[i]-my)*(Y[i]-my);
      num+=(X[i]-mx)*(Y[i]-my);
    }
    if (sx==0 || sy==0) return 0.0;
    return num/std::sqrt(sx*sy);
}


// --- Représentation ASCII d'un nuage de points et de la droite de régression ---
// Trace le nuage (o) et la droite (x) dans une grille width x height.
void StatInfer::regressionAsciiPlot(const std::vector<double>& X, const std::vector<double>& Y, double a, double b, int width, int height) {
    if(X.empty()||Y.empty()||X.size()!=Y.size())
        return;

    // Détermination des bornes du graphique
    double xmin=*std::min_element(X.begin(),X.end());
    double xmax=*std::max_element(X.begin(),X.end());
    double ymin=*std::min_element(Y.begin(),Y.end());
    double ymax=*std::max_element(Y.begin(),Y.end());

    // Ajout d'une petite marge
    double xbuf = (xmax-xmin)*0.05;
    double ybuf = (ymax-ymin)*0.05;
    xmin -= xbuf; xmax += xbuf; ymin -= ybuf; ymax += ybuf;

    std::vector<std::string> grille(height+1, std::string(width+1, ' '));

    // Tracé des points (o)
    for(size_t i=0; i<X.size(); ++i) {
        int xi = (int)((X[i]-xmin)/(xmax-xmin)*width);
        int yi = height-(int)((Y[i]-ymin)/(ymax-ymin)*height); // inversion verticale
        if(xi<0) xi=0; if(xi>width) xi=width;
        if(yi<0) yi=0; if(yi>height) yi=height;
        grille[yi][xi]='o';
    }

    // Tracé de la droite estimée y = a*x + b (x)
    for(int x=0;x<=width;++x) {
        double xval=xmin+(xmax-xmin)*x/width;
        double yval=a*xval+b;
        int yi = height-(int)((yval-ymin)/(ymax-ymin)*height);
        if(yi>=0&&yi<=height)
            if(grille[yi][x]!='o') grille[yi][x]='x';
    }

    // Affichage
    for(int y=0; y<=height;++y) {
        std::cout << grille[y] << "\n";
    }
    std::cout << "o: donnees, x: droite regression Y=aX+b\n";
}
