#ifndef _INCLUDED_LZSS_LCP_COMPRESSOR_HPP_
#define _INCLUDED_LZSS_LCP_COMPRESSOR_HPP_

#include <algorithm>
#include <functional>
#include <vector>

#include <tudocomp/Compressor.hpp>
#include <tudocomp/Range.hpp>
#include <tudocomp/util.hpp>

#include <tudocomp/compressors/lzss/LZSSFactors.hpp>
#include <tudocomp/compressors/lzss/LZSSLiterals.hpp>
#include <tudocomp/compressors/lzss/LZSSCoding.hpp>

#include <tudocomp/ds/TextDS.hpp>

namespace tdc {

/// Computes the LZ77 factorization of the input using its suffix array and
/// LCP table.
template<typename coder_t, typename len_t = uint32_t>
class LZSSLCPCompressor : public Compressor {

private:
    const TypeRange<len_t> len_r = TypeRange<len_t>();

    typedef TextDS<> text_t;

public:
    inline static Meta meta() {
        Meta m("compressor", "lzss_lcp", "LZSS Factorization using LCP");
        m.option("coder").templated<coder_t>();
        return m;
    }

    /// Default constructor (not supported).
    inline LZSSLCPCompressor() = delete;

    /// Construct the class with an environment.
    inline LZSSLCPCompressor(Env&& env) : Compressor(std::move(env)) {
    }

    inline virtual void compress(Input& input, Output& output) override {
        auto view = input.as_view();
		view.ensure_null_terminator();

        // Construct text data structures
        env().begin_stat_phase("Construct SA, ISA and LCP");
        text_t text(view, text_t::SA | text_t::ISA | text_t::LCP);
        env().end_stat_phase();

        auto& sa = text.require_sa();
        auto& isa = text.require_isa();
        auto& lcp = text.require_lcp();

        // Factorize
        const len_t text_length = text.size();
        lzss::FactorBuffer factors;

        env().begin_stat_phase("Factorize");
        len_t fact_min = 3; //factor threshold

        for(size_t i = 0; i+1 < text_length;) { // we omit T[text_length-1] since we assume that it is the \0 byte!
            //get SA position for suffix i
            const size_t& cur_pos = isa[i];
			DCHECK_NE(cur_pos,0); // isa[i] == 0 <=> T[i] = 0

			//compute naively PSV
            //search "upwards" in LCP array
            //include current, exclude last
            size_t psv_lcp = lcp[cur_pos];
            ssize_t psv_pos = cur_pos - 1;
            if (psv_lcp > 0) {
                while (psv_pos >= 0 && sa[psv_pos] > sa[cur_pos]) {
                    psv_lcp = std::min<size_t>(psv_lcp, lcp[psv_pos--]);
                }
            }

			//compute naively NSV, TODO: use NSV data structure
            //search "downwards" in LCP array
            //exclude current, include last
            size_t nsv_lcp = 0;
            size_t nsv_pos = cur_pos + 1;
            if (nsv_pos < text_length) {
                nsv_lcp = SSIZE_MAX;
                do {
                    nsv_lcp = std::min<size_t>(nsv_lcp, lcp[nsv_pos]);
                    if (sa[nsv_pos] < sa[cur_pos]) {
                        break;
                    }
                } while (++nsv_pos < text_length);

                if (nsv_pos >= text_length) {
                    nsv_lcp = 0;
                }
            }

            //select maximum
            const size_t& max_lcp = std::max(psv_lcp, nsv_lcp);
            if(max_lcp >= fact_min) {
				const ssize_t& max_pos = max_lcp == psv_lcp ? psv_pos : nsv_pos;
				DCHECK_LT(max_pos, text_length);
				DCHECK_GE(max_pos, 0);
                // new factor
                factors.push_back(lzss::Factor(i, sa[max_pos], max_lcp));

                i += max_lcp; //advance
            } else {
                ++i; //advance
            }
        }

        env().log_stat("threshold", fact_min);
        env().log_stat("factors", factors.size());
        env().end_stat_phase();

        // encode
        typename coder_t::Encoder coder(env().env_for_option("coder"),
            output, lzss::TextLiterals<text_t>(text, factors));

        lzss::encode_text(coder, text, factors,
            view.is_terminal_null_ensured()); //TODO is this correct?
    }

    inline virtual void decompress(Input& input, Output& output) override {
        typename coder_t::Decoder decoder(env().env_for_option("coder"), input);
        auto outs = output.as_stream();

        lzss::decode_text(decoder, outs);
    }
};

}

#endif