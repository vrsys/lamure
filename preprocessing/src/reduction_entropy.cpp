// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#ifdef CMAKE_OPTION_ENABLE_ALTERNATIVE_STRATEGIES

#include <lamure/pre/reduction_entropy.h>

//#include <math.h>
#include <functional>
#include <numeric>
#include <vector>
#include <queue>
#include <map>
#include <set>

namespace lamure
{
namespace pre
{

surfel_mem_array reduction_entropy::
create_lod(real &reduction_error,
           const std::vector<surfel_mem_array *> &input,
           const uint32_t surfels_per_node,
           const bvh &tree,
           const size_t start_node_id) const
{
    if (input[0]->has_provenance()) {
      throw std::runtime_error("reduction_entropy not supported for PROVENANCE");
    }

    //create output array
    surfel_mem_array mem_array(std::make_shared<surfel_vector>(surfel_vector()), 0, 0);

    //container for all input surfels including entropy (entropy_surfel_array = ESA)
    shared_entropy_surfel_vector entropy_surfel_array;
    //priority queue as vector with min entropy surfels at the back
    shared_entropy_surfel_vector min_entropy_surfel_ptr_queue;

    //final surfels
    shared_entropy_surfel_vector finalized_surfels;

    // wrap all surfels of the input array to entropy_surfels and push them in the ESA
    for (size_t node_id = 0; node_id < input.size(); ++node_id) {
        for (size_t surfel_id = input[node_id]->offset();
             surfel_id < input[node_id]->offset() + input[node_id]->length();
             ++surfel_id) {

            //this surfel will be referenced in the entropy surfel
            auto current_surfel = input[node_id]->surfel_mem_data()->at(input[node_id]->offset() + surfel_id);

            // ignore outlier radii of any kind
            if (current_surfel.radius() == 0.0) {
                continue;
            }
            //create new entropy surfel
            entropy_surfel current_entropy_surfel(current_surfel, surfel_id, node_id);

            // only place where shared pointers should be created
            entropy_surfel_array.push_back(std::make_shared<entropy_surfel>(current_entropy_surfel));
        }
    }

    // iterate all wrapped surfels 
    for (auto &current_entropy_surfel_ptr : entropy_surfel_array) {

        shared_entropy_surfel_vector overlapping_neighbour_ptrs
            = get_locally_overlapping_neighbours(current_entropy_surfel_ptr, entropy_surfel_array);

        //assign/compute missing attributes
        current_entropy_surfel_ptr->neighbours = overlapping_neighbour_ptrs;
        update_entropy(current_entropy_surfel_ptr, overlapping_neighbour_ptrs);

        //if overlapping neighbours were found, put the entropy surfel back into the priority_queue
        if (!overlapping_neighbour_ptrs.empty()) {
            min_entropy_surfel_ptr_queue.push_back(current_entropy_surfel_ptr);
        }
        else { //otherwise, consider this surfel to be finalized
            //std::cout << "**Pushing into finalized surfels (size so far: "<<finalized_surfels.size() << "): " << (int)current_entropy_surfel_ptr->contained_surfel->color()[0] << "\n";
            finalized_surfels.push_back(current_entropy_surfel_ptr);
            //std::cout << "did not find any Neighbours!!!!\n";
            //std::cout << current_entropy_surfel_ptr->contained_surfel->pos() << " -- " << current_entropy_surfel_ptr->contained_surfel->radius() << " ------\n";
        }
    }

    // the queue was modified, therefore it is resorted
    std::sort(min_entropy_surfel_ptr_queue.begin(), min_entropy_surfel_ptr_queue.end(), min_entropy_order());

    size_t num_valid_surfels = min_entropy_surfel_ptr_queue.size() + finalized_surfels.size();


    while (true) {

        if (!min_entropy_surfel_ptr_queue.empty()) {
            shared_entropy_surfel current_entropy_surfel = min_entropy_surfel_ptr_queue.back();

            min_entropy_surfel_ptr_queue.pop_back();

            //the back element is still valid, so we still got something to do
            if (current_entropy_surfel->validity) {
                //std::cout << "Working on the surfel\n";
                // if merge returns true, the surfel still has neighbours


                if (merge(current_entropy_surfel, entropy_surfel_array, num_valid_surfels, surfels_per_node)) {
                    min_entropy_surfel_ptr_queue.push_back(current_entropy_surfel);
                    /*min_entropy_surfel_ptr_queue.insert(min_entropy_surfel_ptr_queue.end(),
                                                        finalized_surfels.begin(),
                                                        finalized_surfels.end());
                    */
                    //finalized_surfels.clear();

                }
                else { //otherwise we can push it directly into the finalized surfel list
                    finalized_surfels.push_back(current_entropy_surfel);
                }


                std::sort(min_entropy_surfel_ptr_queue.begin(), min_entropy_surfel_ptr_queue.end(), min_entropy_order());


                if (num_valid_surfels <= surfels_per_node) {
                    break;
                }
                else {
                    //break
                }
            }

        }
        else {
            break;
        }

    }


    // put valid surfels into final array

    //end of entropy simplification
    while ((!min_entropy_surfel_ptr_queue.empty())) {
        shared_entropy_surfel en_surfel_to_push = min_entropy_surfel_ptr_queue.back();

        if (en_surfel_to_push->validity == true) {
            finalized_surfels.push_back(en_surfel_to_push);
        }

        min_entropy_surfel_ptr_queue.pop_back();
    }


    std::sort(finalized_surfels.begin(), finalized_surfels.end(), min_entropy_order());

    while (num_valid_surfels > surfels_per_node) {
        auto const &min_entropy_surfel = finalized_surfels.back();

        if (min_entropy_surfel->validity) {
            --num_valid_surfels;
        }

        finalized_surfels.pop_back();
    }

    size_t chosen_surfels = 0;
    for (auto const &en_surf : finalized_surfels) {

        if (en_surf->validity) {
            if (chosen_surfels++ < surfels_per_node) {
                mem_array.surfel_mem_data()->push_back(*(en_surf->contained_surfel));
            }
            else {
                break;
            }
        }
    }

    mem_array.set_length(mem_array.surfel_mem_data()->size());
    reduction_error = 0.0;

    return mem_array;
};

void reduction_entropy::
add_neighbours(shared_entropy_surfel entropy_surfel_to_add_neighbours,
               shared_entropy_surfel_vector const &neighbour_ptrs_to_add) const
{

    entropy_surfel_to_add_neighbours->neighbours.insert(std::end(entropy_surfel_to_add_neighbours->neighbours),
                                                        std::begin(neighbour_ptrs_to_add),
                                                        std::end(neighbour_ptrs_to_add));
}

void reduction_entropy::
update_color(shared_surfel target_surfel,
             shared_entropy_surfel_vector const &neighbour_ptrs) const
{

    vec3r accumulated_color(0.0, 0.0, 0.0);
    double accumulated_weight = 0.0;

    accumulated_color = target_surfel->color();
    accumulated_weight = 1.0;

    for (auto const curr_neighbour_ptr : neighbour_ptrs) {
        accumulated_weight += 1.0;
        accumulated_color += curr_neighbour_ptr->contained_surfel->color();
    }

    vec3b normalized_color = vec3b(accumulated_color[0] / accumulated_weight,
                                   accumulated_color[1] / accumulated_weight,
                                   accumulated_color[2] / accumulated_weight);
    target_surfel->color() = normalized_color;
}

void reduction_entropy::
update_normal(shared_surfel target_surfel_ptr,
              shared_entropy_surfel_vector const neighbour_ptrs) const
{
    vec3f new_normal(0.0, 0.0, 0.0);

    real weight_sum = 0.f;

    new_normal = target_surfel_ptr->normal();
    weight_sum = 1.0;

    for (auto const neighbour_ptr : neighbour_ptrs) {
        shared_surfel target_surfel_ptr = neighbour_ptr->contained_surfel;

        real weight = target_surfel_ptr->radius();
        weight_sum += weight;

        new_normal += weight * target_surfel_ptr->normal();
    }

    if (weight_sum != 0.0) {
        new_normal /= weight_sum;
    }
    else {
        new_normal = vec3r(0.0, 0.0, 0.0);
    }

    target_surfel_ptr->normal() = scm::math::normalize(new_normal);
}

// to verify: the center of mass is the point that allows for the minimal enclosing sphere
vec3r reduction_entropy::
compute_center_of_mass(shared_surfel target_surfel_ptr,
                       shared_entropy_surfel_vector const &neighbour_ptrs) const
{

    //volume of a sphere (4/3) * pi * r^3
    real target_surfel_radius = target_surfel_ptr->radius();
    real rad_pow_3 = target_surfel_radius * target_surfel_radius * target_surfel_radius;
    real target_surfel_mass = (4.0 / 3.0) * M_PI * rad_pow_3;

    vec3r center_of_mass_enumerator = target_surfel_mass * target_surfel_ptr->pos();
    real center_of_mass_denominator = target_surfel_mass;

    //center of mass equation: c_o_m = ( sum_of( m_i*x_i) ) / ( sum_of(m_i) )
    for (auto const curr_neighbour_ptr : neighbour_ptrs) {

        shared_surfel current_neighbour_surfel = curr_neighbour_ptr->contained_surfel;

        real neighbour_radius = current_neighbour_surfel->radius();

        real neighbour_mass = (4.0 / 3.0) * M_PI *
            neighbour_radius * neighbour_radius * neighbour_radius;

        center_of_mass_enumerator += neighbour_mass * current_neighbour_surfel->pos();

        center_of_mass_denominator += neighbour_mass;
    }

    return center_of_mass_enumerator / center_of_mass_denominator;
}

real reduction_entropy::
compute_enclosing_sphere_radius(vec3r const &center_of_mass,
                                shared_surfel target_surfel_ptr,
                                std::vector<shared_entropy_surfel> const neighbour_ptrs) const
{

    real enclosing_radius = 0.0;

    enclosing_radius = scm::math::length(center_of_mass - target_surfel_ptr->pos()) + target_surfel_ptr->radius();

    for (auto const curr_neighbour_ptr : neighbour_ptrs) {

        shared_surfel current_neighbour_surfel = curr_neighbour_ptr->contained_surfel;
        real neighbour_enclosing_radius = scm::math::length(center_of_mass - current_neighbour_surfel->pos()) + current_neighbour_surfel->radius();

        if (neighbour_enclosing_radius > enclosing_radius) {
            enclosing_radius = neighbour_enclosing_radius;
        }
    }

    return enclosing_radius;
}

shared_entropy_surfel_vector const reduction_entropy::
get_locally_overlapping_neighbours(shared_entropy_surfel target_entropy_surfel_ptr,
                                   shared_entropy_surfel_vector &entropy_surfel_ptr_array
) const
{
    shared_surfel target_surfel = target_entropy_surfel_ptr->contained_surfel;

    std::vector<shared_entropy_surfel> overlapping_neighbour_ptrs;

    for (auto const array_entr_surfel_ptr : entropy_surfel_ptr_array) {

        // avoid overlaps with the surfel itself
        if (target_entropy_surfel_ptr->surfel_id != array_entr_surfel_ptr->surfel_id ||
            target_entropy_surfel_ptr->node_id != array_entr_surfel_ptr->node_id) {

            shared_surfel contained_surfel_ptr = array_entr_surfel_ptr->contained_surfel;

            if (surfel::intersect(*target_surfel, *contained_surfel_ptr)) {
                overlapping_neighbour_ptrs.push_back(array_entr_surfel_ptr);
            }

        }

    }

    return overlapping_neighbour_ptrs;
}

void reduction_entropy::
update_entropy(shared_entropy_surfel target_en_surfel,
               shared_entropy_surfel_vector const neighbour_ptrs) const
{
    // base entropy for surfel
    //double entropy = target_en_surfel->contained_surfel->radius();
    double entropy = 0.0;

    vec3b const &target_surfel_color = target_en_surfel->contained_surfel->color();

    size_t num_surfels_considered = 1;

    for (auto const curr_neighbour_ptr : neighbour_ptrs) {

        if (curr_neighbour_ptr->validity) {
            shared_surfel current_neighbour_surfel = curr_neighbour_ptr->contained_surfel;

            vec3f const &neighbour_normal = current_neighbour_surfel->normal();

            vec3b const &neighbour_color = current_neighbour_surfel->color();

            float normal_angle = std::fabs(scm::math::dot(target_en_surfel->contained_surfel->normal(), neighbour_normal));
            entropy += (1 + target_en_surfel->level) / (1.0 + normal_angle);
/*
            real color_diff = scm::math::length( vec3r((target_surfel_color[0] - neighbour_color[0]),
                                                       (target_surfel_color[1] - neighbour_color[1]),
                                                       (target_surfel_color[2] - neighbour_color[2]) ) ) ;
*/
            //entropy += (1 + target_en_surfel->level) * (1 + (color_diff));

            //entropy += current_neighbour_surfel->radius();

            ++num_surfels_considered;
        }
    };

    target_en_surfel->entropy = entropy / num_surfels_considered;
    //target_en_surfel->entropy = entropy;
}

void reduction_entropy::
update_entropy_surfel_level(shared_entropy_surfel target_en_surfel_ptr,
                            shared_entropy_surfel_vector const &invalidated_neighbours) const
{
    //target_en_surfel_ptr->level += 1;
    target_en_surfel_ptr->level += invalidated_neighbours.size() * 1000;
}

void reduction_entropy::
update_position(shared_surfel target_surfel_ptr,
                shared_entropy_surfel_vector const neighbour_ptrs) const
{
    target_surfel_ptr->pos() = compute_center_of_mass(target_surfel_ptr,
                                                      neighbour_ptrs);
}

void reduction_entropy::
update_radius(shared_surfel target_surfel_ptr,
              shared_entropy_surfel_vector const neighbour_ptrs) const
{
    target_surfel_ptr->radius()
        = compute_enclosing_sphere_radius(target_surfel_ptr->pos(),
                                          target_surfel_ptr,
                                          neighbour_ptrs);
}

void reduction_entropy::
update_surfel_attributes(shared_surfel target_surfel_ptr,
                         shared_entropy_surfel_vector const invalidated_neighbours) const
{

    update_normal(target_surfel_ptr, invalidated_neighbours);
    update_color(target_surfel_ptr, invalidated_neighbours);

    // position needs to be updated before the radius is updated
    update_position(target_surfel_ptr, invalidated_neighbours);
    update_radius(target_surfel_ptr, invalidated_neighbours);
}

bool reduction_entropy::
merge(shared_entropy_surfel target_entropy_surfel,
      shared_entropy_surfel_vector const &complete_entropy_surfel_array,
      size_t &num_remaining_valid_surfel, size_t num_desired_surfel) const
{

    size_t num_invalidated_surfels = 0;

    shared_entropy_surfel_vector neighbours_to_merge;

    //**replace own invalid neighbours by valid neighbours of invalid neighbours**
    std::map<size_t, std::set<size_t> > added_neighbours_during_merge;

    auto min_distance_ordering = [&target_entropy_surfel](shared_entropy_surfel const &left_entropy_surfel,
                                                          shared_entropy_surfel const &right_entropy_surfel)
    {


        double left_en_surfel_distance_measure =
            (target_entropy_surfel->contained_surfel->radius() +
                left_entropy_surfel->contained_surfel->radius()) -
                scm::math::length(target_entropy_surfel->contained_surfel->pos() -
                    left_entropy_surfel->contained_surfel->pos());

        double right_en_surfel_distance_measure =
            (target_entropy_surfel->contained_surfel->radius() +
                right_entropy_surfel->contained_surfel->radius()) -
                scm::math::length(target_entropy_surfel->contained_surfel->pos() -
                    right_entropy_surfel->contained_surfel->pos());

        if (
            left_en_surfel_distance_measure
                <
                    right_en_surfel_distance_measure
            ) {
            return true;
        }
        else {
            return false;
        }
    };

    //sort neighbours by increasing entropy to current neighbour
    std::sort(target_entropy_surfel->neighbours.begin(),
              target_entropy_surfel->neighbours.end(),
              min_distance_ordering);


    shared_entropy_surfel_vector invalidated_neighbours;

    for (auto const actual_neighbour_ptr : target_entropy_surfel->neighbours) {

        if (actual_neighbour_ptr->validity) {
            actual_neighbour_ptr->validity = false;

            invalidated_neighbours.push_back(actual_neighbour_ptr);

            ++num_invalidated_surfels;
            if (--num_remaining_valid_surfel == num_desired_surfel) {
                break;
            }

        }

    }

    for (auto const actual_neighbour_ptr : invalidated_neighbours) {

        //iterate the neighbours of the invalid neighbour
        for (auto const second_neighbour_ptr : actual_neighbour_ptr->neighbours) {

            // we only have to consider valid neighbours, all the others are also our own neighbours and already invalid
            if (second_neighbour_ptr->validity) {
                size_t n_id = second_neighbour_ptr->node_id;
                size_t s_id = second_neighbour_ptr->surfel_id;

                //avoid getting the surfel itself as neighbour
                if (n_id != target_entropy_surfel->node_id ||
                    s_id != target_entropy_surfel->surfel_id) {
                    //ignore 2nd neighbours which we found already at another neighbour
                    if ((added_neighbours_during_merge[n_id]).find(s_id) == added_neighbours_during_merge[n_id].end()) {
                        (added_neighbours_during_merge[n_id]).insert(s_id);
                        neighbours_to_merge.push_back(second_neighbour_ptr);
                    }
                }
            }
        }

    }

    add_neighbours(target_entropy_surfel, neighbours_to_merge);

    //recompute values for merged surfel
    //target_entropy_surfel->level += 1;
    update_entropy_surfel_level(target_entropy_surfel, invalidated_neighbours);
    update_surfel_attributes(target_entropy_surfel->contained_surfel, invalidated_neighbours);


    // now that we , we also have to look for neighbours that we suddenly overlap due to the higher radius
    shared_entropy_surfel_vector additional_surfels_to_test_for_overlap;


    for (auto const surfel_ptr : complete_entropy_surfel_array) {

        if (surfel_ptr->validity) {
            if (surfel_ptr->node_id != target_entropy_surfel->node_id ||
                surfel_ptr->surfel_id != target_entropy_surfel->surfel_id) {

                if (added_neighbours_during_merge[surfel_ptr->node_id].find(surfel_ptr->surfel_id) ==
                    added_neighbours_during_merge[surfel_ptr->node_id].end()) {
                    // we did not consider this surfel, we can check for an overlap.
                    additional_surfels_to_test_for_overlap.push_back(surfel_ptr);

                }

            }
        }
    }

    auto const additional_overlapping_neighbours
        = get_locally_overlapping_neighbours(target_entropy_surfel,
                                             additional_surfels_to_test_for_overlap);

    //if(additional_overlapping_neighbours.size() != 0)
    //    std::cout << "Found something!\n";

    add_neighbours(target_entropy_surfel, additional_overlapping_neighbours);


    update_entropy(target_entropy_surfel, target_entropy_surfel->neighbours);


    if (num_invalidated_surfels == 0)
        return false;
    if (!target_entropy_surfel->neighbours.empty()) {
        return true;
    }
    else {
        return false;
    }
}

} // namespace pre
} // namespace lamure

#endif // CMAKE_OPTION_ENABLE_ALTERNATIVE_STRATEGIES