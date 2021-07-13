/* ---------------------------------------------------------------------
 *                                       _
 *  _ __ ___   __ _ _ __ _ __ ___   ___ | |_
 * | '_ ` _ \ / _` | '__| '_ ` _ \ / _ \| __|
 * | | | | | | (_| | |  | | | | | | (_) | |_
 * |_| |_| |_|\__,_|_|  |_| |_| |_|\___/ \__|
 *
 * Unit of Strength of Materials and Structural Analysis
 * University of Innsbruck
 * 2020 - today
 *
 * festigkeitslehre@uibk.ac.at
 *
 * Alexander Dummer alexander.dummer@uibk.ac.at
 *
 * This file is part of the MAteRialMOdellingToolbox (marmot).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * The full text of the license can be found in the file LICENSE.md at
 * the top level directory of marmot.
 * ---------------------------------------------------------------------
 */

#pragma once
#include "Marmot/MarmotKelvinChain.h"

namespace Marmot::Materials {
  namespace SolidificationTheory {

    struct Parameters {
      double q1;
      double q2;
      double q3;
      double q4;
      double n       = 0.1;
      double m       = 0.5;
      double lambda0 = 1.;
    };

    struct KelvinChainProperties {
      double                  E0;
      KelvinChain::Properties elasticModuli;
      KelvinChain::Properties retardationTimes;
    };

    struct UniaxialComplianceComponents {
      double elastic      = 0.;
      double viscoelastic = 0.;
      double flow         = 0.;
    };

    struct Result {
      Marmot::Vector6d             creepStrainIncrement;
      UniaxialComplianceComponents complianceComponents;
    };

    Result computeCreepStrainIncrementAndComplianceComponents(
      double                                                 tStartDays,
      double                                                 dTimeDays,
      double                                                 amplificationFactor,
      const Marmot::Matrix6d&                                flowUnitCompliance,
      const Marmot::Vector6d&                                stressOld,
      const Parameters&                                      parameters,
      const KelvinChainProperties&                           kelvinChainProperties,
      const Eigen::Ref< const KelvinChain::StateVarMatrix >& kelvinStateVars );

    double solidifiedVolume( double timeInDays, Parameters params );

    double computeZerothElasticModul( double tauMin, double n, int order );

    template < typename T >
    T phi( T xi, Parameters params )
    {
      return log( 1. + pow( xi, params.n ) );
    }

  } // namespace SolidificationTheory
} // namespace Marmot::Materials
