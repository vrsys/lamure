#include "utils.h"

using namespace utils;

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
void execute_stage(std::string stage_name, app_state& state, F&& func, Args&&... args)
{
    std::cout << std::endl;
    std::cout << "# Executing stage: " << stage_name << std::endl;
    std::cout << std::endl;

#ifdef FLUSH_APP_STATE
    std::string save_path(std::experimental::filesystem::current_path().generic_string() + "/app_state.b_arch");
    std::cout << "Flushing the app state to archive: " << save_path << std::endl;
    ofstream ofstream_state(save_path);
    boost::archive::binary_oarchive oa_state(ofstream_state);
    oa_state << state;
    ofstream_state.close();
    std::cout << "State flushed" << std::endl;
#endif

#ifdef MEASURE_EXECUTION_TIME
    float millis = measure<>::execution(func, std::forward<Args>(args)...);

    std::cout << std::endl;
    std::cout << "# Stage took " << std::to_string(millis) << " ms" << std::endl;
    std::cout << std::endl;
#else
    std::forward<decltype(func)>(func)(std::forward<Args>(args)...);
#endif
}

#ifdef FLUSH_APP_STATE
enum STAGE
{
    LOAD_OBJ = 0,
    BUILD_KD_TREE = 1,
    CHARTIFY = 2,
    CONVERT_BACK = 3,
    CREATE_LOD = 4,
    PREPARE_CHARTS = 5,
    EXPAND_CHARTS = 6,
    ASSIGN_TRIANGLES = 7,
    PACK_AREAS = 8,
    APPLY_TEXTURE_TRANSFORM = 9,
    UPDATE_TEXTURE_COORDINATES = 10,
    WRITE_BVH = 11,
    CREATE_VIEWPORTS = 12,
    LOAD_TEXTURES = 13,
    PRODUCE_TEXTURES = 14
};

void pick_up_from(STAGE& stage, std::string& obj_filename, cmd_options& opt)
{
    app_state state;

    std::string save_path(std::experimental::filesystem::current_path().generic_string() + "/app_state.b_arch");
    std::cout << "Reading the app state from archive: " << save_path << std::endl;
    ifstream ifstream_state(save_path);
    boost::archive::binary_iarchive ia_state(ifstream_state);
    ia_state >> state;
    ifstream_state.close();
    std::cout << "State read" << std::endl;

    if(stage <= LOAD_OBJ)
    {
        execute_stage("Load obj from " + obj_filename, state, [&] { load_obj(obj_filename, state.all_indexed_triangles, state.texture_info_map); });
    }

    /*** Charting operations ***/
    {
        if(stage <= BUILD_KD_TREE)
        {
            execute_stage("Build kd tree and initialize nodes", state, [&] {
                state.kdtree = std::make_shared<kdtree_t>(state.all_indexed_triangles, opt.num_tris_per_node_kdtree);
                initialize_nodes(state);
            });
        }

        if(stage <= CHARTIFY)
        {
            execute_stage("Chartify in parallel", state, [&] { chartify_parallel(state, opt); });
        }

        if(stage <= CONVERT_BACK)
        {
            execute_stage("Convert back to triangle soup in parallel", state, [&] { convert_to_triangle_soup_parallel(state); });
        }

        state.per_node_chart_id_map.clear();
        state.per_node_polyhedron.clear();
        state.kdtree.reset();
        state.node_ids.clear();
    }

    /*** LOD, Texture & Texture coordinates operations ***/
    {
        if(stage <= CREATE_LOD)
        {
            execute_stage("Create LOD hierarchy and reorder triangles", state, [&]
            {
                state.bvh = std::make_shared<lamure::mesh::bvh>(state.triangles, opt.num_tris_per_node_bvh);
                reorder_triangles(state);
            });
        }

        if(stage <= PREPARE_CHARTS)
        {
            execute_stage("Prepare charts", state, [&] { prepare_charts(state); });
        }

        if(stage <= EXPAND_CHARTS)
        {
            execute_stage("Expand charts throughout BVH", state, [&] { expand_charts(state); });
        }

        if(stage <= ASSIGN_TRIANGLES)
        {
            execute_stage("Assign additional triangles to charts in parallel", state, [&] { assign_parallel(state); });
        }

        if(stage <= PACK_AREAS)
        {
            execute_stage("Pack areas", state, [&] { pack_areas(state); });
        }

        if(stage <= APPLY_TEXTURE_TRANSFORM)
        {
            execute_stage("Apply texture space transformations in parallel", state, [&] { apply_texture_space_transformations_in_parallel(state); });
        }

        if(stage <= UPDATE_TEXTURE_COORDINATES)
        {
            execute_stage("Update texture coordinates", state, [&] { update_texture_coordinates(state); });
        }

        state.all_indexed_triangles.clear();
        state.area_rects.clear();

        if(stage <= WRITE_BVH)
        {
            execute_stage("Write BVH", state, [&] { write_bvh(state, obj_filename); });
        }

        if(stage <= CREATE_VIEWPORTS)
        {
            execute_stage("Create viewports", state, [&] { create_viewports(state, opt); });
        }

        state.chart_map.clear();
        state.triangles.clear();

        if(stage <= LOAD_TEXTURES)
        {
            execute_stage("Load textures", state, [&] { load_textures(state); });
        }

        state.texture_info_map.clear();
        state.viewports.clear();
        state.to_upload_per_texture.clear();
        state.frame_buffers.clear();
        state.textures.clear();

        if(stage <= PRODUCE_TEXTURES)
        {
            execute_stage("Produce final textures", state, [&] { produce_texture(state, obj_filename, opt); });
        }
    }
}
#endif

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

#ifdef FLUSH_APP_STATE
    if(cmdOptionExists(argv, argv + argc, "-pickupfrom"))
    {
        STAGE stage_id = (STAGE)std::stoi(getCmdOption(argv, argv + argc, "-pickupfrom"));
        pick_up_from(stage_id, obj_filename, opt);

        exit(0);
    }
#endif

    app_state state;

    // all_indexed_triangles, texture_info_map
    execute_stage("Load obj from " + obj_filename, state, [&] { load_obj(obj_filename, state.all_indexed_triangles, state.texture_info_map); });

    /*** Charting operations ***/
    {
        // node_ids kdtree, all_indexed_triangles
        execute_stage("Build kd tree and initialize nodes", state, [&] {
            state.kdtree = std::make_shared<kdtree_t>(state.all_indexed_triangles, opt.num_tris_per_node_kdtree);
            initialize_nodes(state);
        });

        // node_ids, kdtree, per_node_chart_id_map, per_node_polyhedron
        execute_stage("Chartify in parallel", state, [&] { chartify_parallel(state, opt); });

        // num_areas, triangles, per_node_chart_id_map, per_node_polyhedron,
        execute_stage("Convert back to triangle soup in parallel", state, [&] { convert_to_triangle_soup_parallel(state); });

        state.per_node_chart_id_map.clear();
        state.per_node_polyhedron.clear();
        state.kdtree.reset();
        state.node_ids.clear();
    }

    /*** LOD, Texture & Texture coordinates operations ***/
    {
        // bvh, triangles
        execute_stage("Create LOD hierarchy and reorder triangles", state, [&] {
            state.bvh = std::make_shared<lamure::mesh::bvh>(state.triangles, opt.num_tris_per_node_bvh);
            reorder_triangles(state);
        });

        // triangles, chart_map, num_areas
        execute_stage("Prepare charts", state, [&] { prepare_charts(state); });

        // chart_map, bvh
        execute_stage("Expand charts throughout BVH", state, [&] { expand_charts(state); });

        // num_areas, chart_map, triangles
        execute_stage("Assign additional triangles to charts in parallel", state, [&] { assign_parallel(state); });

        // num_areas, chart_map, triangles, image_rect, area_rects, texture_info_map
        execute_stage("Pack areas", state, [&] { pack_areas(state); });

        // chart_map, image_rect, area_rects
        execute_stage("Apply texture space transformations in parallel", state, [&] { apply_texture_space_transformations_in_parallel(state); });

        // to_upload_per_texture, texture_info_map, bvh, chart_map, area_rects, image_rect, all_indexed_triangles
        execute_stage("Update texture coordinates", state, [&] { update_texture_coordinates(state); });

        state.all_indexed_triangles.clear();
        state.area_rects.clear();

        // bvh
        execute_stage("Write BVH", state, [&] { write_bvh(state, obj_filename); });

        // t_d, chart_map, triangles, viewports
        execute_stage("Create viewports", state, [&] { create_viewports(state, opt); });

        state.chart_map.clear();
        state.triangles.clear();

        // textures, handles, frame_buffers, texture_info_map, t_d, viewports, to_upload_per_texture, area_images
        execute_stage("Load textures", state, [&] { load_textures(state); });

        state.texture_info_map.clear();
        state.viewports.clear();
        state.to_upload_per_texture.clear();
        state.frame_buffers.clear();
        state.textures.clear();

        // t_d, area_images
        execute_stage("Produce final textures", state, [&] { produce_texture(state, obj_filename, opt); });
    }

    exit(0);
}