#include <iostream>
#include <algorithm>
#include <lamure/vt/TextureAtlas.h>

using namespace std;

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
  /*ofstream file("/home/towe2387/Desktop/image.data");

  for(size_t i = 0; i < 128; ++i){
    for(size_t n = 0; n < (256 * 256 * 4); ++n){
      file << (char)i;
    }
  }

  file.close();*/


  TextureAtlas atlas("/home/towe2387/Desktop/image.data", 256 * 256 * 4, TextureAtlas::COMPRESSION::DUMMY);

  atlas.start();

  uint8_t *ptr = nullptr;
  size_t indices[16];

  for(size_t i = 0; i < 16; ++i) {
    indices[i] = i;
  }

  random_shuffle(indices, &indices[16]);

  clock_t start;

  time(&start);

  cout << "Start requesting: " << start  << endl;

  size_t ctr = 0;

  for(size_t i = 0; i < 16; ++i){
    ptr = atlas.getPageById(indices[i]);

    if(ptr == nullptr){
      cout << "Cache miss: " << indices[i] << endl;
    }else{
      cout << "Cache hit: " << indices[i] << endl;
    }
  }

  clock_t end;
  time(&end);

  cout << "Done requesting: " << end  << endl;

  this_thread::sleep_for(chrono::milliseconds(1));

  time(&end);

  cout << "Start requesting: " << end  << endl;

  for(size_t i = 0; i < 16; ++i){
    ptr = atlas.getPageById(i);

    if(ptr == nullptr){
      cout << "Cache miss: " << i << endl;
    }else{
      cout << "Cache hit: " << i << endl;
      ++ctr;
      //printHex(ptr, 256 * 256 * 4, 128);
    }
  }

  time(&end);

  cout << "Done requesting: " << end  << endl;

  cout << "Total: " << ctr << " Cache hits in " << ((end - start) * 1000 / CLOCKS_PER_SEC) << " ms.";

  return 0;
}