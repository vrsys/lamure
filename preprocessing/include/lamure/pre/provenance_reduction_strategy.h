#ifndef LAMURE_PROVENANCE_REDUCTION_STRATEGY_H
#define LAMURE_PROVENANCE_REDUCTION_STRATEGY_H

#include <lamure/pre/bvh_node.h>
#include <lamure/pre/surfel_mem_array.h>

namespace lamure
{
namespace pre
{
class bvh;

class provenance_reduction_strategy : public reduction_strategy
{
  public:
    // TODO: inject a class / struct
    struct LoDMetaData
    {
        double _mean_absolute_deviation;
        double _standard_deviation;
        double _coefficient_of_variation;
    };

    virtual ~provenance_reduction_strategy() {}

    surfel_mem_array create_lod(real &reduction_error, const std::vector<surfel_mem_array *> &input, const uint32_t surfels_per_node, const bvh &tree, const size_t start_node_id) const override
    {
        return surfel_mem_array(nullptr, 0, 0);
    }

    virtual surfel_mem_array create_lod(real &reduction_error, const std::vector<surfel_mem_array *> &input, std::vector<LoDMetaData> &deviations, const uint32_t surfels_per_node,
                                        const bvh &tree, const size_t start_node_id) const = 0;

    void interpolate_approx_natural_neighbours(surfel &surfel_to_update, std::vector<surfel> const &input_surfels, const bvh &tree, size_t const num_nearest_neighbours = 24) const;

    void output_lod(std::vector<LoDMetaData> deviations, uint16_t level)
    {
        std::stringstream sstr;
        sstr << "lod_level_" << level;
        std::ofstream ofstream(sstr.str(), std::ios::out | std::ios::binary | std::ios::app);
        if(ofstream.is_open())
        {
            ofstream << std::hex << 0xAFFE;

            for(LoDMetaData data : deviations)
            {
                ofstream.write(reinterpret_cast<char *>(&data._mean_absolute_deviation), sizeof(data._mean_absolute_deviation));
                ofstream.write(reinterpret_cast<char *>(&data._standard_deviation), sizeof(data._standard_deviation));
                ofstream.write(reinterpret_cast<char *>(&data._coefficient_of_variation), sizeof(data._coefficient_of_variation));
            }
        }
        ofstream.close();
    }
};

} // namespace pre
} // namespace lamure

#endif // LAMURE_PROVENANCE_REDUCTION_STRATEGY_H
