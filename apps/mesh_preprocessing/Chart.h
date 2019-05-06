
#include "CGAL_typedefs.h"

#include "Utils.h"

#include "eig.h"

// struct to hold a vector of facets that make a chart
struct Chart
{
    uint32_t id;
    std::vector<std::shared_ptr<Facet>> facets;
    std::vector<bool> facets_are_inner_facets;
    std::vector<Vector> normals;
    std::vector<double> areas;
    bool active;
    Vector avg_normal;
    Vector init_normal;
    double area;
    double perimeter;

    bool has_border_edge;

    std::set<uint32_t> neighbour_charts;

    ErrorQuadric P_quad; // point error quad
    ErrorQuadric R_quad; // orientation error quad

    Chart() {}

    Chart(uint32_t _id, std::shared_ptr<Facet> f, const Vector normal, const double _area)
    {
        facets.push_back(f);
        facets_are_inner_facets.resize(facets.size(), false);

        normals.push_back(normal);
        areas.push_back(_area);
        active = true;
        area = _area;
        id = _id;
        avg_normal = normal;
        init_normal = normal;

        bool found_mesh_border = false;
        perimeter = get_face_perimeter(f, found_mesh_border);
        has_border_edge = found_mesh_border;

        P_quad = createPQuad(f);
        R_quad = ErrorQuadric(Utils::normalise(normal));
    }

    // create a combined quadric for 3 vertices of a face
    ErrorQuadric createPQuad(std::shared_ptr<Facet> f)
    {
        ErrorQuadric face_quad;
        Halfedge_facet_circulator he = f->facet_begin();
        CGAL_assertion(CGAL::circulator_size(he) >= 3);
        do
        {
            face_quad = face_quad + ErrorQuadric(he->vertex()->point());
        } while(++he != f->facet_begin());

        return face_quad;
    }

    // concatenate data from mc (merge chart) on to data from this chart
    // void merge_with(Chart &mc, const double cost_of_join){
    void merge_with(Chart& mc)
    {
        perimeter = get_chart_perimeter(*this, mc);

        facets.insert(facets.end(), mc.facets.begin(), mc.facets.end());
        facets_are_inner_facets.insert(facets_are_inner_facets.end(), mc.facets_are_inner_facets.begin(), mc.facets_are_inner_facets.end());
        normals.insert(normals.end(), mc.normals.begin(), mc.normals.end());
        areas.insert(areas.end(), mc.areas.begin(), mc.areas.end());

        P_quad = P_quad + mc.P_quad;
        R_quad = (R_quad * area) + (mc.R_quad * mc.area);

        // correct for size?
        R_quad = R_quad * (1.0 / (area + mc.area));

        area += mc.area;

        // recalculate average vector
        Vector v(0, 0, 0);
        for(uint32_t i = 0; i < normals.size(); ++i)
        {
            v += (normals[i] * areas[i]);
        }
        avg_normal = v / area;

        has_border_edge = has_border_edge | mc.has_border_edge;

        neighbour_charts.insert(mc.neighbour_charts.begin(), mc.neighbour_charts.end());

        // clear memory from old chart
        mc.facets.clear();
        mc.facets.shrink_to_fit();
    }

    // merges data from another chart
    // BUT does not calculate perimeter, or average vector
    // call "update_after_quick_merge" to update these variables
    // useful when merging a known quantity of faces
    void quick_merge_with(Chart& mc)
    {
        facets.insert(facets.end(), mc.facets.begin(), mc.facets.end());
        facets_are_inner_facets.insert(facets_are_inner_facets.end(), mc.facets_are_inner_facets.begin(), mc.facets_are_inner_facets.end());
        normals.insert(normals.end(), mc.normals.begin(), mc.normals.end());
        areas.insert(areas.end(), mc.areas.begin(), mc.areas.end());

        P_quad = P_quad + mc.P_quad;
        R_quad = (R_quad * area) + (mc.R_quad * mc.area);

        // correct for size?
        R_quad = R_quad * (1.0 / (area + mc.area));

        area += mc.area;

        has_border_edge = has_border_edge | mc.has_border_edge;

        neighbour_charts.insert(mc.neighbour_charts.begin(), mc.neighbour_charts.end());
    }
    // see function "quick_merge_with()" above
    void update_after_quick_merge()
    {
        // recalculate average vector
        Vector v(0, 0, 0);
        for(uint32_t i = 0; i < normals.size(); ++i)
        {
            v += (normals[i] * areas[i]);
        }
        avg_normal = v / area;

        facets_are_inner_facets.resize(facets.size(), false);
        recalculate_perimeter_from_scratch();
    }
    // also requires update after merge
    void add_facet(std::shared_ptr<Facet> f, const Vector normal, const double _area)
    {
        facets.push_back(f);
        normals.push_back(normal);
        areas.push_back(_area);

        area += _area;

        ErrorQuadric new_face_quad = createPQuad(f);
        P_quad = P_quad + new_face_quad;

        ErrorQuadric new_face_r_quad = ErrorQuadric(Utils::normalise(normal));
        R_quad = R_quad + new_face_r_quad;
    }

    // returns error of points in a joined chart , averga e distance from best fitting plane
    // returns plane normal for e_dir error term
    static double get_fit_error(Chart& c1, Chart& c2, Vector& fit_plane_normal)
    {
        ErrorQuadric fit_quad = c1.P_quad + c2.P_quad;

        Vector plane_normal;
        double scalar_offset;
        get_best_fit_plane(fit_quad, plane_normal, scalar_offset);

        double e_fit = (plane_normal * (fit_quad.A * plane_normal)) + (2 * (fit_quad.b * (scalar_offset * plane_normal))) + (fit_quad.c * scalar_offset * scalar_offset);

        e_fit = e_fit / ((c1.facets.size() * 3) + (c2.facets.size() * 3));
        // divide by number of points in combined chart
        // TODO does it need to be calculated exactly or just faces * 3 ?

        // check against sign of average normal to get sign of plane_normal
        Vector avg_normal = Utils::normalise((c1.avg_normal * c1.area) + (c2.avg_normal * c2.area));
        double angle_between_vectors = acos(avg_normal * plane_normal);

        if(angle_between_vectors > (0.5 * M_PI))
        {
            fit_plane_normal = -plane_normal;
        }
        else
        {
            fit_plane_normal = plane_normal;
        }

        return e_fit;
    }

    // retrieves eigenvector corresponding to lowest eigenvalue
    // which is taken as the normal of the best fitting plane, given an error quadric corresponding to that plane
    static void get_best_fit_plane(ErrorQuadric eq, Vector& plane_normal, double& scalar_offset)
    {
        // get covariance matrix (Z matrix) from error quadric
        double Z[3][3];
        eq.get_covariance_matrix(Z);

        // do eigenvalue decomposition
        double eigenvectors[3][3] = {0};
        double eigenvalues[3] = {0};
        eigen::eig::eigen_decomposition(Z, eigenvectors, eigenvalues);

        // find min eigenvalue
        double min_ev = DBL_MAX;
        int min_loc;
        for(int i = 0; i < 3; ++i)
        {
            if(eigenvalues[i] < min_ev)
            {
                min_ev = eigenvalues[i];
                min_loc = i;
            }
        }

        plane_normal = Vector(eigenvectors[0][min_loc], eigenvectors[1][min_loc], eigenvectors[2][min_loc]);

        // d is described specified as:  d =  (-nT*b)   /     c
        scalar_offset = (-plane_normal * eq.b) / eq.c;
    }

    // as defined in Garland et al 2001
    static double get_compactness_of_merged_charts(Chart& c1, Chart& c2)
    {
        double irreg1 = get_irregularity(c1.perimeter, c1.area);
        double irreg2 = get_irregularity(c2.perimeter, c2.area);
        double irreg_new = get_irregularity(get_chart_perimeter(c1, c2), c1.area + c2.area);

        // as described in the paper:
        // double shape_penalty = ( irreg_new - std::max(irreg1, irreg2) ) / irreg_new;

        // possible edit

        double shape_penalty = irreg_new - (irreg1 + irreg2);

        // correction for area
        shape_penalty /= (c1.area + c2.area);

        return shape_penalty;
    }

    static double get_irregularity(double perimeter, double area) { return (perimeter * perimeter) / (4 * M_PI * area); }

    // adds edge lengths of a face
    //
    // also looks out for border edges
    static double get_face_perimeter(std::shared_ptr<Facet> f, bool& found_mesh_border)
    {
        Halfedge_facet_circulator he = f->facet_begin();
        CGAL_assertion(CGAL::circulator_size(he) >= 3);
        double accum_perimeter = 0;

        // for 3 edges (adjacent faces)
        do
        {
            // check for border
            if((he->is_border()) || (he->opposite()->is_border()))
            {
                found_mesh_border = true;
            }

            accum_perimeter += edge_length(he);
        } while(++he != f->facet_begin());

        return accum_perimeter;
    }

    // calculate perimeter of chart
    // associate a perimeter with each cluster
    // then calculate by adding perimeters and subtractng double shared edges
    // TODO update to make use of list of which faces are inner faces
    static double get_chart_perimeter(Chart& c1, Chart& c2)
    {
        double perimeter = c1.perimeter + c2.perimeter;

        // for each face in c1
        for(uint32_t f1 = 0; f1 < c1.facets.size(); ++f1)
        {
            if(!(c1.facets_are_inner_facets[f1]))
            {
                // for each edge
                Halfedge_facet_circulator he = c1.facets[f1]->facet_begin();
                CGAL_assertion(CGAL::circulator_size(he) >= 3);

                do
                {
                    // check if opposite appears in c2

                    // check this edge is not a border
                    if(!(he->is_border()) && !(he->opposite()->is_border()))
                    {
                        int32_t adj_face_id = he->opposite()->facet()->id();

                        // for (auto& c2_face : c2.facets){
                        for(uint32_t f2 = 0; f2 < c2.facets.size(); ++f2)
                        {
                            if(!(c2.facets_are_inner_facets[f2]))
                            {
                                if(c2.facets[f2]->id() == adj_face_id)
                                {
                                    perimeter -= (2 * edge_length(he));
                                    break;
                                }
                            }
                        }
                    }
                    // else {
                    //   // std::cout << "border edge\n";
                    // }

                } while(++he != c1.facets[f1]->facet_begin());
            }
        }

        return perimeter;
    }

    // recalculates the perimeter of this chart
    // necessary when charts are built from initial grid groups - charts are not built sequentially
    void recalculate_perimeter_from_scratch()
    {
        double perimeter_accum = 0.0;

        // for each face
        for(uint32_t i = 0; i < facets.size(); ++i)
        {
            // for each edge
            Halfedge_facet_circulator fc = facets[i]->facet_begin();
            bool inner_face = true; // set face as an inner face, unless any of its neighbours are not in this chart

            do
            {
                // sum the length of all edges that are borders, or border with another chart

                // if this edge is shared with another face
                if(!fc->is_border() && !(fc->opposite()->is_border()))
                {
                    // search for face in this chart
                    uint32_t nbr_id = fc->opposite()->facet()->id();
                    bool found_nbr = false;
                    for(uint32_t j = 0; j < facets.size(); ++j)
                    {
                        if(nbr_id == facets[j]->id())
                        {
                            found_nbr = true;
                            break;
                        }
                    }
                    if(!found_nbr) // if neighbour is not in this chart, add edge to perimeter accumulation
                    {
                        perimeter_accum += Chart::edge_length(fc);
                        inner_face = false;
                    }
                }
                else
                {
                    // std::cout << "border edge" << std::endl;
                    perimeter_accum += Chart::edge_length(fc);
                }
            } while(++fc != facets[i]->facet_begin());

            facets_are_inner_facets[i] = inner_face;
        }

        perimeter = perimeter_accum;
    }

    void create_neighbour_set(std::map<uint32_t, uint32_t>& chart_id_map)
    {
        // for each face in chart
        for(uint32_t i = 0; i < this->facets.size(); ++i)
        {
            // if face is not an inner face
            if(!(this->facets_are_inner_facets[i]))
            {
                // get id of surrounding faces
                Halfedge_facet_circulator fc = this->facets[i]->facet_begin();
                bool found_nbr_chart = false;
                do
                {
                    if(!fc->is_border() && !(fc->opposite()->is_border()))
                    {
                        uint32_t nbr_face_id = fc->opposite()->facet()->id();
                        // get chart id of neighbour face
                        uint32_t nbr_chart_id = chart_id_map[nbr_face_id];

                        // if chart id of nbr is not this chart, add to set of neighbours
                        if(nbr_chart_id != this->id)
                        {
                            this->neighbour_charts.insert(nbr_chart_id);
                            found_nbr_chart = true;
                        }
                    }
                } while(++fc != this->facets[i]->facet_begin());

                // if a neighbour was found, this is not an inner chart, and vice versa
                this->facets_are_inner_facets[i] = !found_nbr_chart;
            }

        } // for faces

        // std::cout << "Chart " << this->id << " has " << this->neighbour_charts.size() << " neighbours\n";
    }

    static double get_direction_error(Chart& c1, Chart& c2, Vector& plane_normal)
    {
        ErrorQuadric combinedQuad = (c1.R_quad * c1.area) + (c2.R_quad * c2.area);

        double e_direction = evaluate_direction_quadric(combinedQuad, plane_normal);

        return e_direction / (c1.area + c2.area);
    }

    static double evaluate_direction_quadric(ErrorQuadric& eq, Vector& plane_normal) { return (plane_normal * (eq.A * plane_normal)) + (2 * (eq.b * plane_normal)) + eq.c; }

    static double edge_length(Halfedge_facet_circulator he)
    {
        const Point& p = he->prev()->vertex()->point();
        const Point& q = he->vertex()->point();

        return CGAL::sqrt(CGAL::squared_distance(p, q));
    }

    static bool sort_by_id(Chart c1, Chart c2) { return (c1.id < c2.id); }
};