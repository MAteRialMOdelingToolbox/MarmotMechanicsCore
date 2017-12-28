#include "bftEnhancedAssumedStrain.h"


namespace bft{
    namespace EAS{

        MatrixXd F ( const Ref< const MatrixXd >& J )
        {
            // Transformation according to 
            // - Andelfinger, Ramm (1993),
            // - 'Notes on Continuum Mechanics -  Eduardo WV Chaves' !
            // - Lecture Notes S.Kinkel
            // - Quy,Matzenmiller 2007
            //
            // Attention: Incosistent with Simo Rifai ( topleft block!)
            // and FEAP Theory Manual!
            // nDim == 2
            if ( J.cols() == 2 ){

                Matrix3d  F;
                
                F <<    J(0,0)*J(0,0),    J(0,1)*J(0,1),        2*J(0,0)*J(0,1),
                        J(1,0)*J(1,0),    J(1,1)*J(1,1),        2*J(1,0)*J(1,1),
                        J(0,0)*J(1,0),    J(0,1)*J(1,1),        J(0,0)*J(1,1)+J(0,1)*J(1,0);
                        
                 
                return F;
            }
            else if ( J.cols() == 3 ){

                Matrix6  F;
                F.topLeftCorner(3,3) << 
                    J(0,0)*J(0,0),  J(0,1)*J(0,1),  J(0,2)*J(0,2),
                    J(1,0)*J(1,0),  J(1,1)*J(1,1),  J(1,2)*J(1,2),
                    J(2,0)*J(2,0),  J(2,1)*J(2,1),  J(2,2)*J(2,2);

                F.topRightCorner(3,3) <<
                    2*J(0,0)*J(0,1),  2*J(0,0)*J(0,2),  2*J(0,1)*J(0,2),
                    2*J(1,0)*J(1,1),  2*J(1,0)*J(1,2),  2*J(1,1)*J(1,2),
                    2*J(2,0)*J(2,1),  2*J(2,0)*J(2,2),  2*J(2,1)*J(2,2);

                F.bottomLeftCorner(3,3) <<
                    J(0,0)*J(1,0),  J(0,1)*J(1,1),  J(0,2)*J(1,2),
                    J(0,0)*J(2,0),  J(0,1)*J(2,1),  J(0,2)*J(2,2),
                    J(1,0)*J(2,0),  J(1,1)*J(2,1),  J(1,2)*J(2,2);
                
                F.bottomRightCorner(3,3) <<
                    J(0,0)*J(1,1)+J(0,1)*J(1,0),  J(0,0)*J(1,2)+J(0,2)*J(1,0),  J(0,1)*J(1,2)+J(0,2)*J(1,1),
                    J(0,0)*J(2,1)+J(0,1)*J(2,0),  J(0,0)*J(2,2)+J(0,2)*J(2,0),  J(0,1)*J(2,2)+J(0,2)*J(2,1),
                    J(1,0)*J(2,1)+J(1,1)*J(2,0),  J(1,0)*J(2,2)+J(1,2)*J(2,0),  J(1,1)*J(2,2)+J(1,2)*J(2,1);

                 
                return F;
            }
            throw std::invalid_argument ("Invalid Dimension for bft::EnhancedAssumedStrain!" );
        }

        MatrixXd EASInterpolation ( EASType type, const Ref< const VectorXd >& xi )
        {
            // Implementation for 2D
            
            switch(type){
                case DeBorstEAS2: { 

                       Matrix<double, 3, 2> E_; 

                        E_ <<   xi[1],      0,
                                0,          xi[0],
                                0,          0;

                        return E_;
                    }
                case EAS3: {
                                       Matrix< double, 6, 3> E_ = Matrix<double, 6, 3>::Zero();

                                       E_.topLeftCorner(3,3).diagonal() <<  xi[0], xi[1], xi[2];
                                       
                                       return E_;

                                   }

                case DeBorstEAS9 : {
                                       Matrix< double, 6, 9> E_ = Matrix<double, 6, 9>::Zero();


                                       E_.topLeftCorner(3,3).diagonal() <<  xi[0], xi[1], xi[2];
                                       
                                       E_(0,3) = xi[0] * xi[1];
                                       E_(0,4) = xi[0] * xi[2];

                                       E_(1,5) = xi[1] * xi[0];
                                       E_(1,6) = xi[1] * xi[2];

                                       E_(2,7) = xi[2] * xi[0];
                                       E_(2,8) = xi[2] * xi[1];

                                       return E_;

                                   }

                case SimoRifaiEAS5: {

                       Matrix<double, 3, 5> E_; 

                        E_ <<   xi[0],      0,      0,      0,      xi[0]*xi[1],
                                0,          xi[1],  0,      0,      -xi[0]*xi[1],
                                0,          0,      xi[0],  xi[1],  xi[0]*xi[0]-xi[1]*xi[1]; 

                        return E_;
                    }
                case SimoRifaiEAS4: {

                       Matrix<double, 3, 4> E_; 

                        E_ <<   xi[0],      0,      0,      0,      
                                0,          xi[1],  0,      0,     
                                0,          0,      xi[0],  xi[1];
                        return E_;
                    }

                default:    throw std::invalid_argument("Invalid EAS Type Requested"); }

        }
    }
}