// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/reduction_random.h>
#include <lamure/pre/surfel.h>
#include <set>

namespace lamure {
namespace pre {

surfel_mem_array reduction_random::
create_lod(real& reduction_error,
          const std::vector<surfel_mem_array*>& input,
          const real avg_radius_all_nodes,
          const uint32_t surfels_per_node,
          const bvh& tree,
          const size_t start_node_id) const
{
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);
    surfel_mem_array output_mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    const uint32_t fan_factor = input.size();
    size_t point_id = 0;

    std::set <size_t> random_id; 
    std::set <size_t>::iterator set_it;

    

    /*//compute max total number of surfels from all nodes
    for ( size_t node_id = 0; node_id < fan_factor; ++node_id) {
        summed_length_of_nodes += input[node_id]->length();
    }*/

    //push input in a single surfel_mem_array
    for (size_t node_id = 0; node_id < input.size(); ++node_id) {
        for (size_t surfel_id = input[node_id]->offset();
                    surfel_id < input[node_id]->offset() + input[node_id]->length();
                    ++surfel_id){

            auto current_surfel = input[node_id]->mem_data()->at(input[node_id]->offset() + surfel_id);

            // ignore outlier radii of any kind
            if (current_surfel.radius() == 0.0) {
                //^^++radius_discarded_surfels;
                continue;
            }              

            mem_array.mem_data()->push_back(current_surfel);
       }
    }    
    

    subsample(mem_array,avg_radius_all_nodes);
    //draw random number within interval 0 to total number of surfels
    /*for (size_t i = 0; i < input[0]->length(); ++i){
        point_id = rand() % summed_length_of_nodes;

        //check, if point_id repeats
        set_it = random_id.find(point_id);

        //if so, draw another random number 
        if (set_it != random_id.end()){
            do {
                point_id = rand() % (input[1]->length()*fan_factor);   
                set_it = random_id.find(point_id);
                random_id.insert (point_id);
            } while(set_it != random_id.end());
        }*/

    size_t total_num_surfels = mem_array.mem_data()->size();
    
    
    //draw random number within interval 0 to total number of surfels
    for (size_t i = 0; i < surfels_per_node; ++i){
        point_id = rand() % total_num_surfels;

        //check, if point_id repeats
        set_it = random_id.find(point_id);

        //if so, draw another random number 
        if (set_it != random_id.end()){
            do {
                point_id = rand() % (input[1]->length()*fan_factor);   
                set_it = random_id.find(point_id);
                random_id.insert (point_id);
            } while(set_it != random_id.end());
        }

        //store random number
        random_id.insert (point_id);                  
                     

        //compute node id depending on randomly generated int value
       /* size_t node_id = 0;

        for( ; node_id < fan_factor; ++node_id) {
            if ((point_id) < input[node_id]->length()){
                break;
            } else {
                point_id -= input[node_id]->length();
            }
        }*/

                   
    }

    for(auto point_id : random_id){

        auto surfel = mem_array.mem_data()->at(point_id);
        output_mem_array.mem_data()->push_back(surfel); 
    }

    output_mem_array.set_length(output_mem_array.mem_data()->size());

    reduction_error = 0.0;

    return output_mem_array;
}

void reduction_random::
subsample(surfel_mem_array& joined_input, real const avg_radius) const{

    //std::cout<< "I should work on node with id: " << node.node_id() << "\n";
    const real max_radius_difference = 0.0; //
    //real avg_radius = node.avg_surfel_radius();

    auto compute_new_position = [] (surfel const& plane_ref_surfel, real radius_offset, real rot_angle) {
        vec3r new_position (0.0, 0.0, 0.0);

        vec3f n = plane_ref_surfel.normal();

        //poooo[0] += rand_x;
        //poooo[1] += rand_y;
        //from random_point_on_surfel() in surfe.cpp
        //find a vector orthogonal to given normal vector
        scm::math::vec3f  u(std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest(),
                            std::numeric_limits<float>::lowest());
        if(n.z != 0.0) {
            u = scm::math::vec3f( 1, 1, (-n.x - n.y) / n.z);
        } else if (n.y != 0.0) {
            u = scm::math::vec3f( 1, (-n.x - n.z) / n.y, 1);
        } else {
            u = scm::math::vec3f( (-n.y - n.z)/n.x, 1, 1);
        }
        scm::math::normalize(u);
        vec3f p = scm::math::normalize(scm::math::cross(n,u)); //plane of rotation given by cross product of n and u
        u = p; //^^

        //vector rotation according to: https://en.wikipedia.org/wiki/Rodrigues'_rotation_formula
        //rotation around the normal vector n
        vec3f u_rotated = u*cos(rot_angle) + scm::math::normalize(scm::math::cross(u,n))*sin(rot_angle) + n*scm::math::dot(u,n)*(1-cos(rot_angle));

        //extend vector  lenght to match desired radius 
        u_rotated = scm::math::normalize(u_rotated)*radius_offset;
        
        new_position = plane_ref_surfel.pos() + u_rotated;
        return new_position; 
    };

    //create new vector to store node surfels; unmodified + modified ones
    surfel_mem_array modified_mem_array (std::make_shared<surfel_vector>(surfel_vector()), 0, 0);


    for(uint32_t i = 0; i < joined_input.mem_data()->size(); ++i){

        surfel current_surfel = joined_input.read_surfel(i);

        //replace big surfels by collection of average-size surfels
        if (current_surfel.radius() - avg_radius > max_radius_difference){
               
            //how many times does average radius fit into big radius
            int iteration_level = floor(current_surfel.radius()/(2*avg_radius));             

            //keep all surfel properties but shrink its radius to the average radius
            surfel new_surfel = current_surfel;
            //joined_input.mem_data()->pop_back(); //remove the big-radius surfel
            new_surfel.radius() = avg_radius;
            joined_input.write_surfel(new_surfel, i);            
            //modified_mem_array.mem_data()->push_back(new_surfel);

            //create new average-size surfels to fill up the area orininally covered by bigger surfel            
            for(int k = 1; k <= (iteration_level - 1); ++k){
               uint16_t num_new_surfels = 6*k; //^^ formula basis https://en.wikipedia.org/wiki/Circle_packing_in_a_circle
               real angle_offset = (360) / num_new_surfels;
               //std::cout << "angle_offset "<< angle_offset << "\n";
               real angle = 0.0; //reset 
               for(int j = 0; j < num_new_surfels; ++j){
                    real radius_offset = k*2*avg_radius; 
                    //new_surfel.color() = vec3b(50, 130, 130);
                    new_surfel.pos() = compute_new_position(current_surfel, radius_offset, angle);
                    modified_mem_array.mem_data()->push_back(new_surfel);
                    angle = angle + angle_offset;                    
               }
            }          
        }
    }

    for(uint32_t i = 0; i < modified_mem_array.mem_data()->size(); ++i) {
        joined_input.mem_data()->push_back( modified_mem_array.mem_data()->at(i) );
    }

}

} // namespace pre
} // namespace lamure
