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

#include <mesh.h>
#include <Triangle_triangle_intersection.h>

namespace OpenMEEG {

    Mesh::Mesh(const Mesh& m) 
    {
        *this = m;
    }

    Mesh& Mesh::operator=(const Mesh& m) 
    {
        if ( this != &m ) {
            copy(m);
        }
        return *this;
    }

    void Mesh::copy(const Mesh& m) 
    {
        if ( m.allocate_ ) {
            allocate_     = true;
            all_vertices_ = new Vertices; // allocates space for the vertices and copy
            all_vertices_->reserve(m.nb_vertices()); 
            std::map<const Vertex *, Vertex *> map; // for the triangles
            for ( Mesh::const_vertex_iterator vit = m.vertex_begin(); vit != m.vertex_end(); ++vit) {
                add_vertex(**vit);
                map[*vit] = *vertices_.rbegin();
            }
            for ( Triangles::const_iterator tit = m.begin(); tit != m.end(); ++tit) {
                Triangle t(map[&tit->s1()], map[&tit->s2()], map[&tit->s3()]);
                push_back(t);
            }
            update();
        } else {
            all_vertices_ = m.all_vertices_; // allocates space for the vertices and copy
            set_vertices_ = m.set_vertices_;
            allocate_ = false;
            for ( Triangles::const_iterator tit = m.begin(); tit != m.end(); ++tit) {
                push_back(*tit);
            }
        }
        
        outermost_ = m.outermost_;
        name_      = m.name_;
    }

    /// Print informations about the mesh 
    void Mesh::info() const 
    {
        std::cout << "Info:: Mesh name/ID : "  << name() << std::endl;
        std::cout << "\t\t# vertices  : " << nb_vertices() << std::endl;
        std::cout << "\t\t# triangles : " << nb_triangles() << std::endl;
        std::cout << "\t\tEuler characteristic : " << nb_vertices() - 3.*nb_triangles()/2. + nb_triangles() << std::endl;

        double min_area = std::numeric_limits<double>::max();
        double max_area = 0.;
        for ( const_iterator tit = begin(); tit != end(); ++tit) {
            min_area = std::min(tit->area(), min_area);
            max_area = std::max(tit->area(), max_area);
        }
        std::cout << "\t\tMin Area : " << min_area << std::endl;
        std::cout << "\t\tMax Area : " << max_area << std::endl;
    }

    void Mesh::build_mesh_vertices()
    {
        // Sets do not preserve the order, and we would like to preserve it so we push_back in the vector as soon as the element is unique.
        std::set<const Vertex *> mesh_v;
        vertices_.clear();
        for ( const_iterator tit = begin(); tit != end(); ++tit) {
            for ( Triangle::const_iterator sit = tit->begin(); sit != tit->end(); ++sit) {
                if ( mesh_v.insert(*sit).second ) {
                    vertices_.push_back(const_cast<Vertex *>(*sit));
                }
            }
        }
    }

    void Mesh::destroy() 
    {
        if ( allocate_ ) {
            delete all_vertices_;
        }
        clear();
        all_vertices_ = 0;
        vertices_.clear();
        set_vertices_.clear();
        name_.clear();
        links_.clear();
        outermost_ = false;
        allocate_ = false;
    }

    std::istream& operator>>(std::istream& is, Mesh& m) 
    {
        unsigned a, b, c;
        is >> a >> b >> c;
        Triangle t(m.vertices_[a], m.vertices_[b], m.vertices_[c] );
        m.push_back(t);
        return is;
    }

    /// properly add vertex to the list. (if not already added)
    void Mesh::add_vertex(const Vertex& v) 
    {
        // try to insert the vertex to the set
        std::pair<std::set<Vertex>::iterator, bool> ret = set_vertices_.insert(v);
        if ( ret.second ) {
            // if inserted, then it is a new vertex, and we add it to both lists
            all_vertices_->push_back(v);
            vertices_.push_back(&(*all_vertices_->rbegin()));
        } else {
            // if not inserted, Either it belongs to another mesh or it was dupplicated in the same mesh
            // TODO this may take time for too big redundant mesh
            Vertices::iterator vit = std::find(all_vertices_->begin(), all_vertices_->end(), v);
            if ( std::find(vertices_.begin(), vertices_.end(), &(*vit)) == vertices_.end() ) {
                vertices_.push_back(&(*vit));
            }
        }
    }

    /// Update triangles area/normal, update links and vertices normals if needed
    void Mesh::update() 
    {
        // empty unessacary set
        set_vertices_.clear();

        // computes the triangles normals
        for ( iterator tit = begin(); tit != end(); ++tit) {
            tit->normal()  = (tit->s1() - tit->s2())^(tit->s1() - tit->s3());
            tit->area()    = tit->normal().norm() / 2.0;
            tit->normal() /= tit->normal().norm();
        }

        // make links
        links_.clear();
        for ( const_iterator tit = begin(); tit != end(); ++tit) {
            for ( Triangle::const_iterator sit = tit->begin(); sit != tit->end(); ++sit) {
                links_[*sit].push_back(const_cast<Triangle *>(&*tit));
            }
        }

        // recompute the vertex normals // TODO useless are vertex normals
        bool first_time = true;
        for ( const_vertex_iterator vit = vertex_begin(); vit != vertex_end(); ++vit) {
            if ( (*vit)->normal().norm() < 1.e3*std::numeric_limits<double>::min() ) {
                if ( first_time ) {
                    std::cout << "Recompute the normals for each vertex" << std::endl;
                    first_time = false;
                }
                Normal normal(0);
                for ( VectPTriangle::const_iterator tit = links_[*vit].begin(); tit != links_[*vit].end(); ++tit) {
                    normal += (*tit)->normal();
                }
                normal.normalize();
                (*vit)->normal() = normal;
            }
        }
        correct_local_orientation();
    }

    /// properly merge two meshes into one (it does not dupplicate vertices)
    void  Mesh::merge(const Mesh& m1, const Mesh& m2) 
    {
        if ( size() != 0 ) {
            warning("Mesh::merge Mesh must be empty.");
        }
        allocate_ = true;
        all_vertices_ = new Vertices;
        all_vertices_->reserve(m1.nb_vertices() + m2.nb_vertices());
        for ( Mesh::const_vertex_iterator vit = m1.vertex_begin(); vit != m1.vertex_end(); ++vit) {
            add_vertex(**vit);
        }
        for ( Mesh::const_vertex_iterator vit = m2.vertex_begin(); vit != m2.vertex_end(); ++vit) {
            add_vertex(**vit);
        }
        for ( const_iterator tit = m1.begin(); tit != m1.end(); ++tit) {
            Vertices::iterator vit1 = std::find(all_vertices_->begin(), all_vertices_->end(), tit->s1());
            Vertices::iterator vit2 = std::find(all_vertices_->begin(), all_vertices_->end(), tit->s2());
            Vertices::iterator vit3 = std::find(all_vertices_->begin(), all_vertices_->end(), tit->s3());
            push_back(Triangle(*vit1, *vit2, *vit3));
        }
        for ( const_iterator tit = m2.begin(); tit != m2.end(); ++tit) {
            Vertices::iterator vit1 = std::find(all_vertices_->begin(), all_vertices_->end(), tit->s1());
            Vertices::iterator vit2 = std::find(all_vertices_->begin(), all_vertices_->end(), tit->s2());
            Vertices::iterator vit3 = std::find(all_vertices_->begin(), all_vertices_->end(), tit->s3());
            push_back(Triangle(*vit1, *vit2, *vit3));
        }
        update();
    }

    /// Flip all triangles
    void Mesh::flip_triangles() 
    {
        for ( iterator tit = begin(); tit != end(); ++tit)
        {
            tit->flip();
        }
    }

    /// Smooth Mesh
    void Mesh::smooth(const double& smoothing_intensity, const unsigned& niter) 
    {
        std::vector< std::set<Vertex> > neighbors(nb_vertices());
        unsigned i = 0;
        for ( const_vertex_iterator vit = vertex_begin(); vit != vertex_end(); ++vit, ++i) {
            for ( VectPTriangle::const_iterator tit = links_[*vit].begin(); tit != links_[*vit].end(); ++tit) {
                for ( unsigned  k = 0; k < 3; ++k) {
                    if ( (**tit)(k) == **vit ) {
                        neighbors[i].insert((**tit)(k));
                    }
                }
            }
        }

        Vertices new_pts(nb_vertices());
        for ( unsigned n = 0; n < niter; ++n) {
            i = 0;
            for ( const_vertex_iterator vit = vertex_begin(); vit != vertex_end(); ++vit, ++i) {
                new_pts.push_back(**vit);
                for (std::set<Vertex>::const_iterator it = neighbors[i].begin(); it != neighbors[i].end(); ++it) {
                    new_pts[i] = new_pts[i] + (smoothing_intensity * (*it - **vit)) / neighbors[i].size();
                }
            }
            for ( vertex_iterator vit = vertex_begin(); vit != vertex_end(); ++vit) {
                **vit = new_pts[i];
                new_pts.clear();
            }
        }
        update(); // Updating triangles (areas + normals)
    }

    bool Mesh::has_self_intersection() const 
    {
        bool selfIntersects = false;
        for ( const_iterator tit1 = begin(); tit1 != end(); ++tit1) {
            for ( const_iterator tit2 = tit1; tit2 != end(); ++tit2) {
                if ( !tit1->contains(tit2->s1()) && !tit1->contains(tit2->s2()) && !tit1->contains(tit1->s3()) ) {
                    if ( triangle_intersection(*tit1, *tit2) ) {
                        selfIntersects = true;
                        std::cout << "Triangles " << tit1->index() << " and " << tit2->index() << " are intersecting." << std::endl;
                    }
                }
            }
        }
        return selfIntersects;
    }

    bool Mesh::intersection(const Mesh& m) const 
    {
        bool intersects = false;
        for ( const_iterator tit1 = begin(); tit1 != end(); ++tit1) {
            for ( const_iterator tit2 = m.begin(); tit2 != m.end(); ++tit2) {
                intersects = intersects | triangle_intersection(*tit1, *tit2);
            }
        }
        return intersects;
    }

    bool Mesh::triangle_intersection(const Triangle& T1, const Triangle& T2 ) const 
    {
        const Vect3& p1 = T1.s1();
        const Vect3& q1 = T1.s2();
        const Vect3& r1 = T1.s3();
        const Vect3& p2 = T2.s1();
        const Vect3& q2 = T2.s2();
        const Vect3& r2 = T2.s3();

        double pp1[3] = {p1.x(), p1.y(), p1.z()};
        double qq1[3] = {q1.x(), q1.y(), q1.z()};
        double rr1[3] = {r1.x(), r1.y(), r1.z()};
        double pp2[3] = {p2.x(), p2.y(), p2.z()};
        double qq2[3] = {q2.x(), q2.y(), q2.z()};
        double rr2[3] = {r2.x(), r2.y(), r2.z()};
        return tri_tri_overlap_test_3d(pp1, qq1, rr1, pp2, qq2, rr2);
    }

    const Mesh::VectPTriangle& Mesh::get_triangles_for_vertex(const Vertex& V) const 
    {
        std::map<const Vertex *, Mesh::VectPTriangle>::const_iterator it = links_.find(const_cast<Vertex *>(&V));
        if ( it != links_.end() ) {
            return it->second;
        } else {
            static Mesh::VectPTriangle a;
            return a;
        }
        // TODO const_cast here
    }

    /// For IO:s -------------------------------------------------------------------------------------------
    unsigned Mesh::load(const std::string& filename, const bool& verbose, const bool& read_all) 
    {
        if ( size() != 0 ) {
            destroy();
        }

        if ( read_all && ( all_vertices_ == 0 ) ) {
            unsigned nb_v = load(filename, false, false); // first allocates memory for the vertices
            all_vertices_ = new Vertices;
            all_vertices_->reserve(nb_v); 
            allocate_ = true;
        }

        std::string extension = getNameExtension(filename);
        std::transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int))std::tolower);
        unsigned return_value = 0;

        if ( verbose ) {
            std::cout << "loading : " << filename << " as a \"" << extension << "\" file."<< std::endl;
        }

        if ( extension == std::string("vtk") ) {
            return_value = load_vtk(filename, read_all);
        } else if ( extension == std::string("tri") ) {
            return_value = load_tri(filename, read_all);
        } else if ( extension == std::string("bnd") ) {
            return_value = load_bnd(filename, read_all);
        } else if ( extension == std::string("mesh") ) {
            return_value = load_mesh(filename, read_all);
        } else if ( extension == std::string("off") ) {
            return_value = load_off(filename, read_all);
        } else if ( extension == std::string("gii") ) {
            return_value = load_gifti(filename, read_all);
        } else {
            std::cerr << "IO: load: Unknown mesh file format for " << filename << std::endl;
            exit(1);
        }

        if ( read_all ) {
            update();
        }

        if ( verbose ) {
            info();
        }

        if ( allocate_ && read_all ) { // we generates the indices of these mesh vertices
            unsigned index = 0;
            for ( Mesh::vertex_iterator vit = vertex_begin(); vit != vertex_end(); ++vit, ++index) {
                (*vit)->index() = index;
            }
            index = 0;
            for ( Mesh::iterator tit = begin(); tit!= end(); ++tit, ++index) {
                tit->index() = index;
            }
        } 

        return return_value;
    }

    void Mesh::save(const std::string& filename) const
    {
        std::string extension = getNameExtension(filename);

        std::transform(extension.begin(), extension.end(), extension.begin(), (int(*)(int))std::tolower);

        if ( extension==std::string("vtk") ) {
            save_vtk(filename);
        } else if ( extension==std::string("tri") ) {
            save_tri(filename);
        } else if ( extension==std::string("bnd") ) {
            save_bnd(filename);
        } else if ( extension==std::string("mesh") ) {
            save_mesh(filename);
        } else if ( extension==std::string("off") ) {
            save_off(filename);
        } else if ( extension==std::string("gii") ) {
            save_gifti(filename);
        } else {
            std::cerr << "Unknown file format for : " << filename << std::endl;
            exit(1);
        }
    }

    #ifdef USE_VTK
    unsigned Mesh::get_data_from_vtk_reader(vtkPolyDataReader* reader, const bool& read_all)
    {
        reader->Update();
        vtkPolyData *vtkMesh = reader->GetOutput();

        unsigned npts, ntrgs;
        npts = vtkMesh->GetNumberOfPoints();

        if ( !read_all ) {
            return npts;
        }

        if ( reader->GetNumberOfNormalsInFile() == 0 ) {
            vtkPolyDataNormals *newNormals = vtkPolyDataNormals::New();
            newNormals->SetInput(vtkMesh);
            newNormals->Update();
            vtkMesh = newNormals->GetOutput();
        }

        vtkDataArray *normalsData = vtkMesh->GetPointData()->GetNormals();

        if ( npts != normalsData->GetNumberOfTuples() ) {
            std::cerr << "Error: number of vertices is not equal to number of normals in vtk file, correct or remove the normals." << std::endl;
            exit(1);
        }

        if ( normalsData->GetNumberOfComponents() != 3 ) {
            std::cerr << "Error: wrong number of components of normals in vtk file, correct or remove the normals." << std::endl;
            exit(1);
        }

        for ( unsigned i = 0; i < npts; ++i) {
            add_vertex(Vertex(vtkMesh->GetPoint(i)[0], vtkMesh->GetPoint(i)[1], vtkMesh->GetPoint(i)[2], normalsData->GetTuple(i)[0], normalsData->GetTuple(i)[1], normalsData->GetTuple(i)[2]));
        }

        ntrgs = vtkMesh->GetNumberOfCells();

        vtkIdList *l;
        reserve(ntrgs);

        for ( unsigned i = 0; i < ntrgs; ++i) {
            if ( vtkMesh->GetCellType(i) == VTK_TRIANGLE) {
                l = vtkMesh->GetCell(i)->GetPointIds();
                push_back(Triangle(vertices()[l->GetId(0)],
                                   vertices()[l->GetId(1)],
                                   vertices()[l->GetId(2)]))  ;
            } else {
                std::cerr << "Mesh \"" << name_ << "\" is not a triangulation" << std::endl;
                exit(1);
            }
        }

        return 0;
    }

    unsigned Mesh::load_vtk(std::istream& is, const bool& read_all)
    {
        // get length of file:
        is.seekg (0, ios::end);
        int length = is.tellg();
        is.seekg (0, ios::beg);

        // allocate memory:
        char * buffer = new char [length];

        // read data as a block:
        is.read (buffer, length);

        // held buffer by the array buf:
        vtkCharArray* buf = vtkCharArray::New();
        buf->SetArray(buffer, length, 1);

        vtkPolyDataReader* reader = vtkPolyDataReader::New();
        reader->SetInputArray(buf); // Specify 'buf' to be used when reading from a string
        reader->SetReadFromInputString(1);  // Enable reading from the InputArray 'buf' instead of the default, a file

        unsigned return_value = 0;
        return_value = get_data_from_vtk_reader(reader, read_all);

        delete[] buffer;
        reader->Delete();

        return return_value;
    }

    unsigned Mesh::load_vtk(const std::string& filename, const bool& read_all)
    {
        std::string s = filename;
        vtkPolyDataReader *reader = vtkPolyDataReader::New();
        reader->SetFileName(filename.c_str()); // Specify file name of vtk data file to read
        if ( !reader->IsFilePolyData()) {
            std::cerr << "Mesh \"" << name_ << "\" is not a valid vtk poly data file" << std::endl;
            reader->Delete();
            exit(1);
        }

        unsigned return_value = 0;
        return_value = get_data_from_vtk_reader(reader, read_all);
        return return_value;
    }
    #endif

    #ifdef USE_GIFTI
    unsigned Mesh::load_gifti(const std::string& filename, const bool& read_all)
    {   
        // Use gifti_io API
        int read_data = 0; 
        gifti_image* gim = gifti_read_image(filename.c_str(), read_data);
        assert(gim->numDA >= 2);
        // find which array contains the points and which the triangles
        unsigned ipts, itrgs;
        unsigned iit = 0;
        while ( iit < gim->numDA ) {
            if ( gim->darray[iit]->intent == NIFTI_INTENT_POINTSET ) {
                ipts = iit;
            } else if ( gim->darray[iit]->intent == NIFTI_INTENT_TRIANGLE ) {
                itrgs = iit;
            }
            ++iit;
        }
        assert(gim->darray[ipts]->dims[1] == 3); // 3D points
        assert(gim->darray[itrgs]->dims[1] == 3); // 3 indices per triangle
        unsigned npts  = gim->darray[ipts]->dims[0];
        unsigned ntrgs = gim->darray[itrgs]->dims[0];
        if ( !read_all ) { 
            gifti_free_image(gim);
            return npts; 
        }
        read_data = 1; // now, load the data
        gim = gifti_read_image(filename.c_str(), read_data);
        
        float * pts_data = (float *)gim->darray[ipts]->data;
        for ( unsigned i = 0; i < npts; ++i) {
           add_vertex(Vertex(pts_data[i],
                             pts_data[i+npts], 
                             pts_data[i+2*npts]));
        }

        reserve(ntrgs);
        unsigned * trgs_data = (unsigned *)gim->darray[itrgs]->data;
        for ( unsigned i = 0; i < ntrgs; ++i) {
            push_back(Triangle(vertices_[trgs_data[i]],
                               vertices_[trgs_data[i+ntrgs]],
                               vertices_[trgs_data[i+2*ntrgs]]));
        }

        // free all
        gifti_free_image(gim);
        return 0;
    }
    #endif

    unsigned Mesh::load_mesh(std::istream& is, const bool& read_all)
    {
        unsigned char* uc = new unsigned char[5]; // File format
        is.read((char*)uc, sizeof(unsigned char)*5);
        delete[] uc;

        uc = new unsigned char[4]; // lbindian
        is.read((char*)uc, sizeof(unsigned char)*4);
        delete[] uc;

        unsigned int* ui = new unsigned int[1]; // arg_size
        is.read((char*)ui, sizeof(unsigned int));
        unsigned int arg_size = ui[0];
        delete[] ui;

        uc = new unsigned char[arg_size]; // Trash
        is.read((char*)uc, sizeof(unsigned char)*arg_size);
        delete[] uc;

        ui = new unsigned int[1]; // vertex_per_face
        is.read((char*)ui, sizeof(unsigned int));
        unsigned int vertex_per_face = ui[0];
        delete[] ui;

        ui = new unsigned int[1]; // mesh_time
        is.read((char*)ui, sizeof(unsigned int));
        unsigned int mesh_time = ui[0];
        delete[] ui;

        ui = new unsigned int[1]; // mesh_step
        is.read((char*)ui, sizeof(unsigned int));
        delete[] ui;

        ui = new unsigned int[1]; // vertex number
        is.read((char*)ui, sizeof(unsigned int));
        unsigned npts = ui[0];
        
        if ( !read_all ) { 
            return npts; 
        }

        delete[] ui;

        assert(vertex_per_face == 3); // Support only for triangulations
        assert(mesh_time == 1); // Support only 1 time frame

        float* pts_raw = new float[npts*3]; // Points
        is.read((char*)pts_raw, sizeof(float)*npts*3);

        ui = new unsigned int[1]; // arg_size
        is.read((char*)ui, sizeof(unsigned int));
        delete[] ui;

        float* normals_raw = new float[npts*3]; // Normals
        is.read((char*)normals_raw, sizeof(float)*npts*3);

        ui = new unsigned int[1]; // arg_size
        is.read((char*)ui, sizeof(unsigned int));
        delete[] ui;

        ui = new unsigned int[1]; // number of faces
        is.read((char*)ui, sizeof(unsigned int));
        unsigned ntrgs;
        ntrgs = ui[0];
        delete[] ui;

        unsigned int* faces_raw = new unsigned int[ntrgs*3]; // Faces
        is.read((char*)faces_raw, sizeof(unsigned int)*ntrgs*3);

        for ( unsigned i = 0; i < npts; ++i) {
            add_vertex(Vertex(pts_raw[i*3+0], 
                              pts_raw[i*3+1], 
                              pts_raw[i*3+2], 
                              normals_raw[i*3+0], 
                              normals_raw[i*3+1], 
                              normals_raw[i*3+2]));
        }

        reserve(ntrgs);

        for ( unsigned i = 0; i < ntrgs; ++i) {
            push_back(Triangle(vertices_[faces_raw[i*3+0]], 
                               vertices_[faces_raw[i*3+1]],
                               vertices_[faces_raw[i*3+2]]));
        }

        delete[] faces_raw;
        delete[] normals_raw;
        delete[] pts_raw;
        return 0;
    }

    unsigned Mesh::load_mesh(const std::string& filename, const bool& read_all)
    {
        std::ifstream f(filename.c_str(), std::ios::binary);
        if ( !f.is_open()) {
            std::cerr << "Error opening MESH file: " << filename << std::endl;
            exit(1);
        }
        unsigned return_value = 0;
        return_value = load_mesh(f, read_all);
        f.close();
        return return_value;
    }

    unsigned Mesh::load_tri(std::istream& f, const bool& read_all)
    {
        f.seekg( 0, std::ios_base::beg );

        char ch;
        unsigned npts, ntrgs;
        f >> ch;
        f >> npts;

        if ( !read_all ) {
            return npts;
        }

        for ( unsigned i = 0; i < npts; ++i) {
            Vertex v;
            Normal n;
            f >> v >> v.normal();
            add_vertex(v);
        }
        f >> ch >> ntrgs >> ntrgs >> ntrgs; // This number is repeated 3 times

        reserve(ntrgs);

        for ( unsigned i = 0; i < ntrgs; ++i) {
            f >> *this;
        }

        return 0;
    }

    unsigned Mesh::load_tri(const std::string& filename, const bool& read_all)
    {
        std::string s = filename;
        std::ifstream f(filename.c_str());
        if ( !f.is_open() ) {
            std::cerr << "Error opening TRI file: " << filename << std::endl;
            exit(1);
        }
        unsigned return_value = 0;
        return_value = load_tri(f, read_all);
        f.close();
        return return_value;
    }

    unsigned Mesh::load_bnd(std::istream& f, const bool& read_all)
    {
        std::string line;
        std::string st;

        f.seekg( 0, std::ios_base::beg );

        f >> io_utils::skip_comments('#') >> st;
        if ( st == "Type=") {
            io_utils::skip_line(f);
            f >> io_utils::skip_comments('#') >> st;
        }

        assert(st == "NumberPositions=");
        unsigned npts, ntrgs;
        f >> npts;

        if ( !read_all ) {
            return npts;
        }

        f >> io_utils::skip_comments('#') >> st;
        if ( st == "UnitPosition") {
            io_utils::skip_line(f); // skip : "UnitPosition mm"
        }

        f >> io_utils::skip_comments('#') >> st;
        assert(st == "Positions");

        for( unsigned i = 0; i < npts; ++i ) {
            Vertex v;
            f >> io_utils::skip_comments('#') >> v;
            add_vertex(v);
        }

        f >> io_utils::skip_comments('#') >> st;
        assert(st == "NumberPolygons=");
        f >> io_utils::skip_comments('#') >> ntrgs;

        f >> io_utils::skip_comments('#') >> st;
        assert(st == "TypePolygons=");
        f >> io_utils::skip_comments('#') >> st;
        assert(st == "3");

        f >> io_utils::skip_comments('#') >> st;
        assert(st == "Polygons");

        reserve(ntrgs);

        for ( unsigned i = 0; i < ntrgs; ++i) {
             f >> io_utils::skip_comments('#') >> *this;
        }

        return 0;
    }

    unsigned Mesh::load_bnd(const std::string& filename, const bool& read_all)
    {        
        std::string s = filename;
        std::ifstream f(filename.c_str());

        if ( !f.is_open() ) {
            std::cerr << "Error opening BND file: " << filename << std::endl;
            exit(1);
        }
        unsigned return_value = 0;
        return_value = load_bnd(f, read_all);
        f.close();
        return return_value;
    }

    unsigned Mesh::load_off(std::istream& f, const bool& read_all)
    {
        char tmp[128];
        int trash;
        f >> tmp;        // put the "OFF" string
        unsigned npts, ntrgs;
        f >> npts;
        f >> ntrgs;
        f >> trash;

        if ( !read_all ) {
            return npts;
        }

        for ( unsigned i = 0; i < npts; ++i) {
            Vertex v;
            f >> v;
            add_vertex(v);
        }

        reserve(ntrgs);

        for ( unsigned i = 0; i < ntrgs; ++i) {
            f >> trash;        // put the "3" to trash
            f >> *this;
        }

        return 0;
    }

    unsigned Mesh::load_off(const std::string& filename, const bool& read_all) 
    {
        std::string s = filename;
        std::ifstream f(filename.c_str());
        if ( !f.is_open() ) {
            std::cerr << "Error opening OFF file: " << filename << std::endl;
            exit(1);
        }
        unsigned return_value = 0;
        return_value = load_off(f, read_all);
        f.close();
        return return_value;
    }
    
    void Mesh::save_vtk(const std::string& filename) const 
    {
        std::ofstream os(filename.c_str());
        os << "# vtk DataFile Version 2.0" << std::endl;
        os << "File " << filename << " generated by OpenMEEG" << std::endl;
        os << "ASCII" << std::endl;
        os << "DATASET POLYDATA" << std::endl;
        os << "POINTS " << nb_vertices() << " float" << std::endl;

        std::map<const Vertex *, unsigned> map;
        unsigned i = 0;
        for ( const_vertex_iterator vit = vertex_begin(); vit != vertex_end(); ++vit, ++i) {
            map[*vit] = i;
            os << **vit << std::endl;
        }
        os << "POLYGONS " << nb_triangles() << " " << nb_triangles()*4 << std::endl;
        for ( const_iterator tit = begin(); tit != end(); ++tit) {
            os << "3 " << map[&(tit->s1())] << " " << map[&(tit->s2())] << " " << map[&(tit->s3())] << std::endl;
        }

        os << "CELL_DATA " << nb_triangles() << std::endl;
        os << "POINT_DATA " << nb_vertices() << std::endl;
        os << "NORMALS normals float" << std::endl;
        for ( const_vertex_iterator vit = vertex_begin(); vit != vertex_end(); ++vit) {
            os << (*vit)->normal() << std::endl;
        }

        os.close();
    }

    void Mesh::save_bnd(const std::string& filename) const
    {
        std::ofstream os(filename.c_str());
        os << "# Bnd mesh file generated by OpenMeeg" << std::endl;
        os << "Type= Unknown" << std::endl;
        os << "NumberPositions= " << nb_vertices() << std::endl;
        os << "UnitPosition\tmm" << std::endl;
        os << "Positions" << std::endl;
        std::map<const Vertex *, unsigned> map;
        unsigned i = 0;
        for ( const_vertex_iterator vit = vertex_begin(); vit != vertex_end(); ++vit, ++i) {
            map[*vit] = i;
            os << **vit << std::endl;
        }
        os << "NumberPolygons= " << nb_triangles() << std::endl;
        os << "TypePolygons=\t3" << std::endl;
        os << "Polygons" << std::endl;
        for ( const_iterator tit = begin(); tit != end(); ++tit) {
            os << map[&(tit->s1())] << " " << map[&(tit->s2())] << " " << map[&(tit->s3())] << std::endl;
        }

        os.close();
    }

    void Mesh::save_tri(const std::string& filename) const
    {
        std::ofstream os(filename.c_str());
        os << "- " << nb_vertices() << std::endl;
        std::map<const Vertex *, unsigned> map;
        unsigned i = 0;
        for ( const_vertex_iterator vit = vertex_begin(); vit != vertex_end(); ++vit, ++i) {
            map[*vit] = i;
            os << **vit << " " << (*vit)->normal() << std::endl;
        }
        os << "- " << nb_triangles() << " " << nb_triangles() << " " << nb_triangles() << std::endl;
        for ( const_iterator tit = begin(); tit != end(); ++tit) {
            os << map[&(tit->s1())] << " " << map[&(tit->s2())] << " " << map[&(tit->s3())] << std::endl;
        }

        os.close();
    }

    void Mesh::save_off(const std::string& filename) const
    {
        std::ofstream os(filename.c_str());
        os << "OFF" << std::endl;
        os << nb_vertices() << " " << nb_triangles() << " 0" << std::endl;
        std::map<const Vertex *, unsigned> map;
        unsigned i = 0;
        for ( const_vertex_iterator vit = vertex_begin(); vit != vertex_end(); ++vit, ++i) {
            map[*vit] = i;
            os << **vit << std::endl;
        }
        for ( const_iterator tit = begin(); tit != end(); ++tit) {
            os << "3 " << map[&(tit->s1())] << " " << map[&(tit->s2())] << " " << map[&(tit->s3())] << std::endl;
        }

        os.close();
    }

    void Mesh::save_mesh(const std::string& filename) const
    {
        std::ofstream os(filename.c_str(), std::ios::binary);

        unsigned char format[5] = {'b', 'i', 'n', 'a', 'r'}; // File format
        os.write((char*)format, sizeof(unsigned char)*5);

        unsigned char lbindian[4] = {'D', 'C', 'B', 'A'}; // lbindian
        os.write((char*)lbindian, sizeof(unsigned char)*4);

        unsigned int arg_size[1] = {4}; // arg_size
        os.write((char*)arg_size, sizeof(unsigned int));

        unsigned char VOID[4] = {'V', 'O', 'I', 'D'}; // Trash
        os.write((char*)VOID, sizeof(unsigned char)*4);

        unsigned int vertex_per_face[1] = {3}; // vertex_per_face
        os.write((char*)vertex_per_face, sizeof(unsigned int));

        unsigned int mesh_time[1] = {1}; // mesh_time
        os.write((char*)mesh_time, sizeof(unsigned int));

        unsigned int mesh_step[1] = {0}; // mesh_step
        os.write((char*)mesh_step, sizeof(unsigned int));

        unsigned int vertex_number[1] = {nb_vertices()}; // vertex number
        os.write((char*)vertex_number, sizeof(unsigned int));

        float* pts_raw = new float[nb_vertices()*3]; // Points
        float* normals_raw = new float[nb_vertices()*3]; // Normals
        unsigned int* faces_raw = new unsigned int[nb_triangles()*3]; // Faces

        std::map<const Vertex *, unsigned> map;
        unsigned i = 0;

        for ( const_vertex_iterator vit = vertex_begin(); vit != vertex_end(); ++vit, ++i) {
            map[*vit] = i;
            pts_raw[i*3+0]     = (float)((*vit)->x());
            pts_raw[i*3+1]     = (float)((*vit)->y());
            pts_raw[i*3+2]     = (float)((*vit)->z());
            normals_raw[i*3+0] = (float)((*vit)->normal().x());
            normals_raw[i*3+1] = (float)((*vit)->normal().y());
            normals_raw[i*3+2] = (float)((*vit)->normal().z());
        }

        i = 0;
        for ( const_iterator tit = begin(); tit != end(); ++tit, ++i) {
            faces_raw[i*3+0] = map[&(tit->s1())];
            faces_raw[i*3+1] = map[&(tit->s2())];
            faces_raw[i*3+2] = map[&(tit->s3())];
        }

        os.write((char*)pts_raw, sizeof(float)*nb_vertices()*3);           // vertices
        os.write((char*)vertex_number, sizeof(unsigned int));              // arg size : npts
        os.write((char*)normals_raw, sizeof(float)*nb_vertices()*3);       // normals
        unsigned char zero[1] = {0};
        os.write((char*)zero, sizeof(unsigned int));                       // arg size : 0
        unsigned int faces_number[1] = {nb_triangles()};
        os.write((char*)faces_number, sizeof(unsigned int));               // ntrgs
        os.write((char*)faces_raw, sizeof(unsigned int)*nb_triangles()*3); // triangles

        delete[] faces_raw;
        delete[] normals_raw;
        delete[] pts_raw;
        os.close();
    }

    const Mesh::EdgeMap Mesh::compute_edge_map() const 
    {
        // define the triangle edges as (first vertex, second vertex)
        // if a triangle edge is ordered with (lower index, higher index) add 1 to its map else remove 1
        // in the end each mapping should be: 0 (well oriented), 1 for border edge (non closed), and 2 for non well oriented
        EdgeMap mape; // map the edges with an unsigned
        for ( const_iterator tit = begin(); tit != end(); ++tit) {
            for ( unsigned j = 0; j < 3; ++j) {
                if ( (*tit)[j]->index() > (*tit)[j+1]->index() ) {
                    std::pair<const Vertex *, const Vertex *> pairv((*tit)[j], (*tit)[j+1]);
                    if ( mape.count(pairv) == 0 ) {
                        mape[pairv] = 1;
                    } else {
                        mape[pairv]++;
                    }
                } else {
                    std::pair<const Vertex *, const Vertex *> pairv((*tit)[j+1], (*tit)[j]);
                    if ( mape.count(pairv) == 0 ) {
                        mape[pairv] = -1;
                    } else {
                        mape[pairv]--;
                    }
                }
            }
        }
        return mape;
    }

    /// get the 3 adjacents triangles of a triangle t
    Mesh::VectPTriangle Mesh::adjacent_triangles(const Triangle& t)
    {
        VectPTriangle tris;
        std::map<Triangle *, unsigned> mapt;
        for ( Triangle::const_iterator sit = t.begin(); sit != t.end(); ++sit) {
            VectPTriangle tri1 = links_[*sit];
            for ( VectPTriangle::const_iterator tit = tri1.begin(); tit != tri1.end(); ++tit) {
                if ( mapt.count(*tit) == 0 ) {
                    mapt[*tit] = 1;
                } else {
                    mapt[*tit]++; // TODO const_cast are ugly ?
                }
            }
        }
        for ( std::map<Triangle *, unsigned>::iterator mit = mapt.begin(); mit != mapt.end(); ++mit) {
            if ( mit->second == 2) {
                tris.push_back(mit->first);
            }
        }
        return tris;
    }

    void Mesh::correct_local_orientation()
    {
        if ( !has_correct_orientation() ) {
            std::cerr << "Reorienting..." << std::endl << std::endl;
            std::stack<Triangle *>     tri_stack;
            std::map<Triangle *, bool> tri_reoriented;
            tri_stack.push(&*begin());
            tri_reoriented[&*begin()] = true;
            orient_adjacent_triangles(tri_stack, tri_reoriented);
        }
    }

    void Mesh::orient_adjacent_triangles(std::stack<Triangle *>& t_stack, std::map<Triangle *, bool>& tri_reoriented) 
    {
        while ( !t_stack.empty() ) {
            Triangle * t = t_stack.top();
            t_stack.pop();
            VectPTriangle t_adj = adjacent_triangles(*t);
            for ( VectPTriangle::iterator tit = t_adj.begin(); tit != t_adj.end(); ++tit) {
                if ( tri_reoriented.count(*tit) == 0 ) {
                    t_stack.push(*tit);
                    for ( Triangle::iterator vit = (*tit)->begin(); vit != (*tit)->end(); ++vit) {
                        if ( t->next(**vit) == (*tit)->next(**vit)) {
                            (*tit)->flip();
                            break;
                        }
                    }
                    tri_reoriented[*tit] = true;
                }
            }
        }
    }

    bool Mesh::has_correct_orientation() const 
    {
        /// Check the local orientation (that all the triangles are all oriented in the same way)

        const EdgeMap mape = compute_edge_map();

        for ( EdgeMap::const_iterator eit = mape.begin(); eit != mape.end(); ++eit) {
            if ( std::abs(eit->second) == 2 ) {
                std::cerr << "Local orientation problem..." << std::endl << std::endl;
                return false;
            }
        }

        return true;
    }

} // end namespace OpenMeeg
