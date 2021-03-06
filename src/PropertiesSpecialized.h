/*
Project Name : OpenMEEG

© INRIA and ENPC (contributors: Geoffray ADDE, Maureen CLERC, Alexandre 
GRAMFORT, Renaud KERIVEN, Jan KYBIC, Perrine LANDREAU, Théodore PAPADOPOULO,
Emmanuel OLIVI
Maureen.Clerc.AT.sophia.inria.fr, keriven.AT.certis.enpc.fr,
kybic.AT.fel.cvut.cz, papadop.AT.sophia.inria.fr)

The OpenMEEG software is a C++ package for solving the forward/inverse
problems of electroencephalography and magnetoencephalography.

This software is governed by the CeCILL-B license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL-B
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's authors,  the holders of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-B license and that you accept its terms.
*/

#ifndef OPENMEEG_PROPERTIES_SPECIALIZED_H
#define OPENMEEG_PROPERTIES_SPECIALIZED_H

#include "Properties.H"
#include "DataTag.H"

namespace OpenMEEG {

    template <typename REP=double>
    class OPENMEEG_EXPORT Conductivity {
    public:

        Conductivity(): conductivity(1.0) { }

        REP& sigma()       { return conductivity; }
        REP  sigma() const { return conductivity; }
    private:
        REP conductivity;    //  The conductivity of the layer (constant per layer).
    };

    template <typename REP>
    inline std::istream& operator>>(std::istream& is,Conductivity<REP>& m) { return is >> m.sigma(); }

    template <typename REP>
    inline std::ostream& operator<<(std::ostream& os,const Conductivity<REP>& m) { return os << m.sigma(); }
}

namespace Types {
    template<>
    struct DataTrait<Utils::Properties::Named<std::string,OpenMEEG::Conductivity<double> > >{
        static const char TAG[];
    };
    const char DataTrait<Utils::Properties::Named<std::string,OpenMEEG::Conductivity<double> > >::TAG[]= "Conductivities";
};

#endif  //! OPENMEEG_PROPERTIES_SPECIALIZED_H
