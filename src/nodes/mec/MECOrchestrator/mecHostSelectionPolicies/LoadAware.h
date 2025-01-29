//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef NODES_MEC_MECORCHESTRATOR_MECHOSTSELECTIONPOLICIES_LOADAWARE_H_
#define NODES_MEC_MECORCHESTRATOR_MECHOSTSELECTIONPOLICIES_LOADAWARE_H_

#include "nodes/mec/MECOrchestrator/mecHostSelectionPolicies/SelectionPolicyBase.h"

//class MecOrchestrator;

class LoadAware : public SelectionPolicyBase
{
    protected:
        virtual cModule* findBestMecHost(const ApplicationDescriptor&) override;
    public:
        LoadAware(MecOrchestrator* mecOrchestrator):SelectionPolicyBase(mecOrchestrator){}
      virtual ~LoadAware(){}
};


#endif
