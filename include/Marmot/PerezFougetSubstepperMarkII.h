/* ---------------------------------------------------------------------
 *                                       _
 *  _ __ ___   __ _ _ __ _ __ ___   ___ | |_
 * | '_ ` _ \ / _` | '__| '_ ` _ \ / _ \| __|
 * | | | | | | (_| | |  | | | | | | (_) | |_
 * |_| |_| |_|\__,_|_|  |_| |_| |_|\___/ \__|
 *
 * Unit of Strength of Materials and Structural Analysis
 * University of Innsbruck,
 * 2020 - today
 *
 * festigkeitslehre@uibk.ac.at
 *
 * Matthias Neuner matthias.neuner@uibk.ac.at
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
#include "Marmot/MarmotJournal.h"
#include "Marmot/MarmotMath.h"
#include "Marmot/MarmotTypedefs.h"

namespace Marmot::NumericalAlgorithms {

  /** Substepper for (linear elastic) elastoplastic materials, implicit return mapping version
   *
   * */
  template < int nSizeMatTangent >
  class PerezFougetSubstepper {

  public:
    typedef Eigen::Matrix< double, nSizeMatTangent, nSizeMatTangent > TangentSizedMatrix;
    PerezFougetSubstepper( double          initialStepSize,
                           double          minimumStepSize,
                           double          scaleUpFactor,
                           double          scaleDownFactor,
                           int             nPassesToIncrease,
                           const Matrix6d& Cel );

    /// check if the subincrement process has finished
    bool isFinished();

    /// get the next subincrement size
    double getNextSubstep();

    /// decreas the subincrement size
    bool decreaseSubstepSize();

    /// finish an elastic only subincrement
    void finishElasticSubstep();

    /// finish a subincrement, pass the inverse material tangent
    void finishSubstep( const TangentSizedMatrix& dXdY );

    /// get the overall consistent algorithmic tangent
    Matrix6d consistentStiffness();

  private:
    const double initialStepSize, minimumStepSize, scaleUpFactor, scaleDownFactor;
    const int    nPassesToIncrease;

    double currentProgress;
    double currentSubstepSize;
    int    passedSubsteps;

    const Matrix6d& Cel;

    TangentSizedMatrix consistentTangent;
  };
} // namespace Marmot::NumericalAlgorithms

namespace Marmot::NumericalAlgorithms {
  template < int n >
  PerezFougetSubstepper< n >::PerezFougetSubstepper( double          initialStepSize,
                                                     double          minimumStepSize,
                                                     double          scaleUpFactor,
                                                     double          scaleDownFactor,
                                                     int             nPassesToIncrease,
                                                     const Matrix6d& Cel )
    :

      initialStepSize( initialStepSize ),
      minimumStepSize( minimumStepSize ),
      scaleUpFactor( scaleUpFactor ),
      scaleDownFactor( scaleDownFactor ),
      nPassesToIncrease( nPassesToIncrease ),
      currentProgress( 0.0 ),
      currentSubstepSize( initialStepSize ),
      passedSubsteps( 0 ),
      Cel( Cel )
  {
    consistentTangent = TangentSizedMatrix::Zero();
  }

  template < int n >
  bool PerezFougetSubstepper< n >::isFinished()
  {
    // this is due to numerical accuracy ...
    return ( 1.0 - currentProgress ) <= 2e-16;
  }

  template < int n >
  double PerezFougetSubstepper< n >::getNextSubstep()
  {
    if ( passedSubsteps >= nPassesToIncrease )
      currentSubstepSize *= scaleUpFactor;

    const double remainingProgress = 1.0 - currentProgress;
    if ( remainingProgress < currentSubstepSize )
      currentSubstepSize = remainingProgress;

    passedSubsteps++;
    currentProgress += currentSubstepSize;

    return currentSubstepSize;
  }

  template < int n >
  bool PerezFougetSubstepper< n >::decreaseSubstepSize()
  {
    currentProgress -= currentSubstepSize;
    passedSubsteps = 0;

    currentSubstepSize *= scaleDownFactor;

    if ( currentSubstepSize < minimumStepSize )
      return MarmotJournal::warningToMSG( "UMAT: Substepper: Minimal stepzsize reached" );
    else
      return MarmotJournal::notificationToMSG( "UMAT: Substepper: Decreasing stepsize" );
  }

  template < int n >
  void PerezFougetSubstepper< n >::finishElasticSubstep()
  {
    consistentTangent += currentSubstepSize * TangentSizedMatrix::Identity();
  }

  template < int n >
  void PerezFougetSubstepper< n >::finishSubstep( const TangentSizedMatrix& dXdY )
  {
    finishElasticSubstep();
    consistentTangent.applyOnTheLeft( dXdY );

    for ( int i = 0; i < consistentTangent.rows(); i++ )
      for ( int j = 0; j < consistentTangent.cols(); j++ )
        if ( std::abs( consistentTangent( i, j ) ) < 1e-12 )
          consistentTangent( i, j ) = 0.0;
  }

  template < int n >
  Matrix6d PerezFougetSubstepper< n >::consistentStiffness()
  {
    return consistentTangent.topLeftCorner( 6, 6 ) * Cel;
  }
} // namespace Marmot::NumericalAlgorithms
