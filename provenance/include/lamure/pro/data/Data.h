#ifndef LAMURE_DATA_H
#define LAMURE_DATA_H

#include "Point.h"
#include "lamure/pro/common.h"

namespace prov
{
template <class TPoint>
class Data
{
  public:
    Data()
    {
        static_assert(std::is_base_of<Point, TPoint>::value, "The used point type is not a derivative of Point");
        _points = vec<TPoint>();
    }
    ~Data(){};

    ifstream &read_header(ifstream &is)
    {
        uint16_t magic_bytes;
        uint32_t crc_32;
        uint32_t data_length;
        is.read(reinterpret_cast<char *>(&magic_bytes), 2);
        magic_bytes = swap(magic_bytes, true);
        if(magic_bytes != 0xAFFE)
        {
            std::stringstream sstr;
            sstr << "0x" << std::uppercase << std::hex << magic_bytes;
            throw std::runtime_error("File format is incompatible: magic bytes " + sstr.str() + " not equal 0xAFFE");
        }
        is.read(reinterpret_cast<char *>(&crc_32), 4);
        is.read(reinterpret_cast<char *>(&data_length), 4);
        crc_32 = swap(crc_32, true);
        data_length = swap(data_length, true);

        std::streampos fsize = is.tellg();
        is.seekg(0, std::ios::end);
        fsize = is.tellg() - fsize;

        if(fsize != data_length)
        {
            std::stringstream istr;
            istr << fsize;
            std::stringstream dstr;
            dstr << data_length;
            throw std::out_of_range("Data length not equal to declared: " + istr.str() + " instead of " + dstr.str());
        }

        is.clear();
        is.seekg(10);

        std::vector<uint8_t> byte_buffer(data_length, 0);
        is.read(reinterpret_cast<char *>(&byte_buffer[0]), data_length);

        is.clear();
        is.seekg(10);

        boost::crc_32_type crc;
        crc.process_bytes(byte_buffer.data(), data_length);
        if(crc.checksum() != crc_32)
        {
            std::stringstream cstr;
            cstr << "0x" << std::uppercase << std::hex << crc.checksum();
            std::stringstream rstr;
            rstr << "0x" << std::uppercase << std::hex << crc_32;
            throw std::runtime_error("File is corrupted, crc32 checksums do not match: " + cstr.str() + " instead of " + rstr.str());
        }

        return is;
    }

    ifstream &read_points(ifstream &is)
    {
        uint32_t points_length;
        is.read(reinterpret_cast<char *>(&points_length), 4);

        points_length = swap(points_length, true);

        if(DEBUG)
            printf("\nPoints length: %i", points_length);

        for(int i = 0; i < points_length; i++)
        {
            TPoint point = TPoint();
            is >> point;
            _points.push_back(point);
        }

        return is;
    }

  protected:
    const vec<TPoint> &get_points() const { return _points; }

  protected:
    vec<TPoint> _points;
};
}

#endif // LAMURE_DATA_H