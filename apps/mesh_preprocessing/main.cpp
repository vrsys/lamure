#include "utils.h"

using namespace utils;

#define MEASURE_EXECUTION_TIME

#ifdef MEASURE_EXECUTION_TIME
template <typename TimeT = std::chrono::milliseconds>
struct measure
{
    template <typename F, typename... Args>
    static typename TimeT::rep execution(F&& func, Args&&... args)
    {
        auto start = std::chrono::steady_clock::now();
        std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
        auto duration = std::chrono::duration_cast<TimeT>(std::chrono::steady_clock::now() - start);
        return duration.count();
    }
};
#endif

template <typename F, typename... Args>
void execute_stage(std::string stage_name, F&& func, Args&&... args)
{
    std::cout << std::endl;
    std::cout << "# Executing stage: " << stage_name << std::endl;
    std::cout << std::endl;

#ifdef MEASURE_EXECUTION_TIME
    float millis = measure<>::execution(func, std::forward<Args>(args)...);

    std::cout << std::endl;
    std::cout << "# Stage took " << std::to_string(millis) << " ms" << std::endl;
    std::cout << std::endl;
#else
    std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
#endif
}

int main(int argc, char** argv)
{
    std::string obj_filename = "";
    if(cmdOptionExists(argv, argv + argc, "-f"))
    {
        obj_filename = std::string(getCmdOption(argv, argv + argc, "-f"));
    }
    else
    {
        print_help_message();
        return 1;
    }

    cmd_options opt;
    extract_cmd_options(argc, argv, obj_filename, opt);

    initialize_glut_window(argc, argv, opt);

    app_state state;

    execute_stage("Load obj from " + obj_filename, [&] { load_obj(obj_filename, state.all_indexed_triangles, state.texture_info_map); });

    execute_stage("Build kd tree and initialize nodes", [&] {
        state.kdtree = std::make_shared<kdtree_t>(state.all_indexed_triangles, opt.num_tris_per_node_kdtree);
        initialize_nodes(state);
    });

    execute_stage("Chartify in parallel", [&] { chartify_parallel(state, opt); });

    execute_stage("Convert back to triangle soup", [&] { convert_to_triangle_soup(state); });

    execute_stage("Create LOD hierarchy and reorder triangles", [&] {
        state.bvh = std::make_shared<lamure::mesh::bvh>(state.triangles, opt.num_tris_per_node_bvh);
        reorder_triangles(state);
    });

    execute_stage("Prepare charts", [&] { prepare_charts(state); });

    execute_stage("Expand charts throughout BVH", [&] { expand_charts(state); });

    execute_stage("Assign additional triangles to charts in parallel", [&] { assign_parallel(state); });

    execute_stage("Pack areas", [&] { pack_areas(state); });

    execute_stage("Apply texture space transformations", [&] { apply_texture_space_transformation(state); });

    execute_stage("Update texture coordinates", [&] { update_texture_coordinates(state); });

    execute_stage("Write BVH", [&] { write_bvh(state, obj_filename); });

    execute_stage("Create viewports", [&] { create_viewports(state, opt); });

    execute_stage("Load textures", [&] { load_textures(state); });

    execute_stage("Produce final textures", [&] { produce_texture(state, obj_filename, opt); });

    exit(0);
}