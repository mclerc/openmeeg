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

#include "vector.h"

using namespace OpenMEEG;

namespace OpenMEEG {
    namespace Preconditioner {

        template <typename M>
        class None {
        public:
            None (const M m) { }
            Vector operator()(const Vector& g) const {
                return g;
            }

            ~None () {};
        };

        template <typename M>
        class Jacobi {
        public:
            Jacobi (const M m): v(m.nlin()) { 
                for (unsigned i=0;i<m.nlin();i++) 
                    v(i)=1.0/m(i,i);
            }
            Vector operator()(const Vector& g) const {
                Vector result(g.nlin());
                for (unsigned i=0;i<v.nlin();i++) result(i)=v(i)*g(i);
                return result;
            }

            ~Jacobi () {};
        private:
            Vector v;
        };

        class SSOR {
            Matrix prodMdiagM(const Matrix& m,const Vector& v) {
                Matrix C(m.nlin(),m.ncol()); //TODO improve this multiplication
                for (unsigned i=0;i<m.nlin();++i){
                    C(i,i) = v(i);
                }
                return m*C;
            }
        public:
            SSOR (const SymMatrix m, double _omega): omega(_omega) {
                // we split M into E = lower triangular part+D/omega, and D = the diagonal
                Vector D(m.nlin());
                Vector Dinv(m.nlin());
                for (int i=0;i<m.nlin();i++) {
                    D(i)=m(i,i);
                    Dinv(i)=omega/D(i);
                }

                Matrix E(m.nlin());
                E.set(0.0);
                for (int i=0;i<m.nlin();i++) {
                    for (int j=0;j<=i;j++) {
                        E(i,j)=m(i,j);
                    }
                }
                for (int i=0;i<m.nlin();i++) {
                        E(i,i)+=D(i)/omega;
                }

                // SSor = ((Lower+D*1./omega)*Dinv)*((Lower+D*1./omega).transpose()*1./(2-omega));
                SSor = ((prodMdiagM(E,Dinv))*(E.transpose()*1./(2.-omega))).symmetrize().inverse();
            }
           
            Vector operator()(const Vector& g) const {
                Vector result=SSor*g;
                return result;
            }

            ~SSOR () {};
        private:
            double omega;
            SymMatrix SSor;
        };
    }
}