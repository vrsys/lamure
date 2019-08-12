
#include <lamure/ren/data_provenance.h>

namespace lamure {
namespace ren {

std::mutex data_provenance::mutex_;
bool data_provenance::is_instanced_ = false;
data_provenance* data_provenance::single_ = nullptr;

data_provenance::
data_provenance()
: _size_in_bytes(0) {

}

data_provenance::
~data_provenance() {
    std::lock_guard<std::mutex> lock(mutex_);
    is_instanced_ = false;
}

data_provenance* data_provenance::
get_instance() {
    if (!is_instanced_) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!is_instanced_) {
            single_ = new data_provenance();
            is_instanced_ = true;
        }

        return single_;
    }
    else {
        return single_;
    }
}



}
}