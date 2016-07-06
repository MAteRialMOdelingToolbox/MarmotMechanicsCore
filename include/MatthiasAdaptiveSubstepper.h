#pragma once 
#include "bftTypedefs.h"
#include "bftFunctions.h"

namespace bft{

    template <size_t materialTangentSize, size_t nIntegrationDependentStateVars>
        class NeunerAdaptiveSubstepper
        {
            public:
                typedef Matrix<double, materialTangentSize, materialTangentSize> TangentSizedMatrix;
                typedef Matrix<double, nIntegrationDependentStateVars, 1> IntegrationStateVector;
                enum ResultAccuracyModes{SingleStep, DoubleStep, RichardsonExtrapolation};

                NeunerAdaptiveSubstepper(double initialStepSize, double minimumStepSize, double maxScaleUpFactor, double scaleDownFactor, 
                                        double integrationErrorTolerance,
                                        int nPassesToIncrease, const Matrix6& Cel);
                void setConvergedProgress(const Vector6& stressOld, const IntegrationStateVector& stateVarsOld);
                bool isFinished();
                double getNextSubstep();
                bool finishSubstep(const Vector6& resultStress, const TangentSizedMatrix& dXdY, const IntegrationStateVector& stateVars); 
                void finishElasticSubstep(const Vector6& resultStress); 
                bool discardSubstep();
                bool repeatSubstep(double decrementationFactor);
                
                void getConvergedProgress(Vector6& stress, IntegrationStateVector& stateVars);
                Matrix6 getCurrentTangentOperator();
                void getResults(Vector6& stress, Matrix6& consistentTangent, IntegrationStateVector& stateVars);

            private:


                const double    initialStepSize,
                                minimumStepSize,
                                maxScaleUpFactor,
                                scaleDownFactor,
                                integrationErrorTolerance;
                const int       nPassesToIncrease;
                const bool      ignoreErrorToleranceOnMinimumStepSize;

                double currentProgress;
                double currentSubstepSize;
                int passedSubsteps;
                int substepIndex;

                //const ResultAccuracyModes resultAccuracy;
                
                // internal storages for the progress of the total increment
                Vector6                 stressProgress;
                IntegrationStateVector  stateProgress;
                TangentSizedMatrix      consistentTangentProgress;

                // temporal storages, which are used until a cycle full/half/half has finished successfully
                Vector6                 stressProgressHalfTemp, stressProgressFullTemp;
                IntegrationStateVector  stateProgressHalfTemp,  stateProgressFullTemp;
                TangentSizedMatrix      consistentTangentProgressHalfTemp, consistentTangentProgressFullTemp;

                TangentSizedMatrix elasticTangent;

                enum SubsteppingState{FullStep, FirstHalfStep, SecondHalfStep};
                SubsteppingState currentState;

                bool acceptSubstepWithFullStepOnly();
                bool splitCurrentSubstep();

        };
}

namespace bft{
    template <size_t n, size_t nState>
    NeunerAdaptiveSubstepper<n, nState>::NeunerAdaptiveSubstepper(double initialStepSize, 
                                                    double minimumStepSize, 
                                                    double maxScaleUpFactor, 
                                                    double scaleDownFactor, 
                                                    double integrationErrorTolerance,
                                                    int nPassesToIncrease,
                                                    const Matrix6& Cel):
                                                    //ResultAccuracyModes resultMode,
                                                    initialStepSize(initialStepSize),
                                                    minimumStepSize(minimumStepSize),
                                                    maxScaleUpFactor(maxScaleUpFactor), scaleDownFactor(scaleDownFactor),
                                                    integrationErrorTolerance(integrationErrorTolerance),
                                                    nPassesToIncrease(nPassesToIncrease),
                                                    ignoreErrorToleranceOnMinimumStepSize(true),
                                                    currentProgress(0.0),
                                                    currentSubstepSize(initialStepSize),
                                                    passedSubsteps(0),
                                                    substepIndex(-1)
                                                    //resultAccuracy(resultAccuracy)
    {
            elasticTangent = TangentSizedMatrix::Identity();
            elasticTangent.topLeftCorner(6,6) = Cel;

            consistentTangentProgress = TangentSizedMatrix::Zero();
            consistentTangentProgressFullTemp = TangentSizedMatrix::Zero();
            consistentTangentProgressHalfTemp = TangentSizedMatrix::Zero();

            stressProgressHalfTemp.setZero();
            stressProgressFullTemp.setZero();
            stateProgressHalfTemp.setZero();  stateProgressFullTemp.setZero();
            consistentTangentProgressHalfTemp.setZero(); consistentTangentProgressFullTemp.setZero();

            currentState = FullStep;
    }

    template<size_t n, size_t nState>
    void NeunerAdaptiveSubstepper<n, nState>::setConvergedProgress(const Vector6& stressOld, const IntegrationStateVector& stateVarsOld)
    {
            stressProgress = stressOld;
            stateProgress = stateVarsOld;
    }

    template<size_t n, size_t nState>
    bool NeunerAdaptiveSubstepper<n, nState>::isFinished()
    {
        return ( ( 1.0 - currentProgress) <=2e-16 && currentState == FullStep );
    }

    template<size_t n, size_t nState>
    double NeunerAdaptiveSubstepper<n, nState>::getNextSubstep()
    {
        switch(currentState){
            case FullStep:{
                const double remainingProgress = 1.0 - currentProgress;
                if( remainingProgress < currentSubstepSize)
                        currentSubstepSize = remainingProgress;            
                substepIndex++;
                //std::ostringstream str;
                //str << substepIndex << "/" << currentProgress<<": " << currentSubstepSize;
                //notificationToMSG(str.str());
                return currentSubstepSize;
                break;}
            case FirstHalfStep:{
                return 0.5 * currentSubstepSize;
                break;}
            case SecondHalfStep:{
                return 0.5 * currentSubstepSize;
                break;}
        }
    }
    template<size_t n, size_t nState>
    void NeunerAdaptiveSubstepper<n, nState>::getConvergedProgress(Vector6& stress, IntegrationStateVector& stateVars)
    {
        switch(currentState)
        {
            case FullStep:{
                stress = stressProgress;
                stateVars = stateProgress;
                break;}
            case FirstHalfStep:{
                stress = stressProgress;
                stateVars = stateProgress;
                break;}
            case SecondHalfStep: {
                stress = stressProgressHalfTemp;
                stateVars = stateProgressHalfTemp;
                break;}
        }
    }

    template<size_t n, size_t nState>
    bool NeunerAdaptiveSubstepper<n, nState>::discardSubstep()
    {
        passedSubsteps = 0;
        switch(currentState)
        {
            case FullStep: {
                    currentSubstepSize *= scaleDownFactor; // we use the scale factor only here
                    break; }
            // these cases should actually never happen, as the full step has already converged!
            case FirstHalfStep:
                   warningToMSG("UMAT: warning, 1th half sub step has not converged after already converged full step");
                   return acceptSubstepWithFullStepOnly();                

            case SecondHalfStep:
                   warningToMSG("UMAT: warning, 2th half sub step has not converged after already converged full step");
                   return acceptSubstepWithFullStepOnly();
        }

        currentState = FullStep;

        if(currentSubstepSize < minimumStepSize)
                return warningToMSG("UMAT: Substepper: Minimal stepzsize reached");
        else
                return true;
    }

    template<size_t n, size_t nState>
    bool NeunerAdaptiveSubstepper<n, nState>::repeatSubstep(double factorNew)
    {
        currentState = FullStep;
        passedSubsteps = 0;

        currentSubstepSize *= factorNew; // we use the scale factor only here

        if(currentSubstepSize < minimumStepSize)
                return warningToMSG("UMAT: Substepper: Minimal stepzsize reached");
        else
                return true;

    }

    template<size_t n, size_t nState>
    bool NeunerAdaptiveSubstepper<n, nState>::finishSubstep(const Vector6& resultStress, 
            const TangentSizedMatrix& dXdY, 
            const IntegrationStateVector& stateVars)
    {
            if(currentState == FullStep){
                stressProgressFullTemp = resultStress;
                stateProgressFullTemp = stateVars;
                consistentTangentProgressFullTemp = consistentTangentProgress;
                consistentTangentProgressFullTemp += currentSubstepSize * elasticTangent; 
                consistentTangentProgressFullTemp.applyOnTheLeft(dXdY);
                currentState = FirstHalfStep;
                return true; }
            else if (currentState == FirstHalfStep){

                stressProgressHalfTemp = resultStress;
                stateProgressHalfTemp = stateVars;
                consistentTangentProgressHalfTemp = consistentTangentProgress;
                consistentTangentProgressHalfTemp += 0.5* currentSubstepSize * elasticTangent; 
                consistentTangentProgressHalfTemp.applyOnTheLeft(dXdY);
                currentState = SecondHalfStep;
                return true; }

            else if (currentState == SecondHalfStep){
                //error Estimation
                
                currentState = FullStep;
                const double error = (resultStress - stressProgressFullTemp).norm(); 
                const double errorRatio = error / integrationErrorTolerance ;
                double scaleFactor = 1.0;
                if(errorRatio > 1e-10)
                    scaleFactor = 0.9 * std::sqrt( 1./errorRatio );

                //saturations
                if(scaleFactor < 0.1)
                    scaleFactor = 0.1;
                if(scaleFactor * currentSubstepSize < minimumStepSize)
                    scaleFactor = minimumStepSize / currentSubstepSize;
                if(scaleFactor > 10)
                    scaleFactor = 10;

                //Error large than tolerance?
                if( error > integrationErrorTolerance){
                    passedSubsteps = 0;
                    //std::ostringstream str;
                    //str << "UMAT: integration error in sub "<<substepIndex<<": " << error <<", ratio: "<<errorRatio;
                    if ( errorRatio < 2) {
                        //str << "--> splitting substep"/*;*/
                        /*warningToMSG(str.str());*/
                        return splitCurrentSubstep(); }
                    else{
                        //str << "--> discarding substep";
                        //warningToMSG(str.str());
                        return repeatSubstep(scaleFactor); }
                }
                else{
                    stressProgressHalfTemp = resultStress;
                    stateProgressHalfTemp = stateVars;
                    consistentTangentProgressHalfTemp += 0.5 * currentSubstepSize *  elasticTangent;
                    consistentTangentProgressHalfTemp.applyOnTheLeft(dXdY);

                    consistentTangentProgress = 2*consistentTangentProgressHalfTemp - consistentTangentProgressFullTemp;
                    stressProgress = 2*stressProgressHalfTemp - stressProgressFullTemp;
                    stateProgress = 2*stateProgressHalfTemp - stateProgressFullTemp;

                    currentProgress += currentSubstepSize;

                    passedSubsteps++;
                    currentSubstepSize *=   scaleFactor;

                    return true;}
             }
    }

    template<size_t n, size_t nState>
    void NeunerAdaptiveSubstepper<n, nState>::finishElasticSubstep(const Vector6& newStress)
    {  
            switch(currentState)
            {
                case FullStep:{
                    // this means, that the complete current cycle is already successfull, 
                    // as the two half steps must also be elastic!
                    consistentTangentProgress += currentSubstepSize * elasticTangent;
                    stressProgress = newStress;
                    // no need for two half steps if full step was already elastic
                    currentProgress += currentSubstepSize; 
                    currentState = FullStep;
                    passedSubsteps++;
                    break;}

                case FirstHalfStep:{
                    consistentTangentProgressHalfTemp += 0.5 * currentSubstepSize * elasticTangent;
                    stressProgressHalfTemp = newStress;
                    currentState = SecondHalfStep;
                    break;}
                case SecondHalfStep:{
                    acceptSubstepWithFullStepOnly();
                    break;}
            }
    }

    template<size_t n, size_t nState>
    void NeunerAdaptiveSubstepper<n, nState>::getResults(Vector6& stress, Matrix6& consistentTangentOperator, IntegrationStateVector& stateVars)
    {
        stress = stressProgress;
        stateVars = stateProgress;
        consistentTangentOperator = consistentTangentProgress.topLeftCorner(6,6); 
    }

    template<size_t n, size_t nState>
    bool NeunerAdaptiveSubstepper<n, nState>::acceptSubstepWithFullStepOnly()
    {
        consistentTangentProgress = consistentTangentProgressFullTemp;
        stressProgress = stressProgressFullTemp;
        stateProgress = stateProgressFullTemp;

        currentProgress += currentSubstepSize;
        currentState = FullStep;
        
        return true;
    }

    template<size_t n, size_t nState>
    bool NeunerAdaptiveSubstepper<n, nState>::splitCurrentSubstep()
    {
        if(currentSubstepSize < 2 * minimumStepSize)
            if( ignoreErrorToleranceOnMinimumStepSize)
                return acceptSubstepWithFullStepOnly();
            else 
                return false;

       consistentTangentProgressFullTemp = consistentTangentProgressHalfTemp;
       stressProgressFullTemp = stressProgressHalfTemp;
       stateProgressFullTemp = stateProgressHalfTemp;
       currentSubstepSize *= 0.5;
       currentState = FirstHalfStep;

       return true;
    }
}