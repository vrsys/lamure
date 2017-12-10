#include <algorithm>
#include <fstream>
#include <iostream>
#include <lamure/vt/ooc/TileAtlas.h>

using namespace std;

void printHex(uint8_t data) { cout << (int)(data >> 4) << (int)(data & 0xf); }

void printHex(uint8_t *data, size_t len, size_t chars_per_line = 0)
{
    ios::fmtflags f(cout.flags());

    cout << hex << noshowbase;

    for(size_t i = 0; i < len; ++i)
    {
        printHex(data[i]);

        if(chars_per_line != 0 && ((i + 1) % chars_per_line) == 0)
        {
            cout << endl;
        }
    }

    cout.flags(f);
}

int main()
{
    // ofstream file("/home/towe2387/Desktop/image.data");

    /*for(size_t i = 0; i < 128; ++i){
    for(size_t n = 0; n < (256 * 256 * 4); ++n){
      file << (char)i;
    }
    }

    file.close();*/

    string fname = string("/mnt/terabytes_of_textures/montblanc/montblanc_w1202116_h304384.data");

    vt::TileAtlas<uint32_t> atlas(fname, 256 * 256 * 4);

    // atlas.start();

    uint8_t *ptr = nullptr;
    size_t indices[16];

    for(size_t i = 0; i < 16; ++i)
    {
        indices[i] = i;
    }

    random_shuffle(indices, &indices[16]);

    clock_t start;

    start = clock();

    cout << "Start requesting: " << start << endl;

    size_t ctr = 0;

    for(size_t i = 0; i < 16; ++i)
    {
        ptr = atlas.get(indices[i], 100);

        if(ptr == nullptr)
        {
            cout << "Cache miss: " << indices[i] << endl;
        }
        else
        {
            cout << "Cache hit: " << indices[i] << endl;
        }
    }

    clock_t end;
    end = clock();

    cout << "Done requesting: " << end << endl;

    atlas.wait();

    end = clock();

    cout << "Start requesting: " << end << endl;

    for(size_t i = 0; i < 16; ++i)
    {
        ptr = atlas.get(i, 100);

        if(ptr == nullptr)
        {
            cout << "Cache miss: " << i << endl;
        }
        else
        {
            cout << "Cache hit: " << i << endl;
            ++ctr;
            // printHex(ptr, 256 * 256 * 4, 128);
        }
    }

    end = clock();

    cout << "Done requesting: " << end << endl;

    cout << "Total: " << ctr << " Cache hits in " << ((end - start) * 1000 / CLOCKS_PER_SEC) << " ms.";

    return 0;
}