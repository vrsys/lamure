#include <lamure/vt/ooc/TileRequest.h>

namespace vt{
    namespace ooc{
        TileRequest::TileRequest() : PriorityHeapContent<uint32_t>(), Observable() {
            _resource = nullptr;
        }

        void TileRequest::setResource(pre::AtlasFile *resource){
            _resource = resource;
        }

        pre::AtlasFile *TileRequest::getResource(){
            return _resource;
        }

        void TileRequest::setId(uint64_t id){
            _id = id;
        }

        uint64_t TileRequest::getId(){
            return _id;
        }

        void TileRequest::erase(){
            this->inform(0);
        }

        void TileRequest::abort(){
            _aborted = true;
        }

        bool TileRequest::isAborted(){
            return _aborted;
        }
    }
}