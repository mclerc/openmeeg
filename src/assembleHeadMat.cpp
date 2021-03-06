/* OpenMEEG

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

#if WIN32
#define _USE_MATH_DEFINES
#endif

#include <math.h>

#include <matrix.h>
#include <symmatrix.h>
#include <geometry.h>
#include <operators.h>
#include <assemble.h>

namespace OpenMEEG {

    template<class T>
    void deflat(T& M, const Interface& i, double coef) 
    {
        // deflate the Matrix
        for ( Interface::const_iterator omit = i.begin(); omit != i.end(); ++omit) {
            for ( Mesh::const_vertex_iterator vit1 = omit->mesh().vertex_begin(); vit1 != omit->mesh().vertex_end(); ++vit1) {
                #pragma omp parallel for
                for ( Mesh::const_vertex_iterator vit2 = vit1; vit2 < omit->mesh().vertex_end(); ++vit2) {
                    M((*vit1)->index(), (*vit2)->index()) += coef;
                }
            }
        }
    }

    void assemble_HM(const Geometry& geo, SymMatrix& mat, const unsigned gauss_order) 
    {
        mat = SymMatrix((geo.size()-geo.outermost_interface().nb_triangles()));
        mat.set(0.0);
        double K = 1.0 / (4.0 * M_PI);

        // We iterate over the meshes (or pair of domains) to fill the lower half of the HeadMat (since its symmetry)
        for ( Geometry::const_iterator mit1 = geo.begin(); mit1 != geo.end(); ++mit1) {

            for ( Geometry::const_iterator mit2 = geo.begin(); (mit2 != (mit1+1)); ++mit2) {

                // if mit1 and mit2 communicate, i.e they are used for the definition of a common domain
                const int orientation = geo.oriented(*mit1, *mit2); // equals  0, if they don't have any domains in common
                                                                       // equals  1, if they are both oriented toward the same domain
                                                                       // equals -1, if they are not

                if ( orientation != 0 ) {

                    double Scoeff =   orientation * geo.sigma_inv(*mit1, *mit2) * K;
                    double Dcoeff = - orientation * geo.indicator(*mit1, *mit2) * K;
                    double Ncoeff;

                    if ( !(mit1->outermost() || mit2->outermost()) ) {
                        // Computing S block first because it's needed for the corresponding N block
                        operatorS(*mit1, *mit2, mat, Scoeff, gauss_order);
                        Ncoeff = geo.sigma(*mit1, *mit2)/geo.sigma_inv(*mit1, *mit2);
                    } else {
                        Ncoeff = orientation * geo.sigma(*mit1, *mit2) * K;
                    }

                    if ( !mit1->outermost() ) {
                        // Computing D block
                        operatorD(*mit1, *mit2, mat, Dcoeff, gauss_order);
                    }
                    if ( ( *mit1 != *mit2 ) && ( !mit2->outermost() ) ) {
                        // Computing D* block
                        operatorD(*mit1, *mit2, mat, Dcoeff, gauss_order, true);
                    }

                    // Computing N block
                    operatorN(*mit1, *mit2, mat, Ncoeff, gauss_order);
                }
            }
        }

        // Deflate the diagonal block (N33) of 'mat' : (in order to have a zero-mean potential for the outermost interface)
        const Interface i = geo.outermost_interface();
        unsigned i_first = (*i.begin()->mesh().vertex_begin())->index();
        deflat(mat, i, mat(i_first, i_first) / (geo.outermost_interface().nb_vertices()));
    }

    void assemble_cortical(const Geometry& geo, Matrix& mat, const Head2EEGMat& M, const std::string& domain_name, const unsigned gauss_order, double alpha, double beta, const std::string &filename)
    {
        // Following the article: M. Clerc, J. Kybic "Cortical mapping by Laplace–Cauchy transmission using a boundary element method".
        // Assumptions:
        // - domain_name: the domain containing the sources is an innermost domain (defined as the interior of only one interface (called Cortex)
        // - Cortex interface is composed of one mesh only (no shared vertices)
        // TODO check orders of MxM products for efficiency ... delete intermediate matrices
        const Domain& SourceDomain = geo.domain(domain_name);
        const Interface& Cortex    = SourceDomain.begin()->interface();
        const Mesh& cortex         = Cortex.begin()->mesh();
        // test the assumption
        assert(SourceDomain.size() == 1);
        assert(Cortex.size() == 1);
        // shape of the new matrix:
        unsigned Nl = geo.size()-geo.outermost_interface().nb_triangles()-Cortex.nb_vertices()-Cortex.nb_triangles();
        unsigned Nc = geo.size()-geo.outermost_interface().nb_triangles();
        std::fstream f(filename.c_str());
        Matrix P;
        if ( !f ) {
            // build the HeadMat:
            // The following is the same as assemble_HM except N_11, D_11 and S_11 are not computed.
            SymMatrix mat_temp(Nc);
            mat_temp.set(0.0);
            double K = 1.0 / (4.0 * M_PI);
            // We iterate over the meshes (or pair of domains) to fill the lower half of the HeadMat (since its symmetry)
            for ( Geometry::const_iterator mit1 = geo.begin(); mit1 != geo.end(); ++mit1) {
                for ( Geometry::const_iterator mit2 = geo.begin(); (mit2 != (mit1+1)); ++mit2) {
                    // if mit1 and mit2 communicate, i.e they are used for the definition of a common domain
                    const int orientation = geo.oriented(*mit1, *mit2); // equals  0, if they don't have any domains in common
                    // equals  1, if they are both oriented toward the same domain
                    // equals -1, if they are not
                    if ( orientation != 0) {
                        double Scoeff =   orientation * geo.sigma_inv(*mit1, *mit2) * K;
                        double Dcoeff = - orientation * geo.indicator(*mit1, *mit2) * K;
                        double Ncoeff;
                        if ( !(mit1->outermost() || mit2->outermost()) && ( (*mit1 != *mit2)||( *mit1 != cortex) ) ) {
                            // Computing S block first because it's needed for the corresponding N block
                            operatorS(*mit1, *mit2, mat_temp, Scoeff, gauss_order);
                            Ncoeff = geo.sigma(*mit1, *mit2)/geo.sigma_inv(*mit1, *mit2);
                        } else {
                            Ncoeff = orientation * geo.sigma(*mit1, *mit2) * K;
                        }
                        if ( !mit1->outermost() && (( (*mit1 != *mit2)||( *mit1 != cortex) )) ) {
                            // Computing D block
                            operatorD(*mit1, *mit2, mat_temp, Dcoeff, gauss_order);
                        }
                        if ( ( *mit1 != *mit2 ) && ( !mit2->outermost() ) ) {
                            // Computing D* block
                            operatorD(*mit1, *mit2, mat_temp, Dcoeff, gauss_order, true);
                        }
                        // Computing N block
                        if ( (*mit1 != *mit2)||( *mit1 != cortex) ) {
                            operatorN(*mit1, *mit2, mat_temp, Ncoeff, gauss_order);
                        }
                    }
                }
            }
            // Deflate the diagonal block (N33) of 'mat' : (in order to have a zero-mean potential for the outermost interface)
            const Interface i = geo.outermost_interface();
            unsigned i_first = (*i.begin()->mesh().vertex_begin())->index();
            deflat(mat_temp, i, mat_temp(i_first, i_first) / (geo.outermost_interface().nb_vertices()));

            mat = Matrix(Nl, Nc);
            mat.set(0.0);
            // copy mat_temp into mat except the lines for cortex vertices [i_vb_c, i_ve_c] and cortex triangles [i_tb_c, i_te_c].
            unsigned iNl = 0;
            unsigned i_vb_c = (*cortex.vertex_begin())->index();
            unsigned i_ve_c = (*cortex.vertex_rbegin())->index();
            unsigned i_tb_c = cortex.begin()->index();
            unsigned i_te_c = cortex.rbegin()->index();
            for ( unsigned i = 0; i < Nc; ++i) {
                if ( !(i_vb_c<=i && i<=i_ve_c) && !(i_tb_c<=i && i<=i_te_c) ) {
                    mat.setlin(iNl, mat_temp.getlin(i));
                    ++iNl;
                }
            }
            // ** Construct P: the null-space projector **
            Matrix W;
            {
                Matrix U, s;
                mat.svd(U, s, W);
            }

            SparseMatrix S(Nc,Nc);
            // we set S to 0 everywhere, except in the last part of the diag:
            for ( unsigned i = Nl; i < Nc; ++i) {
                S(i, i) = 1.0;
            }
            P = (W * S) * W.transpose(); // P is a projector: P^2 = P and mat*P*X = 0
            if ( filename.length() != 0 ) {
                std::cout << "Saving projector P (" << filename << ")." << std::endl;
                P.save(filename);
            }
        } else {
            std::cout << "Loading projector P (" << filename << ")." << std::endl;
            P.load(filename);
        }

        // ** Get the gradient of P1&P0 elements on the meshes **
        Matrix MM(M.transpose() * M);
        SymMatrix RR(Nc, Nc); RR.set(0.);
        for ( Geometry::const_iterator mit = geo.begin(); mit != geo.end(); ++mit) {
            mit->gradient_norm2(RR);
        }

        // ** Choose Regularization parameter **
        SparseMatrix alphas(Nc,Nc); // diagonal matrix
        Matrix Z;
        if ( alpha < 0 ) { // try an automatic method... TODO find better estimation
            double nRR_v = RR.submat(0, geo.nb_vertices(), 0, geo.nb_vertices()).frobenius_norm();
            alphas.set(0.);
            alpha = MM.frobenius_norm() / (1.e3*nRR_v);
            beta  = alpha * 50000.;
            for ( Vertices::const_iterator vit = geo.vertex_begin(); vit != geo.vertex_end(); ++vit) {
                alphas(vit->index(), vit->index()) = alpha;
            }
            for ( Meshes::const_iterator mit = geo.begin(); mit != geo.end(); ++mit) {
                if ( !mit->outermost() ) {
                    for ( Mesh::const_iterator tit = mit->begin(); tit != mit->end(); ++tit) {
                        alphas(tit->index(), tit->index()) = beta;
                    }
                }
            }
            std::cout << "AUTOMATIC alphas = " << alpha << "\tbeta = " << beta << std::endl;
        } else {
            for ( Vertices::const_iterator vit = geo.vertex_begin(); vit != geo.vertex_end(); ++vit) {
                alphas(vit->index(), vit->index()) = alpha;
            }
            for ( Meshes::const_iterator mit = geo.begin(); mit != geo.end(); ++mit) {
                if ( !mit->outermost() ) {
                    for ( Mesh::const_iterator tit = mit->begin(); tit != mit->end(); ++tit) {
                        alphas(tit->index(), tit->index()) = beta;
                    }
                }
            }
            std::cout << "alphas = " << alpha << "\tbeta = " << beta << std::endl;
        }
        Z = P.transpose() * (MM + alphas*RR) * P;

        // ** PseudoInverse and return **
        // X = P * { (M*P)' * (M*P) + (R*P)' * (R*P) }¡(-1) * (M*P)'m
        // X = P * { P'*M'*M*P + P'*R'*R*P }¡(-1) * P'*M'm
        // X = P * { P'*(MM + a*RR)*P }¡(-1) * P'*M'm
        // X = P * Z¡(-1) * P' * M'm
        Matrix rhs = P.transpose() * M.transpose();
        mat = P * Z.pinverse() * rhs;
    }

    void assemble_Surf2Vol(const Geometry& geo, Matrix& mat, const std::map<const Domain, Vertices> m_points) 
    {
        const double K = 1.0/(4.0*M_PI);

        unsigned size = 0; // total number of inside points
        for ( std::map<const Domain, Vertices>::const_iterator dvit = m_points.begin(); dvit != m_points.end(); ++dvit) {
            size += dvit->second.size();
        }

        mat = Matrix(size, (geo.size() - geo.outermost_interface().nb_triangles()));
        mat.set(0.0);

        for ( std::map<const Domain, Vertices>::const_iterator dvit = m_points.begin(); dvit != m_points.end(); ++dvit) {
            for ( Geometry::const_iterator mit = geo.begin(); mit != geo.end(); ++mit) {
                int orientation = dvit->first.mesh_orientation(*mit);
                if ( orientation != 0 ) {
                    operatorDinternal(*mit, mat, dvit->second, orientation * -1. * K);
                    if ( !mit->outermost() ) {
                        operatorSinternal(*mit, mat, dvit->second, orientation * K / geo.sigma(dvit->first));
                    }
                }
            }
        }
    }

    HeadMat::HeadMat(const Geometry& geo, const unsigned gauss_order)
    {
        assemble_HM(geo, *this, gauss_order);
    }

    CorticalMat::CorticalMat(const Geometry& geo, const Head2EEGMat& M, const std::string& domain_name, const unsigned gauss_order, double a, double b, const std::string &filename)
    {
        assemble_cortical(geo, *this, M, domain_name, gauss_order, a, b, filename);
    }

    Surf2VolMat::Surf2VolMat(const Geometry& geo, const Matrix& points) 
    {
        std::map<const Domain, Vertices> m_points;

        unsigned index = 0;
        // Find the points per domain and generate the indices for the m_points
        for ( unsigned i = 0; i < points.nlin(); ++i) {
            const Domain domain = geo.domain(Vect3(points(i, 0), points(i, 1), points(i, 2)));
            if ( domain.name() == "Air" ) {
                std::cerr << " Surf2Vol: Point [ " << points.getlin(i);
                std::cerr << "] is outside the head. Point is dropped." << std::endl;
            } else {
                m_points[domain].push_back(Vertex(points(i, 0), points(i, 1), points(i, 2), index++));
            }
        }

        assemble_Surf2Vol(geo, *this, m_points);
    }
} // namespace OpenMEEG
