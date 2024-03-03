/*!
 * \file
 * \brief C++ wrapper for the features extraction.
 */

#pragma once

#include <stdint.h>
#include <aff3ct-core.hpp>

namespace ftr_ext {
    enum class tsk : size_t { extract, SIZE };
    namespace sck {
        enum class extract : size_t { in_labels, in_n_RoIs, out_RoIs_basic, status };
    }
}

class Features_extractor : public aff3ct::module::Module {
protected:
    const int i0, i1, j0, j1;
    const int b;
    const size_t max_RoIs_size;
public:
    Features_extractor(const int i0, const int i1, const int j0, const int j1, const int b, const size_t max_RoIs_size);
    virtual ~Features_extractor();
    virtual Features_extractor* clone() const;
    inline aff3ct::runtime::Task& operator[](const ftr_ext::tsk t);
    inline aff3ct::runtime::Socket& operator[](const ftr_ext::sck::extract s);
    inline aff3ct::runtime::Socket& operator[](const std::string &tsk_sck);
};

#include "fmdt/aff3ct_wrapper/Features/Features_extractor.hxx"
