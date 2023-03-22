#include "Marmot/MarmotKinematics.h"
#include "Marmot/MarmotTensor.h"

using namespace Eigen;

namespace Marmot {
  namespace ContinuumMechanics::Kinematics {

    namespace VelocityGradient {

      EigenTensors::Tensor3333d initializeDOmega_dVelocityGradient()
      {
        EigenTensors::Tensor3333d dwdl;

        for ( int i = 0; i < 3; i++ )
          for ( int j = 0; j < 3; j++ )
            for ( int k = 0; k < 3; k++ )
              for ( int l = 0; l < 3; l++ ) {
                dwdl( i, j, k, l ) = 0.5 * ( ( i == k ? 1 : 0 ) * ( j == l ? 1 : 0 ) -
                                             ( k == j ? 1 : 0 ) * ( i == l ? 1 : 0 ) );
              }
        return dwdl;
      }
      const EigenTensors::Tensor3333d dOmega_dVelocityGradient = initializeDOmega_dVelocityGradient();

      EigenTensors::Tensor633d initializeDStretchingRate_dVelocityGradient()
      {
        EigenTensors::Tensor633d dddl;

        for ( int i = 0; i < 3; i++ )
          for ( int j = 0; j < 3; j++ )
            for ( int k = 0; k < 3; k++ )
              for ( int l = 0; l < 3; l++ ) {
                dddl( ContinuumMechanics::TensorUtility::IndexNotation::toVoigt< 3 >( i, j ),
                      k,
                      l ) = 0.5 *
                            ( ( i == k ? 1 : 0 ) * ( j == l ? 1 : 0 ) + ( j == k ? 1 : 0 ) * ( i == l ? 1 : 0 ) ) *
                            ( i == j ? 1 : 2 ); // strain-engineering-notation correction
              }
        return dddl;
      }

      const EigenTensors::Tensor633d dStretchingRate_dVelocityGradient = initializeDStretchingRate_dVelocityGradient();

    } // namespace VelocityGradient

    namespace Strain {

      Marmot::Vector6d GreenLagrange( const Eigen::Matrix3d& F )
      {
        Eigen::Matrix3d H = F - Eigen::Matrix3d::Identity();
        return Marmot::ContinuumMechanics::VoigtNotation::voigtFromStrainMatrix< 3 >(
          0.5 * ( H + H.transpose() + H.transpose() * H ) );
      }

      Marmot::EigenTensors::Tensor633d dGreenLagrangedDeformationGradient( const Eigen::Matrix3d& F )
      {
        EigenTensors::Tensor633d dEdF;
        auto                     kron = Matrix3d::Identity();

        for ( int IJ = 0; IJ < 6; IJ++ ) {
          auto [I, J] = Marmot::ContinuumMechanics::TensorUtility::IndexNotation::fromVoigt< 3 >( IJ );
          for ( int k = 0; k < 3; k++ )
            for ( int L = 0; L < 3; L++ )

              dEdF( IJ, k, L ) = 0.5 * ( kron( I, L ) * F( k, J ) + kron( J, L ) * F( k, I ) ) * ( I == J ? 1 : 2 );
        }

        return dEdF;
      }

    } // namespace Strain
    namespace DeformationGradient {
      template <>
      Eigen::Matrix3d make3D( const Eigen::Ref< const Eigen::Matrix< double, 1, 1 > >& tensor )
      {
        Matrix3d tensor3D = Matrix3d::Identity();
        tensor3D( 0, 0 )  = tensor( 0, 0 );
        return tensor3D;
      }

      template <>
      Eigen::Matrix3d make3D( const Eigen::Ref< const Eigen::Matrix< double, 2, 2 > >& tensor )
      {
        Matrix3d tensor3D              = Matrix3d::Identity();
        tensor3D.topLeftCorner( 2, 2 ) = tensor;
        return tensor3D;
      }

      template <>
      Eigen::Matrix3d make3D( const Eigen::Ref< const Eigen::Matrix< double, 3, 3 > >& tensor )
      {
        return tensor;
      }
    } // namespace DeformationGradient

  } // namespace ContinuumMechanics::Kinematics
} // namespace Marmot
