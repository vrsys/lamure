#include <iostream>
#include <algorithm>
#include "TextureAtlas.h"

void printHex(uint8_t data){
    cout << (int)(data >> 4) << (int)(data & 0xf);
}

void printHex(uint8_t *data, size_t len, size_t chars_per_line = 0){
    ios::fmtflags f(cout.flags());

    cout << hex << noshowbase;

    for(size_t i = 0; i < len; ++i){
        printHex(data[i]);

        if(chars_per_line != 0 && ((i + 1) % chars_per_line) == 0){
            cout << endl;
        }
    }

    cout.flags(f);
}

int main() {
    TextureAtlas atlas("image.data", 256, TextureAtlas::COMPRESSION::DUMMY);

    atlas.start();

    uint8_t *ptr = nullptr;
    size_t indices[128];

    for(size_t i = 0; i < 128; ++i) {
        indices[i] = i;
    }

    random_shuffle(indices, &indices[128]);

    size_t ctr = 0;

    for(size_t i = 0; i < 128; ++i){
        ptr = atlas.getPageById(indices[i]);

        if(ptr == nullptr){
            cout << "Cache miss: " << indices[i] << endl;
        }else{
            cout << "Cache hit: " << indices[i] << endl;
        }
    }

    this_thread::sleep_for(chrono::milliseconds(100));

    for(size_t i = 0; i < 128; ++i){
        ptr = atlas.getPageById(i);

        if(ptr == nullptr){
            cout << "Cache miss: " << i << endl;
        }else{
            cout << "Cache hit: " << i << endl;
            ++ctr;
            //printHex(ptr, 256, 16);
        }
    }

    cout << "Total: " << ctr << " Cache hits." << endl;

    return 0;
}