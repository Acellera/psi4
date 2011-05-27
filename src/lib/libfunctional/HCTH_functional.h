#ifndef HCTH_functional_h
#define HCTH_functional_h
/**********************************************************
* HCTH_functional.h: declarations for HCTH_functional for KS-DFT
* Robert Parrish, robparrish@gmail.com
* Autogenerated by MATLAB Script on 25-May-2011
*
***********************************************************/
#include "functional.h"

namespace psi { namespace functional {

class HCTH_Functional : public Functional {
public:
    HCTH_Functional(int npoints, int deriv);
    virtual ~HCTH_Functional();
    virtual void computeRKSFunctional(boost::shared_ptr<Properties> prop);
    virtual void computeUKSFunctional(boost::shared_ptr<Properties> prop);
};
}}
#endif

