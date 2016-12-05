#ifndef _INCLUDED_DECODE_FORWARD_MM_BUFFER_HPP_
#define _INCLUDED_DECODE_FORWARD_MM_BUFFER_HPP_

#include <map>
#include <sdsl/int_vector.hpp>
#include <tudocomp/def.hpp>

namespace tdc {
namespace lzss {

class DecodeForwardMultimapBuffer {

private:
    std::vector<uliteral_t> m_buffer;
    std::unordered_multimap<len_t, len_t> m_fwd;
    sdsl::bit_vector m_decoded;

    len_t m_cursor;
    len_t m_longest_chain;

    inline void decode_literal_at(len_t pos, uliteral_t c) {
        m_buffer[pos] = c;
        m_decoded[pos] = 1;

        len_t chain = 0;
        for(auto it = m_fwd.find(pos); it != m_fwd.end(); it = m_fwd.find(pos)) {
            m_buffer[it->second] = c;
            m_decoded[it->second] = 1;

            m_fwd.erase(it);

            ++chain;
        }

        m_longest_chain = std::max(m_longest_chain, chain);
    }

public:
    inline DecodeForwardMultimapBuffer(len_t size)
        : m_cursor(0), m_longest_chain(0) {

        m_buffer.resize(size, 0);
        m_decoded = sdsl::bit_vector(size, 0);
    }

    inline void decode_literal(uliteral_t c) {
        decode_literal_at(m_cursor++, c);
    }

    inline void decode_factor(len_t pos, len_t num) {
        for(len_t i = 0; i < num; i++) {
            len_t src = pos+i;
            if(m_decoded[src]) {
                decode_literal_at(m_cursor, m_buffer[src]);
            } else {
                m_fwd.emplace(src, m_cursor);
            }

            ++m_cursor;
        }
    }

    inline len_t longest_chain() const {
        return m_longest_chain;
    }

    inline void write_to(std::ostream& out) {
        for(auto c : m_buffer) out << c;
    }
};

}} //ns

#endif