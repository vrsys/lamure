#ifndef LAMURE_IMAGE_H
#define LAMURE_IMAGE_H

#include <string>

using namespace std;

class Image {
private:

    int _height;
    int _width;
    string _file_name;

    // TODO: EXIF data

public:

    Image();

    Image(int _height, int _width, const string &_file_name);

    int get_height() const;

    void set_height(int _height);

    int get_width() const;

    void set_width(int _width);

    const string &get_file_name() const;

    void set_file_name(const string &_file_name);

    static Image read_from_file(const string &_file_name);
};

#endif //LAMURE_IMAGE_H
