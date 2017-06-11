#ifndef LAMURE_POINT_H
#define LAMURE_POINT_H

#include "MetaContainer.h"
#include "lamure/pro/common.h"

namespace prov
{
class Point : public MetaContainer
{
  public:
    Point(uint32_t _index, const vec3d &_position, const vec3d &_color, const vec<char> &_metadata) : MetaContainer(_metadata), _index(_index), _position(_position), _color(_color) {}
    Point() : MetaContainer()
    {
        _position = vec3d();
        _color = vec3d();
    }
    ~Point() {}
    uint32_t get_index() const { return _index; }
    const vec3d &get_position() const { return _position; }
    const vec3d &get_color() const { return _color; };
    ifstream &read_essentials(ifstream &is)
    {
        is.read(reinterpret_cast<char *>(&_index), 4);
        _index = swap(_index, true);

        if(DEBUG)
            printf("\nPoint index: %i", _index);

        is.read(reinterpret_cast<char *>(&_position[0]), 8);
        _position[0] = swap(_position[0], true);
        is.read(reinterpret_cast<char *>(&_position[1]), 8);
        _position[1] = swap(_position[1], true);
        is.read(reinterpret_cast<char *>(&_position[2]), 8);
        _position[2] = swap(_position[2], true);

        if(DEBUG)
            printf("\nPosition: %f %f %f", _position[0], _position[1], _position[2]);

        is.read(reinterpret_cast<char *>(&_color[0]), 8);
        _color[0] = swap(_color[0], true);
        is.read(reinterpret_cast<char *>(&_color[1]), 8);
        _color[1] = swap(_color[1], true);
        is.read(reinterpret_cast<char *>(&_color[2]), 8);
        _color[2] = swap(_color[2], true);

        if(DEBUG)
            printf("\nColor: %f %f %f", _color[0], _color[1], _color[2]);

        return is;
    }

  protected:
    uint32_t _index;
    vec3d _position;
    vec3d _color;
};
}

#endif // LAMURE_POINT_H