#ifndef LAMURE_PROVENANCE_REDUCTION_STRATEGY_H
#define LAMURE_PROVENANCE_REDUCTION_STRATEGY_H

#include <lamure/pre/bvh_node.h>
#include <lamure/pre/mem_array.h>
#include <sys/stat.h>

namespace lamure
{
namespace pre
{
class bvh;

class reduction_strategy_provenance : public reduction_strategy
{
  public:
    class LoDMetaData
    {
      public:
        float _mean_absolute_deviation;
        float _standard_deviation;
        float _coefficient_of_variation;
        float _debug_red;
        float _debug_green;
        float _debug_blue;
    };

    virtual ~reduction_strategy_provenance() {}

    surfel_mem_array create_lod(real &reduction_error, const std::vector<surfel_mem_array *> &input, const uint32_t surfels_per_node, const bvh &tree, const size_t start_node_id) const override
    {
        return surfel_mem_array(nullptr, 0, 0);
    }

    virtual surfel_mem_array create_lod(real &reduction_error, const std::vector<surfel_mem_array *> &input, std::vector<LoDMetaData> &deviations, const uint32_t surfels_per_node, const bvh &tree,
                                        const size_t start_node_id) const = 0;

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
        sstr << "node_" << node_index;
        std::ofstream ofstream(sstr.str(), std::ios::out | std::ios::binary | std::ios::app);
        if(ofstream.is_open())
        {
            uint64_t length_of_data = deviations.size() * 24;

            ofstream.write(reinterpret_cast<char *>(&length_of_data), 8);

            for(std::vector<LoDMetaData>::reverse_iterator rit = deviations.rbegin(); rit != deviations.rend(); ++rit)
            {
                ofstream.write(reinterpret_cast<char *>(&(*rit)._mean_absolute_deviation), 4);
                ofstream.write(reinterpret_cast<char *>(&(*rit)._standard_deviation), 4);
                ofstream.write(reinterpret_cast<char *>(&(*rit)._coefficient_of_variation), 4);
                ofstream.write(reinterpret_cast<char *>(&(*rit)._debug_red), 4);
                ofstream.write(reinterpret_cast<char *>(&(*rit)._debug_green), 4);
                ofstream.write(reinterpret_cast<char *>(&(*rit)._debug_blue), 4);
            }
        }
        ofstream.close();
    }

    void pack_node(uint32_t node_index, size_t max_size, size_t registered_size)
    {
        std::stringstream sstr;
        sstr << "node_" << node_index;
        std::ifstream is(sstr.str(), std::ios::in | std::ios::binary);
        if(is.is_open())
        {
            uint64_t length_of_data;
            is.read(reinterpret_cast<char *>(&length_of_data), 8);

            std::streampos fsize = is.tellg();
            is.seekg(0, std::ios::end);
            fsize = is.tellg() - fsize;

            if(fsize != length_of_data)
            {
                std::stringstream istr;
                istr << fsize;
                std::stringstream dstr;
                dstr << length_of_data;
                throw std::out_of_range("Node data length not equal to declared: " + istr.str() + " instead of " + dstr.str());
            }

            is.clear();
            is.seekg(0x08);

            std::vector<uint8_t> byte_buffer(length_of_data, 0);
            is.read(reinterpret_cast<char *>(&byte_buffer[0]), length_of_data);

            is.close();

            std::ofstream os("lod.meta", std::ios::out | std::ios::binary | std::ios::app);

            if(os.is_open())
            {
                    os.write(reinterpret_cast<char *>(&byte_buffer[0]), length_of_data);
            }
            else
            {
                throw std::ios_base::failure("Can not open the lod.meta file");
            }

            os.close();

        }
        else
        {
            throw std::ios_base::failure("Can not open the " + sstr.str() + " file");
        }

        std::remove(sstr.str().c_str());
    }

    void pack_empties(size_t surfel_count)
    {
        std::ofstream os("lod.meta", std::ios::out | std::ios::binary | std::ios::app);

        if(os.is_open())
        {
            for(uint64_t i = 0; i < surfel_count * 6; i++)
            {
                float empty;
                switch(i % 6)
                {
                case 3:
                    empty = 255;
                    break;
                case 4:
                    empty = 0;
                    break;
                case 5:
                    empty = 0;
                    break;
                default:
                    empty = -1;
                }

                os.write(reinterpret_cast<char *>(&empty), 4);
            }
        }
        else
        {
            throw std::ios_base::failure("Can not open the lod.meta file");
        }

        os.close();
    }
};

} // namespace pre
} // namespace lamure

#endif // LAMURE_PROVENANCE_REDUCTION_STRATEGY_H
