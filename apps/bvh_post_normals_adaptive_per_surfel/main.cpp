// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <sstream>
#include <vector>
#include <set>
#include <algorithm>
#include <lamure/ren/model_database.h>
#include <lamure/bounding_box.h>

#include <lamure/ren/bvh.h>
#include <lamure/ren/lod_stream.h>

#include "knn.h"

#include "node_queue.h"
#include "normals.h"
#include "statistic.h"

//#define MAIN_VERBOSE
#define RECOMPUTE_NORMALS
#define NUM_THREADS 24
#define PRINT_STATISTICS

float MIN_NOISE_THRESHOLD = 0.2f;
float MAX_NOISE_THRESHOLD = 2.0f;
unsigned int MIN_NUM_NEIGHBOURS = 60;
unsigned int MAX_NUM_NEIGHBOURS = 360;

char* get_cmd_option(char** begin, char** end, const std::string & option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end)
        return *it;
    return 0;
}

bool cmd_option_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

float calc_noise(lamure::ren::bvh* bvh, surfel* data) { 
  
  Statistic x_stat;
  Statistic y_stat;
  Statistic z_stat;
  for (unsigned int surfel_id = 0; surfel_id < bvh->get_primitives_per_node(); ++surfel_id) {
    if (data[surfel_id].size >= std::numeric_limits<float>::min()) {
      x_stat.add(data[surfel_id].nx);
      y_stat.add(data[surfel_id].ny);
      z_stat.add(data[surfel_id].nz);
    }
  }
  x_stat.calc();
  y_stat.calc();
  z_stat.calc();

  return x_stat.getSD() + y_stat.getSD() + z_stat.getSD();

}



void process_node(lamure::ren::bvh* bvh, 
                  surfel* in_surfels, 
                  surfel* out_surfels, 
                  lamure::node_t node_id, 
                  lamure::ren::lod_stream* in_lod_access) {


   for (size_t splat_id = 0; splat_id < bvh->get_primitives_per_node(); ++splat_id) {

       surfel soi = in_surfels[node_id*bvh->get_primitives_per_node()+splat_id];
       NodeSplatId splat_of_interest = NodeSplatId(node_id, splat_id);

       //find initial 40 neighbours
       unsigned int desired_num_initial_neighbours = 40;
       unsigned int min_num_initial_neighbours = desired_num_initial_neighbours / 2;
       std::vector<std::pair<surfel, float>> initial_neighbours;
       knn::find_nearest_neighbours(bvh,
                                    in_lod_access,
                                    splat_of_interest,
                                    desired_num_initial_neighbours,
                                    initial_neighbours,
                                    false,
                                    in_surfels);

       if (initial_neighbours.size() < min_num_initial_neighbours) {
         std::cout << "WARNING! could not find enough initial neighbours ("
             << initial_neighbours.size() << "/" << desired_num_initial_neighbours << ")" << std::endl;
         //set to zero  
         out_surfels[node_id*bvh->get_primitives_per_node() + splat_id].size = 0.0f;
         initial_neighbours.clear();
         continue;
       }

#ifdef MAIN_VERBOSE
       std::cout << "found " << initial_neighbours.size() << " initial neighbours" << std::endl;
#endif

       //estimate noise
       float per_surfel_noise = calc_noise(bvh, (surfel*)(in_surfels + node_id * bvh->get_primitives_per_node())); 
 
       //compute filter size
       float normalized_noise = (per_surfel_noise - MIN_NOISE_THRESHOLD) / (MAX_NOISE_THRESHOLD - MIN_NOISE_THRESHOLD);
       unsigned int desired_num_filter_neighbours = MIN_NUM_NEIGHBOURS + normalized_noise * (MAX_NUM_NEIGHBOURS - MIN_NUM_NEIGHBOURS);

       initial_neighbours.clear();

       //find filter neighbours
       unsigned int min_num_filter_neighbours = desired_num_filter_neighbours / 2;
       std::vector<std::pair<surfel, float>> filter_neighbours;
       knn::find_nearest_neighbours(bvh,
                                    in_lod_access,
                                    splat_of_interest,
                                    desired_num_filter_neighbours,
                                    filter_neighbours,
                                    false,
                                    in_surfels);

       if (filter_neighbours.size() < min_num_filter_neighbours) {
         std::cout << "WARNING! could not find enough filter neighbours ("
             << filter_neighbours.size() << "/" << desired_num_filter_neighbours << ")" << std::endl;
         //set to zero  
         out_surfels[node_id*bvh->get_primitives_per_node() + splat_id].size = 0.0f; 
         filter_neighbours.clear();
         continue;
       }

#ifdef MAIN_VERBOSE
       std::cout << "found " << filter_neighbours.size() << " filter neighbours" << std::endl;
#endif

       //compute filtered normal
       scm::math::vec3f poi = scm::math::vec3f(soi.x, soi.y, soi.z);
       scm::math::vec3f knn_normal = scm::math::vec3f::zero();
       nrm::calculate_normal(filter_neighbours,
                             poi,
                             &knn_normal);
       
       if (scm::math::length(knn_normal) >= std::numeric_limits<float>::min()) {
          knn_normal = scm::math::normalize(knn_normal);
          out_surfels[node_id*bvh->get_primitives_per_node() + splat_id].nx = knn_normal.x;
          out_surfels[node_id*bvh->get_primitives_per_node() + splat_id].ny = knn_normal.y;
          out_surfels[node_id*bvh->get_primitives_per_node() + splat_id].nz = knn_normal.z;
       }
       else {
          std::cout << "WARNING! zero-length normal encountered" << std::endl;
          //set to zero  
          out_surfels[node_id*bvh->get_primitives_per_node() + splat_id].size = 0.0f; 
          filter_neighbours.clear();
          continue; 
       }

   }
}

unsigned process_tree(lamure::ren::bvh* bvh, lamure::ren::lod_stream* in_lod_access, 
    lamure::ren::lod_stream* out_lod_access, std::vector<lamure::node_t> wishlist) {


    size_t node_size_in_bytes = bvh->get_primitives_per_node() * sizeof(surfel);
    surfel* in_surfels;
    
    //read all nodes
    in_surfels = new surfel[bvh->get_num_nodes() * bvh->get_primitives_per_node()];
    in_lod_access->read((char*)in_surfels, 0, bvh->get_num_nodes() * node_size_in_bytes);


    surfel* out_surfels = new surfel[bvh->get_num_nodes() * bvh->get_primitives_per_node()];
    memcpy((unsigned char*)out_surfels, (unsigned char*)in_surfels, 
           bvh->get_num_nodes() * bvh->get_primitives_per_node() * sizeof(surfel));
  
    unsigned int num_nodes_done = 0; 
    unsigned int num_nodes_todo = wishlist.size();
   
    //process wishlist in parallel
    node_queue_t job_queue;
    
    unsigned int num_threads = NUM_THREADS; 
    
    while (true) {
        
        if (true) {
            unsigned int progress = (unsigned int)(((float)(num_nodes_done) / (float)num_nodes_todo) * 25);
            std::cout << "\rLOG: [";
            for (unsigned int p = 0; p < 25; p++) {
                if (p < progress)
                    std::cout << "#";
                else
                    std::cout << ".";
            }
            std::cout << "] progress..." << std::flush;
            
        }
        
        unsigned int i = 0;
        while (i < num_threads && !wishlist.empty()) { 
            unsigned int node_id = wishlist.back();
            job_queue.push_job(node_queue_t::job_t(i, node_id));
            ++i;
            wishlist.pop_back();
        }
        
        unsigned int num_jobs = job_queue.Numjobs();
        if (num_jobs == 0) {
           break;
        }
        
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
                       process_node(bvh, in_surfels, out_surfels, job.node_id_, in_lod_access);
                    }
                }
            
            
            }));
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        num_nodes_done += num_jobs;
        job_queue.relaunch();
        
        if (num_nodes_done >= num_nodes_todo) {
            break;
        }

    }

    //write to disk
    out_lod_access->write((char*)out_surfels, 0, bvh->get_num_nodes() * node_size_in_bytes);

    delete[] in_surfels;
    delete[] out_surfels;

    std::cout << "Done. " << num_nodes_todo << " nodes were re-processed." << std::endl;
    return num_nodes_todo;
}




int main(int argc, char *argv[]) {

    if (argc == 1 ||
        cmd_option_exists(argv, argv+argc, "-h") ||
        !cmd_option_exists(argv, argv+argc, "-o") ||
        !cmd_option_exists(argv, argv+argc, "-f")) {
        std::cout << "Usage: " << argv[0] << " <flags> -f <input_file>\n" <<
            "INFO: post_normals\n" <<
            "\t-f: selects .bvh input file\n" <<
            "\t    (-f flag is required)\n" <<
            "\t-o: select output prefix (without extensions)" << 
            "\t    (-o flag is required)\n" <<
            "\t-m: select MIN_NOISE_THRESHOLD\n" <<
            "\t-M: select MAX_NOISE_THRESHOLD\n" <<
            "\t-n: select MIN_NUM_NEIGHBOURS\n" <<
            "\t-N: select MAX_NUM_NEIGHBOURS\n" << 
            std::endl;
        return 0;
    }
    
    std::string input_bvh_file = std::string(get_cmd_option(argv, argv + argc, "-f"));

    std::string input_bvh_ext = input_bvh_file.substr(input_bvh_file.size()-3);
    if (input_bvh_ext.compare("bvh") != 0) {
        std::cout << "please specify a .bvh file as input" << std::endl;
        return 0;
    }
    auto start_time = std::chrono::high_resolution_clock::now();
    std::string input_lod_file = input_bvh_file.substr(0, input_bvh_file.size()-3) + "lod";

    std::string output_prefix = std::string(get_cmd_option(argv, argv+argc, "-o"));

    std::string output_lod_file = output_prefix + ".lod";
    std::string output_bvh_file = output_prefix + ".bvh";
    
    if (cmd_option_exists(argv, argv+argc, "-m")) {
      MIN_NOISE_THRESHOLD = atof(get_cmd_option(argv, argv+argc, "-m"));
    } 
 
    if (cmd_option_exists(argv, argv+argc, "-M")) {
      MAX_NOISE_THRESHOLD = atof(get_cmd_option(argv, argv+argc, "-M"));
    } 
 
    if (cmd_option_exists(argv, argv+argc, "-n")) {
      MIN_NUM_NEIGHBOURS = atoi(get_cmd_option(argv, argv+argc, "-n"));
    } 
 
    if (cmd_option_exists(argv, argv+argc, "-N")) {
      MAX_NUM_NEIGHBOURS = atoi(get_cmd_option(argv, argv+argc, "-N"));
    } 

    std::cout << "loading tree from " << input_bvh_file << std::endl;
    lamure::ren::bvh* bvh = new lamure::ren::bvh(input_bvh_file);
    
    lamure::ren::lod_stream* in_lod_access = new lamure::ren::lod_stream();
    in_lod_access->open(input_lod_file);

#ifdef RECOMPUTE_NORMALS
    lamure::ren::lod_stream* out_lod_access = new lamure::ren::lod_stream();
    out_lod_access->open_for_writing(output_lod_file);
#endif

    std::cout << "tree has " << bvh->get_num_nodes() << " nodes" << std::endl;
    
    size_t node_size_in_bytes = bvh->get_primitives_per_node() * sizeof(surfel);

    //fill wishlist (entire tree)	    
    std::vector<lamure::node_t> wishlist;
    for (lamure::node_t node_id = 0; node_id < bvh->get_num_nodes(); ++node_id) {
       wishlist.push_back(node_id);
    }

#ifdef RECOMPUTE_NORMALS
    unsigned node_processed = process_tree(bvh, in_lod_access, out_lod_access, wishlist);
#endif

    auto end_time = std::chrono::high_resolution_clock::now();
    delete in_lod_access;
    delete out_lod_access;

#ifdef PRINT_STATISTICS
    //print first 50 to text file with same name
    std::string output_log_file = output_prefix + ".log";
    std::ofstream logfile;
    logfile.open(output_log_file.c_str());
    
    logfile << "input file: " << input_bvh_file << std::endl; 
    logfile << "nodes procesed: " << node_processed << " from " << bvh->get_num_nodes() << std::endl;
    logfile << "duration in seconds: " << std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count() << std::endl;
    logfile << "parameters: " << std::endl;
    logfile << "MIN_NOISE_THRESHOLD: " << MIN_NOISE_THRESHOLD << std::endl;
    logfile << "MAX_NOISE_THRESHOLD: " << MAX_NOISE_THRESHOLD << std::endl;
    logfile << "MIN_NUM_NEIGHBOURS: " << MIN_NUM_NEIGHBOURS << std::endl;
    logfile << "MAX_NUM_NEIGHBOURS: " << MAX_NUM_NEIGHBOURS << std::endl;
   
    logfile << "--------------------------------------------------------------" << std::endl;
    logfile.close();
#endif

#ifdef RECOMPUTE_NORMALS
    bvh->write_bvh_file(output_bvh_file);
    std::cout << "Done. " << std::endl;
#endif

    delete bvh;

    return 0;
}


