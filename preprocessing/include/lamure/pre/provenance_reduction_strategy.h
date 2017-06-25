#ifndef LAMURE_PROVENANCE_REDUCTION_STRATEGY_H
#define LAMURE_PROVENANCE_REDUCTION_STRATEGY_H

#include <lamure/pre/bvh_node.h>
#include <lamure/pre/surfel_mem_array.h>
#include <sys/stat.h>

namespace lamure
{
namespace pre
{
class bvh;

class provenance_reduction_strategy : public reduction_strategy
{
  public:
    // TODO: inject a class / struct
    class LoDMetaData
    {
      public:
        float _mean_absolute_deviation;
        float _standard_deviation;
        float _coefficient_of_variation;
    };

    virtual ~provenance_reduction_strategy() {}

    surfel_mem_array create_lod(real &reduction_error, const std::vector<surfel_mem_array *> &input, const uint32_t surfels_per_node, const bvh &tree, const size_t start_node_id) const override
    {
        return surfel_mem_array(nullptr, 0, 0);
    }

    virtual surfel_mem_array create_lod(real &reduction_error, const std::vector<surfel_mem_array *> &input, std::vector<LoDMetaData> &deviations, const uint32_t surfels_per_node, const bvh &tree,
                                        const size_t start_node_id) const = 0;

    void interpolate_approx_natural_neighbours(surfel &surfel_to_update, std::vector<surfel> const &input_surfels, const bvh &tree, size_t const num_nearest_neighbours = 24) const;

    template <typename T>
    T swap(const T &arg, bool big_in_mem)
    {
        if((BYTE_ORDER == BIG_ENDIAN) == big_in_mem)
        {
            return arg;
        }
        else
        {
            T ret;

            char *dst = reinterpret_cast<char *>(&ret);
            const char *src = reinterpret_cast<const char *>(&arg + 1);

            for(size_t i = 0; i < sizeof(T); i++)
                *dst++ = *--src;

            return ret;
        }
    }

    void output_lod(std::vector<LoDMetaData> deviations, uint32_t node_index)
    {
        std::stringstream sstr;
        sstr << "lod_level_" << node_index;
        std::ofstream ofstream(sstr.str(), std::ios::out | std::ios::binary | std::ios::app);
        if(ofstream.is_open())
        {
            uint16_t magic_bytes = 0xAFFE;
            ofstream.write(reinterpret_cast<char *>(&magic_bytes), sizeof(magic_bytes));

            uint64_t length_of_data = deviations.size() * 0x0C;

            ofstream.write(reinterpret_cast<char *>(&length_of_data), sizeof(length_of_data));

            for(std::vector<LoDMetaData>::reverse_iterator rit = deviations.rbegin(); rit != deviations.rend(); ++rit)
            {
                //                printf("\nMAD: %e ", (*rit)._mean_absolute_deviation);
                //                printf("\nSTD: %e ", (*rit)._standard_deviation);
                //                printf("\nCoV: %e ", (*rit)._coefficient_of_variation);

                ofstream.write(reinterpret_cast<char *>(&(*rit)._mean_absolute_deviation), 4);
                ofstream.write(reinterpret_cast<char *>(&(*rit)._standard_deviation), 4);
                ofstream.write(reinterpret_cast<char *>(&(*rit)._coefficient_of_variation), 4);
            }
        }
        ofstream.close();
    }

    void pack_node(uint32_t node_index)
    {
        std::stringstream sstr;
        sstr << "lod_level_" << node_index;
        std::ifstream is(sstr.str(), std::ios::in | std::ios::binary);
        if(is.is_open())
        {
            is.ignore(0x02);

            uint64_t length_of_data;
            is.read(reinterpret_cast<char *>(&length_of_data), 8);

//            printf("\nNode data length: %lu ", length_of_data);

            std::streampos fsize = is.tellg();
            is.seekg(0, std::ios::end);
            fsize = is.tellg() - fsize;

//            printf("\nFile size length: %lu ", fsize);

            if(fsize != length_of_data)
            {
                std::stringstream istr;
                istr << fsize;
                std::stringstream dstr;
                dstr << length_of_data;
                throw std::out_of_range("Node data length not equal to declared: " + istr.str() + " instead of " + dstr.str());
            }

            is.clear();
            is.seekg(0x0A);

            std::vector<uint8_t> byte_buffer(length_of_data, 0);
            is.read(reinterpret_cast<char *>(&byte_buffer[0]), length_of_data);

//            printf("\nByte buffer length: %lu ", byte_buffer.size());

            //            is.clear();
            //            is.seekg(0x0A);
            //
            //            for(uint64_t index = 0; index < byte_buffer.size() / 24; index++)
            //            {
            //                double mad;
            //                double std;
            //                double cov;
            //
            //                is.read(reinterpret_cast<char *>(&mad), 8);
            //                is.read(reinterpret_cast<char *>(&std), 8);
            //                is.read(reinterpret_cast<char *>(&cov), 8);
            //
            //                printf("\nMAD: %e ", mad);
            //                printf("\nSTD: %e ", std);
            //                printf("\nCoV: %e ", cov);
            //
            //                std::cin.get();
            //            }

            is.close();

            std::ofstream os("lod.meta", std::ios::out | std::ios::binary | std::ios::app);

                if(os.is_open())
                {
                    // uint16_t magic_bytes = 0xAFFE;
                    // magic_bytes = swap(magic_bytes, true);
                    // os.write(reinterpret_cast<char *>(&magic_bytes), sizeof(magic_bytes));

                    // length_of_data = swap(length_of_data, true);
                    // os.write(reinterpret_cast<char *>(&length_of_data), sizeof(length_of_data));

                    os.write(reinterpret_cast<char *>(&byte_buffer[0]), length_of_data);
                }
                else
                {
                    throw std::ios_base::failure("Can not open the lod.meta file");
                }

                os.close();

            // struct stat buffer;
            // if((stat("lod.meta", &buffer) == 0))
            // {
            //     // file exists
            //     uint64_t _length_of_data = 0;


            //     std::ifstream hs("lod.meta", std::ios::in | std::ios::binary);
            //     if(hs.is_open())
            //     {
            //         hs.ignore(0x02);

            //         hs.read(reinterpret_cast<char *>(&_length_of_data), sizeof(_length_of_data));
            //         _length_of_data = swap(_length_of_data, true);

            //         std::vector<uint8_t> _byte_buffer(_length_of_data, 0);
            //         hs.read(reinterpret_cast<char *>(&_byte_buffer[0]), _length_of_data);

            //         hs.close();

            //         std::ofstream os("lod.meta", std::ios::out | std::ios::binary | std::ios::trunc);

            //         if(os.is_open())
            //         {
            //             uint16_t magic_bytes = 0xAFFE;
            //             magic_bytes = swap(magic_bytes, true);
            //             os.write(reinterpret_cast<char *>(&magic_bytes), sizeof(magic_bytes));

            //             uint64_t e_length = length_of_data + _length_of_data;
            //             e_length = swap(e_length, true);
            //             os.write(reinterpret_cast<char *>(&e_length), sizeof(e_length));

            //             os.write(reinterpret_cast<char *>(&_byte_buffer[0]), _length_of_data);
            //             os.write(reinterpret_cast<char *>(&byte_buffer[0]), length_of_data);
            //         }
            //         else
            //         {
            //             throw std::ios_base::failure("Can not open the lod.meta file");
            //         }

            //         os.close();
            //     }
            // }
            // else
            // {
            //     std::ofstream os("lod.meta", std::ios::out | std::ios::binary);

            //     if(os.is_open())
            //     {
            //         uint16_t magic_bytes = 0xAFFE;
            //         magic_bytes = swap(magic_bytes, true);
            //         os.write(reinterpret_cast<char *>(&magic_bytes), sizeof(magic_bytes));

            //         length_of_data = swap(length_of_data, true);
            //         os.write(reinterpret_cast<char *>(&length_of_data), sizeof(length_of_data));

            //         os.write(reinterpret_cast<char *>(&byte_buffer[0]), length_of_data);
            //     }
            //     else
            //     {
            //         throw std::ios_base::failure("Can not open the lod.meta file");
            //     }

            //     os.close();
            // }
        }
        else
        {
            throw std::ios_base::failure("Can not open the " + sstr.str() + " file");
        }

        std::remove(sstr.str().c_str());
    }
};

} // namespace pre
} // namespace lamure

#endif // LAMURE_PROVENANCE_REDUCTION_STRATEGY_H
