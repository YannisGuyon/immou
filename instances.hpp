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

#ifndef IMMOU_INSTANCES_HPP
#define IMMOU_INSTANCES_HPP

#include "./modele.hpp"

namespace immou {

struct Instances {
  static void LeMillion() {
    const Economie economie{.inflation = 1,
                            .loyer_augmente_avec_inflation = false};
    const Bien bien{.economie = economie,
                    .surface = 50.0,
                    .prix = 500000.00,
                    .frais_notaire = 40000.00,
                    .loyer_hors_charges = 2000.00,
                    .charges_recuperables = 400.00,
                    .charges_copro = 4000.00 / 12,
                    .taxe_fonciere = 2000.00 / 12,
                    .gestion_locative = 7.0,
                    .mise_en_location = 15.0,
                    .mois_entre_locataires = 3 * 12,
                    .travaux_amortis = 0.00,
                    .assurance_habitation = 40.00};
    const Emprunt emprunt{.capital = 400000.00,
                          .frais_dossier = 2000.00,
                          .frais_garantie = 5000.00,
                          .taux = 3.0,
                          .duree = 20 * 12,
                          .assurance = 6.0};
    const Profil profil{.revenu_fiscal_de_reference = 100000};

    Simulation simulation{economie, bien, emprunt, profil};
    simulation.TableauAmortissement();
  }
};

}  // namespace immou

#endif  // IMMOU_INSTANCES_HPP
