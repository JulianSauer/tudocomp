#ifndef _INCLUDED_ASCII_CODER_HPP_
#define _INCLUDED_ASCII_CODER_HPP_

#include <sstream>
#include <tudocomp/Coder.hpp>

namespace tdc {

class ASCIICoder : public Algorithm {
public:
    inline static Meta meta() {
        Meta m("coder", "ascii", "Simple ASCII encoding");
        return m;
    }

    ASCIICoder() = delete;

    class Encoder : public tdc::Encoder {
    public:
        inline Encoder(Env&& env, Output& out) : tdc::Encoder(std::move(env), out) {}

        template<typename value_t, typename range_t>
        inline void encode(value_t v, const range_t& r) {
            std::ostringstream s;
            s << v;
            for(uint8_t c : s.str()) m_out->write_int(c);
            m_out->write_int(':');
        }
    };

    class Decoder : public tdc::Decoder {
    public:
        inline Decoder(Env&& env, Input& in) : tdc::Decoder(std::move(env), in) {}

        template<typename value_t, typename range_t>
        inline value_t decode(const range_t& r) {
            std::ostringstream os;
            for(uint8_t c = m_in->read_int<uint8_t>();
                c != ':';
                c = m_in->read_int<uint8_t>()) {

                os << c;
            }

            std::string s = os.str();

            value_t v;
            std::istringstream is(s);
            is >> v;
            return v;
        }
    };
};

template<>
inline void ASCIICoder::Encoder::encode<uint8_t, CharRange>(uint8_t v, const CharRange& r) {
    m_out->write_int(uint8_t(v));
}

template<>
inline void ASCIICoder::Encoder::encode<bool, BitRange>(bool v, const BitRange& r) {
    m_out->write_int(v ? '1' : '0');
}

template<>
inline uint8_t ASCIICoder::Decoder::decode<uint8_t, CharRange>(const CharRange& r) {
    return m_in->read_int<uint8_t>();
}

template<>
inline bool ASCIICoder::Decoder::decode<bool, BitRange>(const BitRange& r) {
    uint8_t b = m_in->read_int<uint8_t>();
    return (b != '0');
}

}

#endif
