// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <lamure/ren/model_database.h>
#include <lamure/bounding_box.h>

#include <lamure/ren/bvh.h>
#include <lamure/ren/lod_stream.h>

#include "knn.h"
#include "nni.h"

#include "node_queue.h"
#include "plane.h"

//#define MAIN_VERBOSE
//#define WRITE_UPDATED_KDN_FILE
#define RECOMPUTE_RADII
#define NUM_NEAREST_NEIGHBOURS_CONSIDERED 20
#define NUM_THREADS 24  
#define ON_FAIL_SET_ZERO

char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

struct s_surfel {
    float x, y, z;
    uint8_t r, g, b, fake;
    float size;
    float nx, ny, nz;
};

void process_tree(lamure::ren::bvh* bvh, lamure::ren::LodStream* in_lod_access, lamure::ren::LodStream* out_lod_access) {

    size_t node_size_in_bytes = bvh->surfels_per_node() * sizeof(surfel);
    surfel* in_surfels;
    
    //read all nodes
    in_surfels = new surfel[bvh->num_nodes() * bvh->surfels_per_node()];
    in_lod_access->read((char*)in_surfels, 0, bvh->num_nodes() * node_size_in_bytes);
    
    unsigned int num_threads = NUM_THREADS;
    surfel* out_surfels = new surfel[num_threads * bvh->surfels_per_node()];
    
    unsigned int num_nodes_done = 0; 
    node_queue_t job_queue;
    
    while (true) {
        
        if (true) {
            unsigned int progress = (unsigned int)(((float)(num_nodes_done) / (float)bvh->num_nodes()) * 25);
            std::cout << "\rLOG: [";
            for (unsigned int p = 0; p < 25; p++) {
                if (p < progress)
                    std::cout << "#";
                else
                    std::cout << ".";
            }
            std::cout << "] progress..." << std::flush;
            
        }
        
        for (unsigned int i = 0; i < num_threads; ++i) {
            unsigned int current = num_nodes_done + i;
            
            if (current < bvh->num_nodes()) {
                in_lod_access->read(((char*)out_surfels)+i*node_size_in_bytes, 
                    (size_t)current * node_size_in_bytes, node_size_in_bytes);
                
                job_queue.push_job(node_queue_t::job_t(i, current));
            }
        }
        
        unsigned int num_jobs = job_queue.Numjobs();
        
        
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < num_threads; ++i) {
            threads.push_back(std::thread([&]{
                
                while (true) {
                
                    job_queue.wait();
                    
                    if (job_queue.is_shutdown()) {
                        break;
                    }
                    
                    node_queue_t::job_t job = job_queue.pop_job();
                    
                    if (job.job_id_ != -1) {
                        
                        for (size_t splat_id = 0; splat_id < bvh->surfels_per_node(); ++splat_id) {
                            
                            surfel soi = in_surfels[job.node_id_*bvh->surfels_per_node()+splat_id];
                            NodeSplatId splat_of_interest = NodeSplatId(job.node_id_, splat_id);
                            std::vector<std::pair<surfel, float>> nearest_neighbours;
                           
                            unsigned int desired_num_nearest_neighbours = NUM_NEAREST_NEIGHBOURS_CONSIDERED;
                            
                            unsigned int min_num_nearest_neighbours = 10;
                            unsigned int min_num_natural_neighbours = 3;
                            
                            knn::find_nearest_neighbours(bvh, 
                                                         in_lod_access, 
                                                         splat_of_interest, 
                                                         desired_num_nearest_neighbours, 
                                                         nearest_neighbours, 
                                                         false, 
                                                         in_surfels);
                                                         
                            if (nearest_neighbours.size() < min_num_nearest_neighbours) {
                                 //out_surfels[job.job_id_*bvh->surfels_per_node() + splat_id].size = 0.f;
                                 nearest_neighbours.clear();
#ifdef MAIN_VERBOSE
                                 std::cout << "WARNING! could not find enough nearest neighbours (" 
                                    << nearest_neighbours.size() << "/" << min_num_nearest_neighbours << ")" << std::endl;
#endif                           

#ifdef ON_FAIL_SET_ZERO
                                 out_surfels[job.job_id_*bvh->surfels_per_node() + splat_id].size = 0.f;
#endif
                                 continue;
                            }
                            
#ifdef MAIN_VERBOSE
                            std::cout << "found " << nearest_neighbours.size() << " nearest neighbours" << std::endl;
#endif
                            //assert(!out_of_core);
                            scm::math::vec3f poi = scm::math::vec3f(soi.x, soi.y, soi.z);
 
#ifdef RECOMPUTE_RADII /*RECOMPUTE_RADII*/
                           
                            std::vector<std::pair<surfel, float>> natural_neighbours; //surfel + weight
                            
                            nni::find_natural_neighbours(nearest_neighbours, 
                                                         poi, 
                                                         natural_neighbours);
                            if (natural_neighbours.size() < min_num_natural_neighbours) {
                                 //out_surfels[job.job_id_*bvh->surfels_per_node() + splat_id].size = 0.f;
                                 nearest_neighbours.clear();
                                 natural_neighbours.clear();
#ifdef MAIN_VERBOSE
                                 std::cout << "WARNING! could not find enough natural neighbours ("
                                    << natural_neighbours.size() << "/" << min_num_natural_neighbours << ")" << std::endl;
#endif

#ifdef ON_FAIL_SET_ZERO
                                 out_surfels[job.job_id_*bvh->surfels_per_node() + splat_id].size = 0.f;
#endif
                                 continue;
                            }
                            
#ifdef MAIN_VERBOSE
                            std::cout << "found " << natural_neighbours.size() << " natural neighbours" << std::endl;
#endif


                            //determine most distant natural neighbour
                            float max_distance = 0.f;
                            for (const auto& nn : natural_neighbours) {
                                float new_dist = scm::math::length_sqr(poi-(scm::math::vec3f(nn.first.x, nn.first.y, nn.first.z)));
                                max_distance = std::max(max_distance, new_dist);
                            }

                            if (max_distance >= std::numeric_limits<float>::min()) {
                                max_distance = scm::math::sqrt(max_distance);
                                out_surfels[job.job_id_*bvh->surfels_per_node() + splat_id].size = 0.5f*max_distance;
                            }

#ifdef ON_FAIL_SET_ZERO
                            else { 
                                out_surfels[job.job_id_*bvh->surfels_per_node() + splat_id].size = 0.f;
                            }
#endif


                            natural_neighbours.clear();
#endif /*RECOMPUTE_RADII*/
                        }
                        
                    }
                
                
                }
            
            
            }));
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        //write results to file
        for (unsigned int i = 0; i < num_threads; ++i) {
        
            unsigned int current = num_nodes_done + i;
            
            if (current < bvh->num_nodes()) {
                out_lod_access->write(((char*)out_surfels)+i*node_size_in_bytes, 
                    (size_t)current * node_size_in_bytes, node_size_in_bytes);
                    
#ifdef WRITE_UPDATED_KDN_FILE
                //compute representative radius
                float representative_radius = 0.f;
                unsigned int num_valid_splats = 0;
                for (unsigned int splat_id = 0; splat_id < bvh->surfels_per_node(); ++splat_id) {
                    
                    if (out_surfels[i*bvh->surfels_per_node() + splat_id].size > 0.f) {
                        representative_radius += out_surfels[i*bvh->surfels_per_node() + splat_id].size;
                        ++num_valid_splats;
                    }
                    
                }
                if (num_valid_splats) {
                    representative_radius /= (float)num_valid_splats;
                }
        
                bvh->set_average_surfel_radius(current, representative_radius);
#endif
            }
        }
        
        num_nodes_done += num_jobs;
        job_queue.relaunch();
        
        if (num_nodes_done >= bvh->num_nodes()) {
            break;
        }

    }
    
    std::cout << std::endl;

    delete[] in_surfels;
    delete[] out_surfels;
    

}

int main(int argc, char *argv[]) {

    if (argc == 1 ||
        cmd_option_exists(argv, argv+argc, "-h") ||
        !cmd_option_exists(argv, argv+argc, "-f")) {
        std::cout << "Usage: " << argv[0] << " <flags> -f <input_file>\n" <<
            "INFO: bvh_post_radii\n" <<
            "\t-f: selects .bvh input file\n" <<
            "\t    (-f flag is required)\n" <<
            std::endl;
        return 0;
    }

    std::string input_bvh_file = std::string(get_cmd_option(argv, argv + argc, "-f"));

    std::string input_bvh_ext = input_bvh_file.substr(input_bvh_file.size()-3);
    if (input_bvh_ext.compare("bvh") != 0) {
        std::cout << "please specify a .bvh file as input" << std::endl;
        return 0;
    }
    
    std::string input_lod_file = input_bvh_file.substr(0, input_bvh_file.size()-3) + "lod";

    std::string output_lod_file = input_lod_file.substr(0, input_lod_file.size()-4) + "_pr.lod";
    std::string output_bvh_file = input_bvh_file.substr(0, input_bvh_file.size()-4) + "_pr.bvh";
    
    std::cout << "loading tree from " << input_bvh_file << std::endl;
    lamure::ren::bvh* bvh = new lamure::ren::bvh(input_bvh_file);
    
    lamure::ren::LodStream* in_lod_access = new lamure::ren::LodStream();
    in_lod_access->open(input_lod_file);
    lamure::ren::LodStream* out_lod_access = new lamure::ren::LodStream();
    out_lod_access->openForWriting(output_lod_file);
    
    std::cout << "tree has " << bvh->num_nodes() << " nodes" << std::endl;
    
    process_tree(bvh, in_lod_access, out_lod_access);
    
    delete in_lod_access;
    delete out_lod_access;
 
    bvh->write_bvh_file(output_bvh_file);

    delete bvh;

    return 0;
}


