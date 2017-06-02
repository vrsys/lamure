#include "lamure/pro/data/MetaContainer.h"

namespace prov
{
const vec<char> &prov::MetaContainer::get_metadata() const { return _metadata; }
MetaContainer::MetaContainer(const prov::vec<char> &_metadata) : _metadata(_metadata) {}
MetaContainer::~MetaContainer() {}
}