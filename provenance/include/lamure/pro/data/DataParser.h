#ifndef LAMURE_DATA_PARSER_H
#define LAMURE_DATA_PARSER_H

#include "Data.h"
#include "lamure/pro/common.h"

namespace prov
{
class DataParser
{
  public:
    DataParser(string &fname);
    ~DataParser();
    u_ptr<Data> &parse();

  protected:
    u_ptr<Data> _data;

  private:
    class impl;
    u_ptr<impl> pimpl;
};
}

#endif // LAMURE_DATA_PARSER_H
