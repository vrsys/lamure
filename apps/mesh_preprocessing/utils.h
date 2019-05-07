#include "structures.h"

#ifndef UTILSH
#define UTILSH

namespace utils
{
static void print_help_message()
{
    std::cout << "Please provide an obj filename using -f <filename.obj>" << std::endl;
    std::cout << "Optional: -of specifies outfile name" << std::endl;
    std::cout << "Optional: -co specifies cost threshold (=infinite)" << std::endl;
    std::cout << "Optional: -ct specifies threshold for grid chart splitting by normal variance (=0.001)" << std::endl;
    std::cout << "Optional: -debug writes charts to obj file, as colours that override texture coordinates (=false)" << std::endl;
    std::cout << "Optional: -tkd num triangles per kdtree node (default: 32000)" << std::endl;
    std::cout << "Optional: -tbvh num triangles per kdtree node (default: 8192)" << std::endl;
    std::cout << "Optional: -single-max: specifies largest possible single output texture (=4096)" << std::endl;
    std::cout << "Optional: -multi-max: specifies largest possible output texture (=8192)" << std::endl;
}

static void initialize_glut_window(int argc, char** argv, cmd_options& opt)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(opt.single_tex_limit, opt.single_tex_limit);
    glutInitWindowPosition(64, 64);
    glutCreateWindow(argv[0]);
    glutSetWindowTitle(STRING_APP_NAME.c_str());
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glewExperimental = GL_TRUE;
    glewInit();
    glutHideWindow();
}

static char* getCmdOption(char** begin, char** end, const std::string& option)
{
    char** itr = std::find(begin, end, option);
    if(itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

static bool cmdOptionExists(char** begin, char** end, const std::string& option) { return std::find(begin, end, option) != end; }

// parses a face string like "f  2//1  8//1  4//1 " into 3 given arrays
static void parse_face_string(std::string face_string, int (&index)[3], int (&coord)[3], int (&normal)[3])
{
    // split by space into faces
    std::vector<std::string> faces;
    boost::algorithm::trim(face_string);
    boost::algorithm::split(faces, face_string, boost::algorithm::is_any_of(" "), boost::algorithm::token_compress_on);

    for(int i = 0; i < 3; ++i)
    {
        // split by / for indices
        std::vector<std::string> inds;
        boost::algorithm::split(inds, faces[i], [](char c) { return c == '/'; }, boost::algorithm::token_compress_off);

        for(int j = 0; j < (int)inds.size(); ++j)
        {
            int idx = 0;
            // parse value from string
            if(inds[j] != "")
            {
                idx = stoi(inds[j]);
            }
            if(j == 0)
            {
                index[i] = idx;
            }
            else if(j == 1)
            {
                coord[i] = idx;
            }
            else if(j == 2)
            {
                normal[i] = idx;
            }
        }
    }
}

static std::shared_ptr<texture_t> load_image(const std::string& filepath)
{
    std::vector<unsigned char> img;
    unsigned int width = 0;
    unsigned int height = 0;
    int tex_error = lodepng::decode(img, width, height, filepath);
    if(tex_error)
    {
        std::cout << "ERROR: unable to load image file (not a .png?) " << filepath << std::endl;
        exit(1);
    }
    else
    {
        std::cout << "Loaded texture from " << filepath << std::endl;
    }

    auto texture = std::make_shared<texture_t>(width, height, GL_LINEAR);
    texture->set_pixels(&img[0]);

    return texture;
}

static void save_image(std::string& filename, std::vector<uint8_t>& image, int width, int height)
{
    unsigned int tex_error = lodepng::encode(filename, image, width, height);

    if(tex_error)
    {
        std::cerr << "ERROR: unable to save image file " << filename << std::endl;
        std::cerr << tex_error << ": " << lodepng_error_text(tex_error) << std::endl;
    }
    std::cout << "Saved image to " << filename << std::endl;
}

static void save_framebuffer_to_image(std::string filename, std::shared_ptr<frame_buffer_t> frame_buffer)
{
    std::vector<uint8_t> pixels;
    frame_buffer->get_pixels(0, pixels);

    int tex_error = lodepng::encode(filename, pixels, frame_buffer->get_width(), frame_buffer->get_height());
    if(tex_error)
    {
        std::cout << "ERROR: unable to save image file " << filename << std::endl;
        exit(1);
    }
    std::cout << "Framebuffer written to image " << filename << std::endl;
}

#ifdef ADHOC_PARSER
// load obj function from vt_obj_loader/utils.h
static BoundingBoxLimits
load_obj(const std::string& filename, std::vector<double>& v, std::vector<int>& vindices, std::vector<double>& t, std::vector<int>& tindices, std::vector<std::string>& materials)
{
    scm::math::vec3f min_pos(std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
    scm::math::vec3f max_pos(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest());

    FILE* file = fopen(filename.c_str(), "r");

    if(0 != file)
    {
        std::string current_material = "";

        while(true)
        {
            char line[128];
            int32_t l = fscanf(file, "%s", line);

            if(l == EOF)
                break;
            if(strcmp(line, "usemtl") == 0)
            {
                char name[128];
                fscanf(file, "%s\n", name);
                current_material = std::string(name);
                current_material.erase(std::remove(current_material.begin(), current_material.end(), '\n'), current_material.end());
                current_material.erase(std::remove(current_material.begin(), current_material.end(), '\r'), current_material.end());
                boost::trim(current_material);
                std::cout << "obj switch material: " << current_material << std::endl;
            }
            else if(strcmp(line, "v") == 0)
            {
                double vx, vy, vz;
                fscanf(file, "%lf %lf %lf\n", &vx, &vy, &vz);

                // scaling for zebra
                // vx *= 0.01;
                // vy *= 0.01;
                // vz *= 0.01;

                v.insert(v.end(), {vx, vy, vz});

                // compare to find bounding box limits
                if(vx > max_pos.x)
                {
                    max_pos.x = vx;
                }
                else if(vx < min_pos.x)
                {
                    min_pos.x = vx;
                }
                if(vy > max_pos.y)
                {
                    max_pos.y = vy;
                }
                else if(vy < min_pos.y)
                {
                    min_pos.y = vy;
                }
                if(vz > max_pos.z)
                {
                    max_pos.z = vz;
                }
                else if(vz < min_pos.z)
                {
                    min_pos.z = vz;
                }
            }
            // else if (strcmp(line, "vn") == 0) {
            //   float nx, ny, nz;
            //   fscanf(file, "%f %f %f\n", &nx, &ny, &nz);
            //   n.insert(n.end(), {nx,ny, nz});
            // }
            else if(strcmp(line, "vt") == 0)
            {
                double tx, ty;
                fscanf(file, "%lf %lf\n", &tx, &ty);
                t.insert(t.end(), {tx, ty});
            }
            else if(strcmp(line, "f") == 0)
            {
                fgets(line, 128, file);
                std::string face_string = line;
                int index[3];
                int coord[3];
                int normal[3];

                parse_face_string(face_string, index, coord, normal);

                // here all indices are decremented by 1 to fit 0 indexing schemes
                vindices.insert(vindices.end(), {index[0] - 1, index[1] - 1, index[2] - 1});
                tindices.insert(tindices.end(), {coord[0] - 1, coord[1] - 1, coord[2] - 1});
                // nindices.insert(nindices.end(), {normal[0]-1, normal[1]-1, normal[2]-1});

                materials.push_back(current_material);
            }
        }

        fclose(file);

        std::cout << "positions: " << v.size() / 3 << std::endl;
        // std::cout << "normals: " << n.size()/3 << std::endl;
        std::cout << "coords: " << t.size() / 2 << std::endl;
        std::cout << "faces: " << vindices.size() / 3 << std::endl;
    }

    BoundingBoxLimits bbox;
    bbox.min = min_pos;
    bbox.max = max_pos;

    return bbox;
}

static bool load_mtl(const std::string& mtl_filename, std::map<std::string, std::pair<std::string, int>>& material_map)
{
    // parse .mtl file
    std::cout << "loading .mtl file ..." << std::endl;
    std::ifstream mtl_file(mtl_filename.c_str());
    if(!mtl_file.is_open())
    {
        std::cout << "could not open .mtl file" << std::endl;
        return false;
    }

    std::string current_material = "";
    int material_index = 0;

    std::string line;
    while(std::getline(mtl_file, line))
    {
        boost::trim(line);
        if(line.length() >= 2)
        {
            if(line[0] == '#')
            {
                continue;
            }
            if(line.substr(0, 6) == "newmtl")
            {
                current_material = line.substr(7);
                boost::trim(current_material);
                current_material.erase(std::remove(current_material.begin(), current_material.end(), '\n'), current_material.end());
                current_material.erase(std::remove(current_material.begin(), current_material.end(), '\r'), current_material.end());
                std::cout << "found: " << current_material << std::endl;
                material_map[current_material] = std::make_pair("", -1);
            }
            else if(line.substr(0, 6) == "map_Kd")
            {
                std::string current_texture = line.substr(7);
                current_texture.erase(std::remove(current_texture.begin(), current_texture.end(), '\n'), current_texture.end());
                current_texture.erase(std::remove(current_texture.begin(), current_texture.end(), '\r'), current_texture.end());
                boost::trim(current_texture);

                std::cout << current_material << " -> " << current_texture << ", " << material_index << std::endl;
                material_map[current_material] = std::make_pair(current_texture, material_index);
                ++material_index;
            }
        }
    }

    mtl_file.close();

    return true;
}
#endif

// writes the texture id of each face of a polyhedron into a text file
static void write_tex_id_file(Polyhedron& P, std::string tex_file_name)
{
    // write chart file
    std::ofstream ocfs(tex_file_name);
    for(Facet_iterator fi = P.facets_begin(); fi != P.facets_end(); ++fi)
    {
        ocfs << fi->tex_id << " ";
    }
    ocfs.close();
    std::cout << "Texture id per face file written to:  " << tex_file_name << std::endl;
}

// reads dimensions of png from header
static scm::math::vec2i get_png_dimensions(std::string filepath)
{
    std::ifstream in(filepath);
    uint32_t width, height;

    in.seekg(16);
    in.read((char*)&width, 4);
    in.read((char*)&height, 4);

    width = be32toh(width);
    height = be32toh(height);

    return scm::math::vec2i(width, height);
}

#ifdef VCG_PARSER
static void load_obj(const std::string& file, std::vector<indexed_triangle_t>& triangles, std::map<uint32_t, texture_info>& texture_info_map)
{
    triangles.clear();
    texture_info_map.clear();

    CMesh m;

    {
        using namespace vcg::tri;
        using namespace vcg::tri::io;

        ImporterOBJ<CMesh>::Info oi;
        bool mask_load_success = ImporterOBJ<CMesh>::LoadMask(file.c_str(), oi);
        const int load_mask = oi.mask;
        int load_error = ImporterOBJ<CMesh>::Open(m, file.c_str(), oi);

        const int expected_mask = Mask::IOM_VERTCOORD | Mask::IOM_VERTTEXCOORD | Mask::IOM_WEDGTEXCOORD | Mask::IOM_VERTNORMAL;

        if(!(load_mask & expected_mask))
        {
            throw std::runtime_error("Mesh does not contain necessary components, mask of missing components: " + std::to_string(load_mask ^ expected_mask));
        }

        if(!mask_load_success || load_error != ImporterOBJ<CMesh>::OBJError::E_NOERROR)
        {
            if(ImporterOBJ<CMesh>::ErrorCritical(load_error))
            {
                throw std::runtime_error("Failed to load the model: " + std::string(ImporterOBJ<CMesh>::ErrorMsg(load_error)));
            }
            else
            {
                std::cerr << std::string(ImporterOBJ<CMesh>::ErrorMsg(load_error)) << std::endl;
            }
        }

        UpdateTopology<CMesh>::FaceFace(m);
        UpdateTopology<CMesh>::VertexFace(m);
    }

    triangles.resize((size_t)m.FN());

    for(size_t i = 0; i < m.FN(); i++)
    {
        auto v_0 = m.face[i].V(0);
        auto v_1 = m.face[i].V(1);
        auto v_2 = m.face[i].V(2);

        memcpy(&triangles[i].v0_.pos_.data_array[0], &v_0->P()[0], sizeof(float) * 3);
        memcpy(&triangles[i].v1_.pos_.data_array[0], &v_1->P()[0], sizeof(float) * 3);
        memcpy(&triangles[i].v2_.pos_.data_array[0], &v_2->P()[0], sizeof(float) * 3);

        memcpy(&triangles[i].v0_.nml_.data_array[0], &v_0->N()[0], sizeof(float) * 3);
        memcpy(&triangles[i].v1_.nml_.data_array[0], &v_1->N()[0], sizeof(float) * 3);
        memcpy(&triangles[i].v2_.nml_.data_array[0], &v_2->N()[0], sizeof(float) * 3);

        triangles[i].v0_.tex_.x = m.face[i].WT(0).U();
        triangles[i].v0_.tex_.y = m.face[i].WT(0).V();

        triangles[i].v1_.tex_.x = m.face[i].WT(1).U();
        triangles[i].v1_.tex_.y = m.face[i].WT(1).V();

        triangles[i].v2_.tex_.x = m.face[i].WT(2).U();
        triangles[i].v2_.tex_.y = m.face[i].WT(2).V();

        triangles[i].tri_id_ = (uint32_t)i;
        triangles[i].tex_idx_ = m.face[i].WT(0).n();
    }

    for(uint32_t i = 0; i < m.textures.size(); i++)
    {
        std::string texture_filename = m.textures[i];

        if(texture_filename.size() > 3)
        {
            texture_filename = texture_filename.substr(0, texture_filename.size() - 3) + "png";
        }

        uint32_t material_id = i;
        std::cout << "Material " << m.textures[i] << " : " << texture_filename << " : " << material_id << std::endl;

        if(boost::filesystem::exists(texture_filename) && boost::algorithm::ends_with(texture_filename, ".png"))
        {
            texture_info_map[material_id] = {texture_filename, get_png_dimensions(texture_filename)};
        }
        else if(texture_filename == "")
        {
            // ok
        }
        else
        {
            std::cout << "ERROR: texture " << texture_filename << " was not found or is not a .png" << std::endl;
            exit(1);
        }
    }
}
#endif

#ifdef ADHOC_PARSER
// load an .obj file and return all vertices, normals and coords interleaved
static void load_obj(const std::string& _file, std::vector<lamure::mesh::triangle_t>& triangles, std::vector<std::string>& materials)
{
    triangles.clear();

    std::vector<float> v;
    std::vector<uint32_t> vindices;
    std::vector<float> n;
    std::vector<uint32_t> nindices;
    std::vector<float> t;
    std::vector<uint32_t> tindices;

    uint32_t num_tris = 0;

    FILE* file = fopen(_file.c_str(), "r");

    std::string current_material = "";

    if(0 != file)
    {
        while(true)
        {
            char line[128];
            int32_t l = fscanf(file, "%s", line);

            if(l == EOF)
                break;
            if(strcmp(line, "usemtl") == 0)
            {
                char name[128];
                fscanf(file, "%s\n", name);
                current_material = std::string(name);
                current_material.erase(std::remove(current_material.begin(), current_material.end(), '\n'), current_material.end());
                current_material.erase(std::remove(current_material.begin(), current_material.end(), '\r'), current_material.end());
                boost::trim(current_material);
                std::cout << "obj switch material: " << current_material << std::endl;
            }
            else if(strcmp(line, "v") == 0)
            {
                float vx, vy, vz;
                fscanf(file, "%f %f %f\n", &vx, &vy, &vz);
                v.insert(v.end(), {vx, vy, vz});
            }
            else if(strcmp(line, "vn") == 0)
            {
                float nx, ny, nz;
                fscanf(file, "%f %f %f\n", &nx, &ny, &nz);
                n.insert(n.end(), {nx, ny, nz});
            }
            else if(strcmp(line, "vt") == 0)
            {
                float tx, ty;
                fscanf(file, "%f %f\n", &tx, &ty);
                t.insert(t.end(), {tx, ty});
            }
            else if(strcmp(line, "f") == 0)
            {
                std::string vertex1, vertex2, vertex3;
                uint32_t index[3];
                uint32_t coord[3];
                uint32_t normal[3];
                fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &index[0], &coord[0], &normal[0], &index[1], &coord[1], &normal[1], &index[2], &coord[2], &normal[2]);

                vindices.insert(vindices.end(), {index[0], index[1], index[2]});
                tindices.insert(tindices.end(), {coord[0], coord[1], coord[2]});
                nindices.insert(nindices.end(), {normal[0], normal[1], normal[2]});

                materials.push_back(current_material);
            }
        }

        fclose(file);

        std::cout << "positions: " << vindices.size() << std::endl;
        std::cout << "normals: " << nindices.size() << std::endl;
        std::cout << "coords: " << tindices.size() << std::endl;

        triangles.resize(nindices.size() / 3);

        for(uint32_t i = 0; i < nindices.size() / 3; i++)
        {
            lamure::mesh::triangle_t tri;
            for(uint32_t j = 0; j < 3; ++j)
            {
                scm::math::vec3f position(v[3 * (vindices[3 * i + j] - 1)], v[3 * (vindices[3 * i + j] - 1) + 1], v[3 * (vindices[3 * i + j] - 1) + 2]);

                scm::math::vec3f normal(n[3 * (nindices[3 * i + j] - 1)], n[3 * (nindices[3 * i + j] - 1) + 1], n[3 * (nindices[3 * i + j] - 1) + 2]);

                scm::math::vec2f coord(t[2 * (tindices[3 * i + j] - 1)], t[2 * (tindices[3 * i + j] - 1) + 1]);

                switch(j)
                {
                case 0:
                    tri.v0_.pos_ = position;
                    tri.v0_.nml_ = normal;
                    tri.v0_.tex_ = coord;
                    break;

                case 1:
                    tri.v1_.pos_ = position;
                    tri.v1_.nml_ = normal;
                    tri.v1_.tex_ = coord;
                    break;

                case 2:
                    tri.v2_.pos_ = position;
                    tri.v2_.nml_ = normal;
                    tri.v2_.tex_ = coord;
                    break;

                default:
                    break;
                }
            }
            triangles[i] = tri;
        }
    }
    else
    {
        std::cout << "failed to open file: " << _file << std::endl;
        exit(1);
    }
}
#endif

static void extract_cmd_options(int argc, char** argv, std::string& obj_filename, cmd_options& opt)
{
    opt.out_filename = obj_filename.substr(0, obj_filename.size() - 4) + "_charts.obj";
    if(cmdOptionExists(argv, argv + argc, "-of"))
    {
        opt.out_filename = getCmdOption(argv, argv + argc, "-of");
    }
    opt.want_raw_file = false;
    if(cmdOptionExists(argv, argv + argc, "-raw"))
    {
        opt.want_raw_file = true;
    }
    opt.cost_threshold = std::numeric_limits<double>::max();
    if(cmdOptionExists(argv, argv + argc, "-co"))
    {
        opt.cost_threshold = atof(getCmdOption(argv, argv + argc, "-co"));
    }
    opt.chart_threshold = std::numeric_limits<uint32_t>::max();
    if(cmdOptionExists(argv, argv + argc, "-ch"))
    {
        opt.chart_threshold = atoi(getCmdOption(argv, argv + argc, "-ch"));
    }
    opt.e_fit_cf = 1.0;
    if(cmdOptionExists(argv, argv + argc, "-ef"))
    {
        opt.e_fit_cf = atof(getCmdOption(argv, argv + argc, "-ef"));
    }
    opt.e_ori_cf = 1.0;
    if(cmdOptionExists(argv, argv + argc, "-eo"))
    {
        opt.e_ori_cf = atof(getCmdOption(argv, argv + argc, "-eo"));
    }
    opt.e_shape_cf = 1.0;
    if(cmdOptionExists(argv, argv + argc, "-es"))
    {
        opt.e_shape_cf = atof(getCmdOption(argv, argv + argc, "-es"));
    }
    opt.cst = 0.001;
    if(cmdOptionExists(argv, argv + argc, "-ct"))
    {
        opt.cst = atof(getCmdOption(argv, argv + argc, "-ct"));
    }

    if(cmdOptionExists(argv, argv + argc, "-debug"))
    {
        opt.write_charts_as_textures = true;
    }
    opt.num_tris_per_node_kdtree = 1024 * 32;
    if(cmdOptionExists(argv, argv + argc, "-tkd"))
    {
        opt.num_tris_per_node_kdtree = atoi(getCmdOption(argv, argv + argc, "-tkd"));
    }
    opt.num_tris_per_node_bvh = 8 * 1024;
    if(cmdOptionExists(argv, argv + argc, "-tbvh"))
    {
        opt.num_tris_per_node_bvh = atoi(getCmdOption(argv, argv + argc, "-tbvh"));
    }

    opt.single_tex_limit = 4096;
    if(cmdOptionExists(argv, argv + argc, "-single-max"))
    {
        opt.single_tex_limit = atoi(getCmdOption(argv, argv + argc, "-single-max"));
        std::cout << "Single output texture limited to " << opt.single_tex_limit << std::endl;
    }

    opt.multi_tex_limit = 8192;
    if(cmdOptionExists(argv, argv + argc, "-multi-max"))
    {
        opt.multi_tex_limit = atoi(getCmdOption(argv, argv + argc, "-multi-max"));
        std::cout << "Multi output texture limited to " << opt.multi_tex_limit << std::endl;
    }
}

static void initialize_nodes(app_state& state)
{
    uint32_t first_leaf_id = state.kdtree->get_first_node_id_of_depth(state.kdtree->get_depth());
    uint32_t num_leaf_ids = state.kdtree->get_length_of_depth(state.kdtree->get_depth());

    for(uint32_t i = 0; i < num_leaf_ids; ++i)
    {
        state.node_ids.push_back(i + first_leaf_id);
    }
}

static void reorder_triangles(app_state& state)
{
    // here, we make sure that triangles is in the same ordering as the leaf level triangles

    uint32_t first_leaf_id = state.bvh->get_first_node_id_of_depth(state.bvh->get_depth());
    uint32_t num_leaf_ids = state.bvh->get_length_of_depth(state.bvh->get_depth());

    state.triangles.clear();
    for(uint32_t node_id = first_leaf_id; node_id < first_leaf_id + num_leaf_ids; ++node_id)
    {
        state.triangles.insert(state.triangles.end(), state.bvh->get_triangles(node_id).begin(), state.bvh->get_triangles(node_id).end());
    }

    std::cout << "Reordered " << state.triangles.size() << " triangles" << std::endl;
}

static void prepare_charts(app_state& state)
{
    for(uint32_t tri_id = 0; tri_id < state.triangles.size(); ++tri_id)
    {
        const auto& tri = state.triangles[tri_id];
        uint32_t area_id = tri.area_id;
        uint32_t chart_id = tri.chart_id;

        if(chart_id != -1 && tri.get_area() > 0.f)
        {
            state.chart_map[area_id][chart_id].id_ = chart_id;
            state.chart_map[area_id][chart_id].original_triangle_ids_.insert(tri_id);
        }
    }

    for(uint32_t area_id = 0; area_id < state.num_areas; ++area_id)
    {
        // init charts
        for(auto& it : state.chart_map[area_id])
        {
            it.second.rect_ = rectangle{scm::math::vec2f(std::numeric_limits<float>::max()), scm::math::vec2f(std::numeric_limits<float>::lowest()), it.first, false};

            it.second.box_ = lamure::bounding_box(scm::math::vec3d(std::numeric_limits<float>::max()), scm::math::vec3d(std::numeric_limits<float>::lowest()));
        }
    }
}

static void expand_charts(app_state& state)
{
    // grow chart boxes by all triangles in all levels of bvh
    for(uint32_t node_id = 0; node_id < state.bvh->get_num_nodes(); node_id++)
    {
        const std::vector<lamure::mesh::Triangle_Chartid>& tris = state.bvh->get_triangles(node_id);

        for(const auto& tri : tris)
        {
            state.chart_map[tri.area_id][tri.chart_id].box_.expand(scm::math::vec3d(tri.v0_.pos_));
            state.chart_map[tri.area_id][tri.chart_id].box_.expand(scm::math::vec3d(tri.v1_.pos_));
            state.chart_map[tri.area_id][tri.chart_id].box_.expand(scm::math::vec3d(tri.v2_.pos_));
        }
    }
}

static void chartify_parallel(app_state& state, cmd_options& opt)
{
    int prev_percent = -1;

    CLUSTER_SETTINGS cluster_settings(opt.e_fit_cf, opt.e_ori_cf, opt.e_shape_cf, opt.cst);

    auto lambda_chartify = [&](uint64_t i, uint32_t id) -> void {
        int percent = (int)(((float)i / (float)state.node_ids.size()) * 100.f);
        if(percent != prev_percent)
        {
            prev_percent = percent;
            std::cout << "Chartification: " << percent << " %" << std::endl;
        }

        // build polyhedron from node.begin to node.end in accordance with indices

        uint32_t node_id = state.node_ids[i];
        const auto& node = state.kdtree->get_nodes()[node_id];
        const auto& indices = state.kdtree->get_indices();

        BoundingBoxLimits limits;
        limits.min = scm::math::vec3f(std::numeric_limits<float>::max());
        limits.max = scm::math::vec3f(std::numeric_limits<float>::lowest());

        std::vector<indexed_triangle_t> node_triangles;
        for(uint32_t idx = node.begin_; idx < node.end_; ++idx)
        {
            const auto& tri = state.all_indexed_triangles[indices[idx]];
            node_triangles.push_back(tri);

            limits.min.x = std::min(limits.min.x, tri.v0_.pos_.x);
            limits.min.y = std::min(limits.min.y, tri.v0_.pos_.y);
            limits.min.z = std::min(limits.min.z, tri.v0_.pos_.z);

            limits.min.x = std::min(limits.min.x, tri.v1_.pos_.x);
            limits.min.y = std::min(limits.min.y, tri.v1_.pos_.y);
            limits.min.z = std::min(limits.min.z, tri.v1_.pos_.z);

            limits.min.x = std::min(limits.min.x, tri.v2_.pos_.x);
            limits.min.y = std::min(limits.min.y, tri.v2_.pos_.y);
            limits.min.z = std::min(limits.min.z, tri.v2_.pos_.z);

            limits.max.x = std::max(limits.max.x, tri.v0_.pos_.x);
            limits.max.y = std::max(limits.max.y, tri.v0_.pos_.y);
            limits.max.z = std::max(limits.max.z, tri.v0_.pos_.z);

            limits.max.x = std::max(limits.max.x, tri.v1_.pos_.x);
            limits.max.y = std::max(limits.max.y, tri.v1_.pos_.y);
            limits.max.z = std::max(limits.max.z, tri.v1_.pos_.z);

            limits.max.x = std::max(limits.max.x, tri.v2_.pos_.x);
            limits.max.y = std::max(limits.max.y, tri.v2_.pos_.y);
            limits.max.z = std::max(limits.max.z, tri.v2_.pos_.z);
        }

        Polyhedron polyMesh;
        polyhedron_builder<HalfedgeDS> builder(node_triangles);
        polyMesh.delegate(builder);

        if(!CGAL::is_triangle_mesh(polyMesh))
        {
            std::cerr << "ERROR: Input geometry is not valid / not triangulated." << std::endl;
            return;
        }

        // key: face_id, value: chart_id
        std::map<uint32_t, uint32_t> chart_id_map;
        uint32_t active_charts = ParallelClusterCreator::create_charts(chart_id_map, polyMesh, opt.cost_threshold, opt.chart_threshold, cluster_settings);

        state.per_node_chart_id_map[node_id] = chart_id_map;
        state.per_node_polyhedron[node_id] = polyMesh;
    };

    uint32_t num_threads = std::min((size_t)24, state.node_ids.size());
    lamure::mesh::parallel_for(num_threads, state.node_ids.size(), lambda_chartify);
}

static void convert_to_triangle_soup(app_state& state)
{
    state.num_areas = 0;
    for(auto& per_node_chart_id_map_it : state.per_node_chart_id_map)
    {
        uint32_t node_id = per_node_chart_id_map_it.first;
        auto polyMesh = state.per_node_polyhedron[node_id];

        // create index
        typedef CGAL::Inverse_index<Polyhedron::Vertex_const_iterator> Index;
        Index index(polyMesh.vertices_begin(), polyMesh.vertices_end());

        // extract triangle soup
        for(Polyhedron::Facet_const_iterator fi = polyMesh.facets_begin(); fi != polyMesh.facets_end(); ++fi)
        {
            Polyhedron::Halfedge_around_facet_const_circulator hc = fi->facet_begin();
            Polyhedron::Halfedge_around_facet_const_circulator hc_end = hc;

            if(circulator_size(hc) != 3)
            {
                std::cout << "ERROR: mesh corrupt!" << std::endl;
                exit(1);
            }

            lamure::mesh::Triangle_Chartid tri;

            Polyhedron::Vertex_const_iterator it = polyMesh.vertices_begin();
            std::advance(it, index[Polyhedron::Vertex_const_iterator(hc->vertex())]);
            tri.v0_.pos_ = scm::math::vec3f(it->point().x(), it->point().y(), it->point().z());
            tri.v0_.tex_ = scm::math::vec2f(fi->t_coords[0].x(), fi->t_coords[0].y());
            tri.v0_.nml_ = scm::math::vec3f(it->point().normal.x(), it->point().normal.y(), it->point().normal.z());
            ++hc;

            it = polyMesh.vertices_begin();
            std::advance(it, index[Polyhedron::Vertex_const_iterator(hc->vertex())]);
            tri.v1_.pos_ = scm::math::vec3f(it->point().x(), it->point().y(), it->point().z());
            tri.v1_.tex_ = scm::math::vec2f(fi->t_coords[1].x(), fi->t_coords[1].y());
            tri.v1_.nml_ = scm::math::vec3f(it->point().normal.x(), it->point().normal.y(), it->point().normal.z());
            ++hc;

            it = polyMesh.vertices_begin();
            std::advance(it, index[Polyhedron::Vertex_const_iterator(hc->vertex())]);
            tri.v2_.pos_ = scm::math::vec3f(it->point().x(), it->point().y(), it->point().z());
            tri.v2_.tex_ = scm::math::vec2f(fi->t_coords[2].x(), fi->t_coords[2].y());
            tri.v2_.nml_ = scm::math::vec3f(it->point().normal.x(), it->point().normal.y(), it->point().normal.z());
            ++hc;

            tri.area_id = state.num_areas;
            tri.chart_id = per_node_chart_id_map_it.second[fi->id()];
            tri.tex_id = fi->tex_id;
            tri.tri_id = fi->tri_id;

            state.triangles.push_back(tri);
        }
        ++state.num_areas;
    }
};

static void assign_parallel(app_state& state)
{
    std::vector<uint32_t> area_ids;
    for(uint32_t area_id = 0; area_id < state.num_areas; ++area_id)
    {
        area_ids.push_back(area_id);
    }

    auto lambda_append = [&](uint64_t i, uint32_t id) -> void {
        // compare all triangles with chart bounding boxes

        uint32_t area_id = area_ids[i];

        for(auto& it : state.chart_map[area_id])
        {
            int chart_id = it.first;
            auto& chart = it.second;

            chart.all_triangle_ids_.insert(chart.original_triangle_ids_.begin(), chart.original_triangle_ids_.end());

            // add any triangles that intersect chart
            for(uint32_t tri_id = 0; tri_id < state.triangles.size(); ++tri_id)
            {
                const auto& tri = state.triangles[tri_id];
                if(tri.chart_id != -1 && tri.get_area() > 0.f)
                {
                    if(chart.box_.contains(scm::math::vec3d(tri.v0_.pos_)) || chart.box_.contains(scm::math::vec3d(tri.v1_.pos_)) || chart.box_.contains(scm::math::vec3d(tri.v2_.pos_)))
                    {
                        chart.all_triangle_ids_.insert(tri_id);
                    }
                }
            }
        }
    };

    uint32_t num_threads = std::min((size_t)24, area_ids.size());
    lamure::mesh::parallel_for(num_threads, area_ids.size(), lambda_append);
}

static void pack_areas(app_state& state)
{
    for(uint32_t area_id = 0; area_id < state.num_areas; ++area_id)
    {
        calculate_chart_tex_space_sizes(state.chart_map[area_id], state.triangles, state.texture_info_map);

        project_charts(state.chart_map[area_id], state.triangles);

        std::cout << "Projected " << state.chart_map[area_id].size() << " charts for area " << area_id << std::endl;
        std::cout << "Running rectangle packing for area " << area_id << std::endl;

        // init the rectangles
        std::vector<rectangle> rects;
        for(auto& chart_it : state.chart_map[area_id])
        {
            chart& chart = chart_it.second;
            if(chart.original_triangle_ids_.size() > 0)
            {
                rectangle rect = chart.rect_;
                rect.max_ *= packing_scale;
                rects.push_back(rect);
            }
        }

        // rectangle packing
        rectangle area_rect = pack(rects);
        area_rect.id_ = area_id;
        area_rect.flipped_ = false;
        state.area_rects.push_back(area_rect);

        std::cout << "Packing of area " << area_id << " complete (" << area_rect.max_.x << ", " << area_rect.max_.y << ")" << std::endl;

        // apply rectangles
        for(const auto& rect : rects)
        {
            state.chart_map[area_id][rect.id_].rect_ = rect;
            state.chart_map[area_id][rect.id_].projection.tex_space_rect = rect; // save for rendering from texture later on
        }
    }

    std::cout << "Packing " << state.area_rects.size() << " areas..." << std::endl;
    state.image_rect = pack(state.area_rects);
    std::cout << "Packing of all areas complete (" << state.image_rect.max_.x << ", " << state.image_rect.max_.y << ")" << std::endl;
}

static void apply_texture_space_transformation(app_state& state)
{
    std::cout << "Applying texture space transformation..." << std::endl;

#pragma omp parallel for
    for(uint32_t k = 0; k < state.area_rects.size(); k++)
    {
        auto& area_rect = state.area_rects[k];

        std::cout << "Area " << area_rect.id_ << " min: (" << area_rect.min_.x << ", " << area_rect.min_.y << ")" << std::endl;
        std::cout << "Area " << area_rect.id_ << " max: (" << area_rect.max_.x << ", " << area_rect.max_.y << ")" << std::endl;

#pragma omp parallel for
        // next, apply the global transformation from area packing onto all individual chart rects per area
        for(uint32_t j = 0; j < state.chart_map[area_rect.id_].size(); j++)
        {
            auto& chart = state.chart_map[area_rect.id_][j];
            chart.rect_.min_ += area_rect.min_;

            std::vector<int> ids(chart.all_triangle_ids_.begin(), chart.all_triangle_ids_.end());
#pragma omp parallel for
            // apply this transformation to the new parameterization
            for(uint32_t a = 0; a < ids.size(); a++)
            {
                int tri_id = ids[a];
                if((chart.rect_.flipped_ && !area_rect.flipped_) || (area_rect.flipped_ && !chart.rect_.flipped_))
                {
                    float temp = chart.all_triangle_new_coods_[tri_id][0].x;
                    chart.all_triangle_new_coods_[tri_id][0].x = chart.all_triangle_new_coods_[tri_id][0].y;
                    chart.all_triangle_new_coods_[tri_id][0].y = temp;

                    temp = chart.all_triangle_new_coods_[tri_id][1].x;
                    chart.all_triangle_new_coods_[tri_id][1].x = chart.all_triangle_new_coods_[tri_id][1].y;
                    chart.all_triangle_new_coods_[tri_id][1].y = temp;

                    temp = chart.all_triangle_new_coods_[tri_id][2].x;
                    chart.all_triangle_new_coods_[tri_id][2].x = chart.all_triangle_new_coods_[tri_id][2].y;
                    chart.all_triangle_new_coods_[tri_id][2].y = temp;
                }

                for(uint32_t i = 0; i < 3; ++i)
                {
                    chart.all_triangle_new_coods_[tri_id][i] *= packing_scale;
                    chart.all_triangle_new_coods_[tri_id][i].x += chart.rect_.min_.x;
                    chart.all_triangle_new_coods_[tri_id][i].y += chart.rect_.min_.y;
                    chart.all_triangle_new_coods_[tri_id][i].x /= state.image_rect.max_.x;
                    chart.all_triangle_new_coods_[tri_id][i].y /= state.image_rect.max_.x;
                }
            }
        }
    }
}

static void update_texture_coordinates(app_state& state)
{
    // use 2D array to account for different textures (if no textures were found, make sure it has at least one row)
    std::cout << "texture info map size: " << state.texture_info_map.size() << std::endl;
    state.to_upload_per_texture.resize(state.texture_info_map.size());

    // replacing texture coordinates in LOD file
    //...and at the same time, we will fill the upload per texture list
    std::cout << "Updating texture coordinates in leaf-level LOD nodes..." << std::endl;
    uint32_t num_invalid_tris = 0;
    uint32_t num_dropped_tris = 0;

    uint32_t first_leaf_id = state.bvh->get_first_node_id_of_depth(state.bvh->get_depth());
    uint32_t num_leaf_ids = state.bvh->get_length_of_depth(state.bvh->get_depth());

    for(uint32_t node_id = first_leaf_id; node_id < first_leaf_id + num_leaf_ids; ++node_id)
    {
        auto& tris = state.bvh->get_triangles(node_id);

        for(int local_tri_id = 0; local_tri_id < tris.size(); ++local_tri_id)
        {
            int32_t tri_id = ((node_id - first_leaf_id) * (state.bvh->get_primitives_per_node() / 3)) + local_tri_id;

            auto& tri = tris[local_tri_id];
            // std::cout << "tri id << " << tri.tri_id << " area id " << tri.area_id << " chart id " << tri.chart_id << std::endl;

            if(tri.chart_id != -1 && tri.get_area() > 0.f)
            {
                auto& chart = state.chart_map[tri.area_id][tri.chart_id];

                if(chart.all_triangle_ids_.find(tri_id) != chart.all_triangle_ids_.end())
                {
                    // create per-texture render list
                    if(tri.tex_id != -1)
                    {
                        const auto& old_tri = state.all_indexed_triangles[tri.tri_id];

                        double epsilon = FLT_EPSILON;

                        // obtain original coordinates
                        // since indexed cgal polyhedra dont preserve texture coordinates correctly
                        if(scm::math::length(tri.v0_.pos_ - old_tri.v0_.pos_) < epsilon)
                            tri.v0_.tex_ = old_tri.v0_.tex_;
                        else if(scm::math::length(tri.v0_.pos_ - old_tri.v1_.pos_) < epsilon)
                            tri.v0_.tex_ = old_tri.v1_.tex_;
                        else if(scm::math::length(tri.v0_.pos_ - old_tri.v2_.pos_) < epsilon)
                            tri.v0_.tex_ = old_tri.v2_.tex_;
                        else
                        {
                            std::cout << "WARNING: tex coord v0 could not be disambiguated (" << (int)(tri.tri_id == old_tri.tri_id_) << ")" << std::endl;
                        }

                        if(scm::math::length(tri.v1_.pos_ - old_tri.v0_.pos_) < epsilon)
                            tri.v1_.tex_ = old_tri.v0_.tex_;
                        else if(scm::math::length(tri.v1_.pos_ - old_tri.v1_.pos_) < epsilon)
                            tri.v1_.tex_ = old_tri.v1_.tex_;
                        else if(scm::math::length(tri.v1_.pos_ - old_tri.v2_.pos_) < epsilon)
                            tri.v1_.tex_ = old_tri.v2_.tex_;
                        else
                        {
                            std::cout << "WARNING: tex coord v1 could not be disambiguated (" << (int)(tri.tri_id == old_tri.tri_id_) << ")" << std::endl;
                        }

                        if(scm::math::length(tri.v2_.pos_ - old_tri.v0_.pos_) < epsilon)
                            tri.v2_.tex_ = old_tri.v0_.tex_;
                        else if(scm::math::length(tri.v2_.pos_ - old_tri.v1_.pos_) < epsilon)
                            tri.v2_.tex_ = old_tri.v1_.tex_;
                        else if(scm::math::length(tri.v2_.pos_ - old_tri.v2_.pos_) < epsilon)
                            tri.v2_.tex_ = old_tri.v2_.tex_;
                        else
                        {
                            std::cout << "WARNING: tex coord v2 could not be disambiguated (" << (int)(tri.tri_id == old_tri.tri_id_) << ")" << std::endl;
                        }

                        state.to_upload_per_texture[tri.tex_id].push_back(blit_vertex_t{tri.v0_.tex_, chart.all_triangle_new_coods_[tri_id][0]});
                        state.to_upload_per_texture[tri.tex_id].push_back(blit_vertex_t{tri.v1_.tex_, chart.all_triangle_new_coods_[tri_id][1]});
                        state.to_upload_per_texture[tri.tex_id].push_back(blit_vertex_t{tri.v2_.tex_, chart.all_triangle_new_coods_[tri_id][2]});
                    }
                    else
                    {
                        ++num_dropped_tris;
                    }

                    // override texture coordinates
                    tri.v0_.tex_ = chart.all_triangle_new_coods_[tri_id][0];
                    tri.v1_.tex_ = chart.all_triangle_new_coods_[tri_id][1];
                    tri.v2_.tex_ = chart.all_triangle_new_coods_[tri_id][2];

                    tri.v0_.tex_.y = 1.0 - tri.v0_.tex_.y; // flip y coord
                    tri.v1_.tex_.y = 1.0 - tri.v1_.tex_.y;
                    tri.v2_.tex_.y = 1.0 - tri.v2_.tex_.y;
                }
                else
                {
                    ++num_dropped_tris;
                }
            }
            else
            {
                ++num_invalid_tris;
            }
        }
    }

    std::cout << "Num tris with invalid chart ids encountered: " << num_invalid_tris << std::endl;
    std::cout << "Num dropped tris encountered: " << num_dropped_tris << std::endl;
}

static void write_bvh(app_state& state, std::string& obj_filename)
{
    std::string bvh_filename = obj_filename.substr(0, obj_filename.size() - 4) + ".bvh";
    state.bvh->write_bvh_file(bvh_filename);
    std::cout << "Bvh file written to " << bvh_filename << std::endl;

    std::string lod_filename = obj_filename.substr(0, obj_filename.size() - 4) + ".lod";
    state.bvh->write_lod_file(lod_filename);
    std::cout << "Lod file written to " << lod_filename << std::endl;

    // cleanup
    std::cout << "Cleanup bvh" << std::endl;
    state.bvh.reset();
}

static void create_viewports(app_state& state, cmd_options& opt)
{
    std::cout << "Single texture size limit: " << opt.single_tex_limit << std::endl;
    std::cout << "Multi texture size limit: " << opt.multi_tex_limit << std::endl;

    {
        state.t_d.render_to_texture_width_ = std::max(opt.single_tex_limit, 4096);
        state.t_d.render_to_texture_height_ = std::max(opt.single_tex_limit, 4096);

        opt.multi_tex_limit = std::max(state.t_d.render_to_texture_width_, (uint32_t)opt.multi_tex_limit); // TODO: why?

        state.t_d.full_texture_width_ = state.t_d.render_to_texture_width_;
        state.t_d.full_texture_height_ = state.t_d.render_to_texture_height_;
    }

    // double texture size up to 8k if a given percentage of charts do not have enough pixels
    std::cout << "Adjusting final texture size (" << state.t_d.full_texture_width_ << " x " << state.t_d.full_texture_height_ << ")" << std::endl;
    {
        calculate_new_chart_tex_space_sizes(state.chart_map, state.triangles, scm::math::vec2i(state.t_d.full_texture_width_, state.t_d.full_texture_height_));
        while(!is_output_texture_big_enough(state.chart_map, target_percentage_charts_with_enough_pixels))
        {
            if(std::max(state.t_d.full_texture_width_, state.t_d.full_texture_height_) >= opt.multi_tex_limit)
            {
                std::cout << "Maximum texture size limit reached (" << state.t_d.full_texture_width_ << " x " << state.t_d.full_texture_height_ << ")" << std::endl;
                break;
            }

            state.t_d.full_texture_width_ *= 2;
            state.t_d.full_texture_height_ *= 2;

            std::cout << "Not enough pixels! Adjusting final texture size (" << state.t_d.full_texture_width_ << " x " << state.t_d.full_texture_height_ << ")" << std::endl;

            calculate_new_chart_tex_space_sizes(state.chart_map, state.triangles, scm::math::vec2i(state.t_d.full_texture_width_, state.t_d.full_texture_height_));
        }
    }

    {
        // if output texture is bigger than 8k, create a set if viewports that will be rendered separately
        if(state.t_d.full_texture_width_ > state.t_d.render_to_texture_width_ || state.t_d.full_texture_height_ > state.t_d.render_to_texture_height_)
        {
            // calc num of viewports needed from size of output texture
            int viewports_w = std::ceil(state.t_d.full_texture_width_ / state.t_d.render_to_texture_width_); // TODO: looks broken, will never ceil due to int division
            int viewports_h = std::ceil(state.t_d.full_texture_height_ / state.t_d.render_to_texture_height_);

            scm::math::vec2f viewport_normed_size(1.f / viewports_w, 1.f / viewports_h);

            // create a vector of viewports needed
            for(int y = 0; y < viewports_h; ++y)
            {
                for(int x = 0; x < viewports_w; ++x)
                {
                    viewport vp;
                    vp.normed_dims = viewport_normed_size;
                    vp.normed_offset = scm::math::vec2f(viewport_normed_size.x * x, viewport_normed_size.y * y);
                    state.viewports.push_back(vp);
                }
            }
        }
        else
        {
            viewport single_viewport;
            single_viewport.normed_offset = scm::math::vec2f(0.f, 0.f);
            single_viewport.normed_dims = scm::math::vec2f(1.0, 1.0);
            state.viewports.push_back(single_viewport);
        }

        std::cout << "Created " << state.viewports.size() << " viewports to render multiple output textures" << std::endl;
    }
}

// subroutine for error-checking during shader compilation
static GLint compile_shader(const std::string& _src, GLint _shader_type)
{
    const char* shader_src = _src.c_str();
    GLuint shader = glCreateShader(_shader_type);

    glShaderSource(shader, 1, &shader_src, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        GLint log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

        GLchar* log = new GLchar[log_length + 1];
        glGetShaderInfoLog(shader, log_length, NULL, log);

        const char* type = NULL;
        switch(_shader_type)
        {
        case GL_VERTEX_SHADER:
            type = "vertex";
            break;
        case GL_FRAGMENT_SHADER:
            type = "fragment";
            break;
        default:
            break;
        }

        std::string error_message = "ERROR: Compile shader failure in " + std::string(type) + " shader:\n" + std::string(log);
        delete[] log;
        std::cout << error_message << std::endl;
        exit(1);
    }

    return shader;
}

// compile and link the shader programs
static void make_shader_program(GL_handles& handles)
{
    // locates vertices at new uv position on screen
    // passes old uvs in order to read from the texture

    // compile shaders
    GLint vertex_shader = compile_shader(vertex_shader_src, GL_VERTEX_SHADER);
    GLint fragment_shader = compile_shader(fragment_shader_src, GL_FRAGMENT_SHADER);

    // create the GL resource and save the handle for the shader program
    handles.shader_program_ = glCreateProgram();
    glAttachShader(handles.shader_program_, vertex_shader);
    glAttachShader(handles.shader_program_, fragment_shader);
    glLinkProgram(handles.shader_program_);

    // since the program is already linked, we do not need to keep the separate shader stages
    glDetachShader(handles.shader_program_, vertex_shader);
    glDeleteShader(vertex_shader);
    glDetachShader(handles.shader_program_, fragment_shader);
    glDeleteShader(fragment_shader);
}

// compile and link the shader programs
static void make_dilation_shader_program(GL_handles& handles)
{
    // compile shaders
    GLint vertex_shader = compile_shader(dilation_vertex_shader_src, GL_VERTEX_SHADER);
    GLint fragment_shader = compile_shader(dilation_fragment_shader_src, GL_FRAGMENT_SHADER);

    // create the GL resource and save the handle for the shader program
    handles.dilation_shader_program_ = glCreateProgram();
    glAttachShader(handles.dilation_shader_program_, vertex_shader);
    glAttachShader(handles.dilation_shader_program_, fragment_shader);
    glLinkProgram(handles.dilation_shader_program_);

    // since the program is already linked, we do not need to keep the separate shader stages
    glDetachShader(handles.dilation_shader_program_, vertex_shader);
    glDeleteShader(vertex_shader);
    glDetachShader(handles.dilation_shader_program_, fragment_shader);
    glDeleteShader(fragment_shader);
}

static void load_textures(app_state& state)
{
    state.area_images.resize(state.viewports.size());

    std::cout << "Loading all textures..." << std::endl;
    for(auto tex_it : state.texture_info_map)
    {
        state.textures.push_back(load_image(tex_it.second.filename_));
    }

    std::cout << "Compiling shaders..." << std::endl;
    make_shader_program(state.handles);
    make_dilation_shader_program(state.handles);

    std::cout << "Creating framebuffers..." << std::endl;

    // create output frame buffers
    for(int i = 0; i < 2; ++i)
    {
        state.frame_buffers.push_back(std::make_shared<frame_buffer_t>(1, state.t_d.render_to_texture_width_, state.t_d.render_to_texture_height_, GL_RGBA, GL_LINEAR));
    }

    // create vertex buffer for dilation
    float screen_space_quad_geometry[30]{-1.0, -1.0, 0.0, 0.0, 1.0, -1.0, 1.0, 0.0, -1.0, 1.0, 0.0, 1.0,

                                         1.0,  -1.0, 1.0, 0.0, 1.0, 1.0,  1.0, 1.0, -1.0, 1.0, 0.0, 1.0};
    glGenBuffers(1, &state.handles.dilation_vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, state.handles.dilation_vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), &screen_space_quad_geometry[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // set the viewport size
    glViewport(0, 0, (GLsizei)state.t_d.render_to_texture_width_, (GLsizei)state.t_d.render_to_texture_height_);

    // set background colour
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);

    glGenBuffers(1, &state.handles.vertex_buffer_);

    for(uint32_t view_id = 0; view_id < state.viewports.size(); ++view_id)
    {
        std::cout << "Rendering into viewport " << view_id << "..." << std::endl;

        viewport vport = state.viewports[view_id];

        std::cout << "Viewport start: " << vport.normed_offset.x << ", " << vport.normed_offset.y << std::endl;
        std::cout << "Viewport size: " << vport.normed_dims.x << ", " << vport.normed_dims.y << std::endl;

        state.frame_buffers[0]->enable();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for(uint32_t i = 0; i < state.to_upload_per_texture.size(); ++i)
        {
            uint32_t num_vertices = state.to_upload_per_texture[i].size();

            if(num_vertices == 0)
            {
                std::cout << "Nothing to render for texture " << i << " (" << state.texture_info_map[i].filename_ << ")" << std::endl;
                continue;
            }

            std::cout << "Rendering from texture " << i << " (" << state.texture_info_map[i].filename_ << ")" << std::endl;

            glUseProgram(state.handles.shader_program_);

            // upload this vector to GPU
            glBindBuffer(GL_ARRAY_BUFFER, state.handles.vertex_buffer_);
            glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(blit_vertex_t), &state.to_upload_per_texture[i][0], GL_STREAM_DRAW);

            // define the layout of the vertex buffer:
            // setup 2 attributes per vertex (2x texture coord)
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(blit_vertex_t), (void*)0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(blit_vertex_t), (void*)(2 * sizeof(float)));

            // get texture location
            int slot = 0;
            glUniform1i(glGetUniformLocation(state.handles.shader_program_, "image"), slot);
            glUniform2f(glGetUniformLocation(state.handles.shader_program_, "viewport_offset"), vport.normed_offset[0], vport.normed_offset[1]);
            glUniform2f(glGetUniformLocation(state.handles.shader_program_, "viewport_scale"), vport.normed_dims[0], vport.normed_dims[1]);

            glActiveTexture(GL_TEXTURE0 + slot);

            // here, enable the current texture
            state.textures[i]->enable(slot);

            // draw triangles from the currently bound buffer
            glDrawArrays(GL_TRIANGLES, 0, num_vertices);

            // unbind, unuse
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glUseProgram(0);

            state.textures[i]->disable();

        } // end for each texture

        state.frame_buffers[0]->disable();

        uint32_t current_framebuffer = 0;
        if(true)
        {
            std::cout << "Dilating view " << view_id << "..." << std::endl;

            uint32_t num_dilations = state.t_d.render_to_texture_width_ / 2;

            for(int i = 0; i < num_dilations; ++i)
            {
                current_framebuffer = (i + 1) % 2;

                state.frame_buffers[current_framebuffer]->enable();
                int current_texture = 0;
                if(current_framebuffer == 0)
                {
                    current_texture = 1;
                }

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glUseProgram(state.handles.dilation_shader_program_);
                glBindBuffer(GL_ARRAY_BUFFER, state.handles.dilation_vertex_buffer_);

                glEnableVertexAttribArray(0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(blit_vertex_t), (void*)0);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(blit_vertex_t), (void*)(2 * sizeof(float)));

                int slot = 0;
                glUniform1i(glGetUniformLocation(state.handles.dilation_shader_program_, "image"), slot);
                glUniform1i(glGetUniformLocation(state.handles.dilation_shader_program_, "image_width"), state.t_d.render_to_texture_width_);
                glUniform1i(glGetUniformLocation(state.handles.dilation_shader_program_, "image_height"), state.t_d.render_to_texture_height_);
                glActiveTexture(GL_TEXTURE0 + slot);
                state.frame_buffers[current_texture]->bind_texture(slot);

                glDrawArrays(GL_TRIANGLES, 0, 6);

                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glUseProgram(0);

                state.frame_buffers[current_texture]->unbind_texture(slot);

                state.frame_buffers[current_framebuffer]->disable();
            }
        }

        std::vector<uint8_t> pixels;
        state.frame_buffers[current_framebuffer]->get_pixels(0, pixels);
        state.area_images[view_id] = pixels;

    } // end for each viewport
}

static void produce_texture(app_state& state, std::string& obj_filename, cmd_options& opt)
{
    if(!opt.want_raw_file)
    {
        // concatenate all area images to one big texture
        uint32_t num_bytes_per_pixel = 4;
        std::vector<uint8_t> final_texture(state.t_d.full_texture_width_ * state.t_d.full_texture_height_ * num_bytes_per_pixel);

        uint32_t num_lookups_per_line = state.t_d.full_texture_width_ / state.t_d.render_to_texture_width_;

        for(uint32_t y = 0; y < state.t_d.full_texture_height_; ++y)
        { // for each line
            for(uint32_t tex_x = 0; tex_x < num_lookups_per_line; ++tex_x)
            {
                void* dst = ((void*)&final_texture[0]) + y * state.t_d.full_texture_width_ * num_bytes_per_pixel + tex_x * state.t_d.render_to_texture_width_ * num_bytes_per_pixel;
                uint32_t tex_y = y / state.t_d.render_to_texture_height_;
                uint32_t tex_id = tex_y * num_lookups_per_line + tex_x;
                void* src = ((void*)&state.area_images[tex_id][0]) + (y % state.t_d.render_to_texture_height_) * state.t_d.render_to_texture_width_ * num_bytes_per_pixel; //+ 0;

                memcpy(dst, src, state.t_d.render_to_texture_width_ * num_bytes_per_pixel);
            }
        }

        std::string image_filename = obj_filename.substr(0, obj_filename.size() - 4) + "_texture.png";
        utils::save_image(image_filename, final_texture, state.t_d.full_texture_width_, state.t_d.full_texture_height_);
    }
    else
    {
        std::ofstream raw_file;
        std::string image_filename =
            obj_filename.substr(0, obj_filename.size() - 4) + "_rgba_w" + std::to_string(state.t_d.full_texture_width_) + "_h" + std::to_string(state.t_d.full_texture_height_) + ".data";
        raw_file.open(image_filename, std::ios::out | std::ios::trunc | std::ios::binary);

        // concatenate all area images to one big texture
        uint32_t num_bytes_per_pixel = 4;

        uint32_t num_lookups_per_line = state.t_d.full_texture_width_ / state.t_d.render_to_texture_width_;

        for(uint32_t y = 0; y < state.t_d.full_texture_height_; ++y)
        { // for each line
            for(uint32_t tex_x = 0; tex_x < num_lookups_per_line; ++tex_x)
            {
                uint32_t tex_y = y / state.t_d.render_to_texture_height_;
                uint32_t tex_id = tex_y * num_lookups_per_line + tex_x;
                char* src = ((char*)&state.area_images[tex_id][0]) + (y % state.t_d.render_to_texture_height_) * state.t_d.render_to_texture_width_ * num_bytes_per_pixel; //+ 0;

                raw_file.write(src, state.t_d.render_to_texture_width_ * num_bytes_per_pixel);
            }
        }

        raw_file.close();
    }
}
} // namespace utils

#endif