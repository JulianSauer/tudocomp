#ifndef _INCLUDED_DECODE_FORWARD_BUFFER_HPP_
#define _INCLUDED_DECODE_FORWARD_BUFFER_HPP_

#include <vector>
#include <sdsl/int_vector.hpp>
#include <tudocomp/def.hpp>

namespace tdc {
namespace lzss {

class DecodeForwardBuffer {

private:
    std::vector<uliteral_t> m_buffer;
    std::vector<len_t> m_fwd;
    sdsl::bit_vector m_decoded;

    len_t m_cursor;

    inline void decode_literal_at(len_t pos, uliteral_t c) {
        len_t fwd;
        while(pos != LEN_MAX) {
            m_buffer[pos] = c;
            m_decoded[pos] = 1;

            fwd = m_fwd[pos];
            m_fwd[pos] = LEN_MAX;
            pos = fwd;
        }
    }

public:
    inline DecodeForwardBuffer(len_t size) : m_cursor(0) {
        m_buffer.resize(size, 0);
        m_fwd.resize(size, LEN_MAX);
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
                while(m_fwd[src] != LEN_MAX) {
                    src = m_fwd[src];
                }

                m_fwd[src] = m_cursor;
            }

            ++m_cursor;
        }
    }

    inline void write_to(std::ostream& out) {
        for(auto c : m_buffer) out << c;
    }
};

}} //ns

#endif