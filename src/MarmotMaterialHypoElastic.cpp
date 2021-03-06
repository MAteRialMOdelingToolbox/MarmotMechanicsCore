#include "Marmot/MarmotMaterialHypoElastic.h"
#include "Marmot/HughesWinget.h"
#include "Marmot/MarmotJournal.h"
#include "Marmot/MarmotKinematics.h"
#include "Marmot/MarmotMath.h"
#include "Marmot/MarmotTensor.h"
#include "Marmot/MarmotVoigt.h"
#include <iostream>

using namespace Eigen;

void MarmotMaterialHypoElastic::setCharacteristicElementLength( double length )
{
  characteristicElementLength = length;
}

void MarmotMaterialHypoElastic::computeStress( double*       stress_,
                                               double*       dStressDDDeformationGradient_,
                                               const double* FOld_,
                                               const double* FNew_,
                                               const double* timeOld,
                                               const double  dT,
                                               double&       pNewDT )
{
  // Standard implemenation of the Abaqus like Hughes-Winget algorithm
  // Approximation of the algorithmic tangent in order to
  // facilitate the dCauchy_dStrain tangent provided by
  // small strain material models

  using namespace Marmot;
  using namespace Marmot::ContinuumMechanics::TensorUtility;
  using namespace ContinuumMechanics::Kinematics::velocityGradient;

  const Map< const Matrix3d > FOld( FOld_ );
  const Map< const Matrix3d > FNew( FNew_ );
  Marmot::mVector6d           stress( stress_ );

  Marmot::NumericalAlgorithms::HughesWinget
    hughesWingetIntegrator( FOld, FNew, Marmot::NumericalAlgorithms::HughesWinget::Formulation::AbaqusLike );

  auto dEps = hughesWingetIntegrator.getStrainIncrement();
  stress    = hughesWingetIntegrator.rotateTensor( stress );

  Matrix6d CJaumann;

  computeStress( stress.data(), CJaumann.data(), dEps.data(), timeOld, dT, pNewDT );

  TensorMap< Eigen::Tensor< double, 3 > > dS_dF( dStressDDDeformationGradient_, 6, 3, 3 );

  dS_dF = hughesWingetIntegrator.compute_dS_dF( stress, FNew.inverse(), CJaumann );
}

void MarmotMaterialHypoElastic::computePlaneStress( double*       stress_,
                                                    double*       dStressDDStrain_,
                                                    double*       dStrain_,
                                                    const double* timeOld,
                                                    const double  dT,
                                                    double&       pNewDT )
{
  using namespace Marmot;

  Map< Vector6d > stress( stress_ );
  Map< Matrix6d > dStressDDStrain( dStressDDStrain_ );
  Map< Vector6d > dStrain( dStrain_ );
  Map< VectorXd > stateVars( this->stateVars, this->nStateVars );

  Vector6d stressTemp;
  VectorXd stateVarsOld = stateVars;
  Vector6d dStrainTemp  = dStrain;

  // assumption of isochoric deformation for initial guess
  dStrainTemp( 2 ) = ( -dStrain( 0 ) - dStrain( 1 ) );

  int planeStressCount = 1;
  while ( true ) {
    stressTemp = stress;
    stateVars  = stateVarsOld;

    computeStress( stressTemp.data(), dStressDDStrain.data(), dStrainTemp.data(), timeOld, dT, pNewDT );

    if ( pNewDT < 1.0 ) {
      return;
    }

    double residual = stressTemp.array().abs()[2];

    if ( residual < 1.e-10 || ( planeStressCount > 7 && residual < 1e-8 ) ) {
      break;
    }

    double tangentCompliance = 1. / dStressDDStrain( 2, 2 );
    if ( Math::isNaN( tangentCompliance ) || std::abs( tangentCompliance ) > 1e10 )
      tangentCompliance = 1e10;

    dStrainTemp[2] -= tangentCompliance * stressTemp[2];

    planeStressCount += 1;
    if ( planeStressCount > 13 ) {
      pNewDT = 0.25;
      MarmotJournal::warningToMSG( "PlaneStressWrapper requires cutback" );
      return;
    }
  }

  dStrain = dStrainTemp;
  stress  = stressTemp;
}

void MarmotMaterialHypoElastic::computeUniaxialStress( double* stress_,
                                                       double* dStressDDStrain_,

                                                       double*       dStrain_,
                                                       const double* timeOld,
                                                       const double  dT,
                                                       double&       pNewDT )
{
  using namespace Marmot;

  Map< Vector6d > stress( stress_ );
  Map< Matrix6d > dStressDDStrain( dStressDDStrain_ );
  Map< Vector6d > dStrain( dStrain_ );
  Map< VectorXd > stateVars( this->stateVars, this->nStateVars );

  Vector6d stressTemp;
  VectorXd stateVarsOld = stateVars;
  Vector6d dStrainTemp  = dStrain;

  dStrainTemp( 2 ) = 0.0;
  dStrainTemp( 1 ) = 0.0;

  int count = 1;
  while ( true ) {
    stressTemp = stress;
    stateVars  = stateVarsOld;

    computeStress( stressTemp.data(), dStressDDStrain.data(), dStrainTemp.data(), timeOld, dT, pNewDT );

    if ( pNewDT < 1.0 ) {
      return;
    }

    const double residual = stressTemp.array().abs().segment( 1, 2 ).sum();

    if ( residual < 1.e-10 || ( count > 7 && residual < 1e-8 ) ) {
      break;
    }

    dStrainTemp.segment< 2 >( 1 ) -= dStressDDStrain.block< 2, 2 >( 1, 1 ).colPivHouseholderQr().solve(
      stressTemp.segment< 2 >( 1 ) );

    count += 1;
    if ( count > 13 ) {
      pNewDT = 0.25;
      MarmotJournal::warningToMSG( "UniaxialStressWrapper requires cutback" );
      return;
    }
  }

  dStrain = dStrainTemp;
  stress  = stressTemp;
}
