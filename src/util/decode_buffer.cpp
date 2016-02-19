#include <tudocomp/io.h>

namespace tudocomp {

void DecodeBuffer::decodeCallback(size_t source) {
    if (pointers.count(source) > 0) {
        std::vector<size_t> list = pointers[source];
        for (size_t target : list) {
            // decode this char
            text.at(target) = text.at(source);
            byte_is_decoded.at(target) = true;

            // decode all chars depending on this location
            decodeCallback(target);
        }

        pointers.erase(source);
    }
}

}