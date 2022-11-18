// Copyright 2022 Yannis Guyon
//
// Licensed under the Apache License,  Version 2.0 (the "License");  you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless  required  by  applicable  law  or  agreed  to  in  writing,  software
// distributed  under  the  License   is  distributed  on  an   "AS  IS"  BASIS,
// WITHOUT  WARRANTIES OR CONDITIONS  OF ANY KIND,  either express  or  implied.
// See  the  License  for  the  specific  language  governing  permissions   and
// limitations under the License.

#ifndef IMMOU_MODELE_HPP
#define IMMOU_MODELE_HPP

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace immou {

struct Economie {
  double inflation;  // pourcents annuels
  double AppliquerInflation(int mois, double montant) const {
    return montant * std::pow(1 + inflation / 100, mois / 12);
  }
};

struct Bien {
  Economie economie;
  double surface;               // metres carres Carrez
  double prix;                  // euros
  double frais_notaire;         // euros
  double loyer_hors_charges;    // euros par mois hors charges
  double charges_recuperables;  // euros par mois
  double loyer_charges_comprises(int mois) const {
    return economie.AppliquerInflation(
        mois, loyer_hors_charges + charges_recuperables);
  }
  double charges_copro;  // euros prevus par mois
  double charges_non_recuperables(int mois) const {
    return economie.AppliquerInflation(mois,
                                       charges_copro - charges_recuperables);
  }
  double taxe_fonciere;     // euros lisses par mois
  double gestion_locative;  // pourcents du loyer charges comprises
  double frais_de_gestion(int mois) const {
    return loyer_charges_comprises(mois) * gestion_locative * 0.01;
  }
  double mise_en_location;  // euros par metre carre Carrez
  double mois_entre_locataires;
  double frais_de_mise_en_location(int mois) const {
    return economie.AppliquerInflation(mois, mise_en_location) * surface;
  }

  double frais_amortis_de_mise_en_location(int mois) const  // euros par mois
  {
    return frais_de_mise_en_location(mois) / mois_entre_locataires;
  }
  double travaux_amortis;       // euros par mois
  double assurance_habitation;  // euros par mois (proprietaire non occupant)
};

struct Emprunt {
  double capital;         // euros empruntes
  double frais_dossier;   // euros
  double frais_garantie;  // euros
  double taux;            // nominal annuel en pourcents
  int duree;              // mois
  double assurance;       // euros par mois

  // euros par mois
  double mensualite() const {
    const double coeff = taux * 0.01 / 12;
    // https://www.hellopret.fr/taux-immobilier/calcul-interet-emprunt
    return (capital * coeff) / (1 - std::pow(1 + coeff, -duree));
  }
  // euros
  double cout() const {
    return (mensualite() + assurance) * duree - capital + frais_dossier +
           frais_garantie;
  }
  // euros pour un mois donne
  double interets(int mois) const {
    return interets_et_capital_restant_du(mois).interets;
  }
  // euros pour un mois donne, avant paiement de la mensualite
  double capital_restant_du(int mois) const {
    return interets_et_capital_restant_du(mois).capital_restant_du;
  }

 private:
  struct InteretsEtCapitalRestantDu {
    double interets, capital_restant_du;
  };
  InteretsEtCapitalRestantDu interets_et_capital_restant_du(int mois) const {
    if (mois == 0) return {(taux / 12) * capital / 100, capital};
    if (mois >= duree) return {0, 0};
    // https://aide.horiz.io/hc/fr/articles/360013338600-Calculer-un-cr%C3%A9dit-
    // immobilier-formules-et-explications-d%C3%A9taill%C3%A9es
    InteretsEtCapitalRestantDu mois_precedent =
        interets_et_capital_restant_du(mois - 1);
    double capital_restant_du = mois_precedent.capital_restant_du -
                                (mensualite() - mois_precedent.interets);
    return {(taux / 12) * capital_restant_du / 100, capital_restant_du};
  }
};

struct Profil {
  double revenu_fiscal_de_reference;  // euros declares l'annee precedente
  double taux_marginal() const {
    if (revenu_fiscal_de_reference >= 160336) return 0.45;
    if (revenu_fiscal_de_reference >= 74545) return 0.41;
    if (revenu_fiscal_de_reference >= 26070) return 0.3;
    if (revenu_fiscal_de_reference >= 10225) return 0.11;
    return 0;
  }
};

class Simulation {
  // TODO: https://www.economie.gouv.fr/particuliers/tout-savoir-deficit-foncier
 public:
  // euros par mois
  double revenu_brut(int mois) const {
    return economie.AppliquerInflation(mois, bien.loyer_hors_charges);
  }
  // euros pour un mois donne
  double revenu_net(int mois) const {
    double net = revenu_brut(mois);
    net -= bien.charges_non_recuperables(mois);
    net -= economie.AppliquerInflation(mois, bien.taxe_fonciere);
    net -= bien.frais_de_gestion(mois);
    net -= bien.frais_amortis_de_mise_en_location(mois);
    net -= economie.AppliquerInflation(mois, bien.travaux_amortis);
    net -= economie.AppliquerInflation(mois, bien.assurance_habitation);
    if (mois < emprunt.duree) {
      net -= emprunt.interets(mois);
    }
    return std::max(0.0, net);
  }
  double revenu_net_avec_frais_amortis(int mois) const {
    double net = revenu_net(mois);
    if (mois < emprunt.duree) {
      double frais = emprunt.frais_dossier + emprunt.frais_garantie;
      for (int a = 0; a < mois; ++a) {
        frais -= std::min(frais, revenu_net(a));
      }
      net -= std::min(frais, net);
    }
    return net;
  }
  double csg_deductible(int mois) const {
    if (mois < 12) return 0;
    return revenu_net_avec_frais_amortis(mois - 12) * 0.068;
  }
  double prelevements_sociaux(int mois) const {
    return revenu_net_avec_frais_amortis(mois) *
           0.172;  // - csg_deductible(mois) ?
  }
  double impot_sur_le_revenu(int mois) const {
    return std::max(
               0., revenu_net_avec_frais_amortis(mois) - csg_deductible(mois)) *
           profil.taux_marginal();
  }
  // double revenu_net_net(int mois) const {
  //   return std::max(0., revenu_net(mois) - prelevements_sociaux(mois) -
  //                           impot_sur_le_revenu(mois));
  // }

  double recettes(int mois) const { return bien.loyer_charges_comprises(mois); }
  double depenses(int mois) const {
    double montant = 0;
    montant += bien.taxe_fonciere;
    montant += economie.AppliquerInflation(mois, bien.charges_copro);
    montant += bien.frais_de_gestion(mois);
    montant += bien.frais_amortis_de_mise_en_location(mois);
    montant += economie.AppliquerInflation(mois, bien.assurance_habitation);
    montant += economie.AppliquerInflation(mois, bien.travaux_amortis);
    if (mois == 0) {
      montant += bien.prix + bien.frais_notaire - emprunt.capital;
    }
    if (mois < emprunt.duree) {
      if (mois == 0) {
        montant += emprunt.frais_dossier + emprunt.frais_garantie;
      }
      montant += emprunt.assurance;
      montant += emprunt.mensualite();
    }
    return montant;
  }
  double benefices(int mois) const {
    return recettes(mois) - depenses(mois) - prelevements_sociaux(mois) -
           impot_sur_le_revenu(mois);
  }

  void TableauAmortissement() {
    std::cout << std::setprecision(2) << std::fixed;
    std::cout << "Inflation:   " << economie.inflation << "% par an"
              << std::endl;
    std::cout << "Prix m2:                " << std::setw(11)
              << (bien.prix / bien.surface) << std::endl;
    std::cout << "Mensualite + assurance: " << std::setw(11)
              << (emprunt.mensualite() + emprunt.assurance) << " = "
              << emprunt.mensualite() << " + " << emprunt.assurance
              << std::endl;
    std::cout << "Cout emprunt:           " << std::setw(11) << emprunt.cout()
              << std::endl;
    std::cout << "Total projet:           " << std::setw(11)
              << (bien.prix + bien.frais_notaire) << std::endl;
    std::cout << "Apport personnel:       " << std::setw(11)
              << (bien.prix + bien.frais_notaire - emprunt.capital +
                  emprunt.frais_dossier + emprunt.frais_garantie)
              << std::endl;
    std::cout << std::endl;

    std::cout << "mois,   capital,  recettes,  depenses,  rev nets, % deduc,"
                 "       isr,    impots,     benefs,    cumules,   solde TC"
              << std::endl;
    double benefices_cumules = 0.;
    bool revente_zero_profit = false, benefices_cumules_positifs = false;
    bool mois_ignore = false;
    for (int mois = 0; mois < emprunt.duree || benefices_cumules < 0; ++mois) {
      benefices_cumules += benefices(mois);
      bool premiere_revente_zero_profit = false;
      // TODO(yguyon): Prendre en compte indemnites remboursement anticipe
      const double solde_revente =
          economie.AppliquerInflation(mois, bien.prix) + benefices_cumules -
          emprunt.capital_restant_du(mois);
      if (!revente_zero_profit && solde_revente >= 0) {
        revente_zero_profit = true;
        premiere_revente_zero_profit = true;
      }
      bool premiere_fois_benefices_cumules_positifs = false;
      if (!benefices_cumules_positifs && benefices_cumules >= 0) {
        benefices_cumules_positifs = true;
        premiere_fois_benefices_cumules_positifs = true;
      }

      if (mois < 15 || (mois > emprunt.duree - 3 && mois < emprunt.duree + 1) ||
          premiere_revente_zero_profit ||
          premiere_fois_benefices_cumules_positifs) {
        const double ratio_net_brut =
            revenu_net_avec_frais_amortis(mois) / revenu_brut(mois);
        std::cout << std::setw(4) << (mois + 1) << ",";
        std::cout << std::setw(10) << emprunt.capital_restant_du(mois) << ",";
        std::cout << std::setw(10) << recettes(mois) << ",";
        std::cout << std::setw(10) << depenses(mois) << ",";
        std::cout << std::setw(10) << revenu_net_avec_frais_amortis(mois)
                  << ",";
        std::cout << std::setw(7) << (100.0 - 100.0 * ratio_net_brut) << "%,";
        std::cout << std::setw(10) << impot_sur_le_revenu(mois) << ",";
        std::cout << std::setw(10)
                  << (prelevements_sociaux(mois) + impot_sur_le_revenu(mois))
                  << ",";
        std::cout << std::setw(11) << benefices(mois) << ",";
        std::cout << std::setw(11) << benefices_cumules << ",";
        std::cout << std::setw(11) << solde_revente;
        if ((mois + 1) % 12 == 0 || premiere_revente_zero_profit ||
            premiere_fois_benefices_cumules_positifs) {
          std::cout << "   " << ((mois + 1) / 12) << " ans";
          if ((mois + 1) % 12 != 0) {
            std::cout << " et " << ((mois + 1) % 12) << " mois";
          }
        }
        if (mois == emprunt.duree - 1) {
          std::cout << ", fin emprunt";
        }
        if (premiere_revente_zero_profit) {
          std::cout << ", 0 profit avec revente au prix de "
                    << economie.AppliquerInflation(mois, bien.prix);
        }
        if (premiere_fois_benefices_cumules_positifs) {
          std::cout << ", 0 profit sans revente";
        }
        std::cout << std::endl;
        mois_ignore = false;
      } else if (!mois_ignore) {
        std::cout << " ..." << std::endl;
        mois_ignore = true;
      }

      if (mois > 12 * 100) {
        std::cout << "Trop long" << std::endl;
        break;
      }
    }
  }

  const Economie economie;
  const Bien bien;
  const Emprunt emprunt;
  const Profil profil;
};

}  // namespace immou

#endif  // IMMOU_MODELE_HPP
