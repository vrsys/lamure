#include "utils.h"

int main(int argc, char** argv)
{
    std::string obj_filename = "";
    if(utils::cmdOptionExists(argv, argv + argc, "-f"))
    {
        obj_filename = std::string(utils::getCmdOption(argv, argv + argc, "-f"));
    }
    else
    {
        utils::print_help_message();
        return 1;
    }

    utils::initialize_glut_window(argc, argv);

    cmd_options opt;
    utils::extract_cmd_options(argc, argv, obj_filename, opt);

    app_state state;

    {
        std::cout << "Loading obj from " << obj_filename << "..." << std::endl;

        utils::load_obj(obj_filename, state.all_indexed_triangles, state.texture_info_map);
    }

    std::cout << "Building kd tree..." << std::endl;
    state.kdtree = std::make_shared<kdtree_t>(state.all_indexed_triangles, opt.num_tris_per_node_kdtree);
    utils::initialize_nodes(state);

    utils::chartify_parallel(state, opt);
    utils::convert_to_triangle_soup(state);

    std::cout << "Creating LOD hierarchy..." << std::endl;
    state.bvh = std::make_shared<lamure::mesh::bvh>(state.triangles, opt.num_tris_per_node_bvh);

    utils::reorder_triangles(state);

    std::cout << "Preparing charts..." << std::endl;
    utils::prepare_charts(state);

    std::cout << "Expanding charts throughout BVH..." << std::endl;
    utils::expand_charts(state);

    utils::assign_parallel(state);

    utils::pack_areas(state);
    utils::apply_texture_space_transformation(state);

    utils::update_texture_coordinates(state);

    utils::write_bvh(state, obj_filename);

    std::cout << "Single texture size limit: " << opt.single_tex_limit << std::endl;
    std::cout << "Multi texture size limit: " << opt.multi_tex_limit << std::endl;

    utils::create_viewports(state, opt);

    utils::load_textures(state);

    std::cout << "Producing final texture..." << std::endl;

    utils::produce_texture(state, obj_filename);

    return 0;
}