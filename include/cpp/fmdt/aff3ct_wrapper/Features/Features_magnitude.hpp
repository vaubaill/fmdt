/*!
 * \file
 * \brief C++ wrapper to compute the magnitudes.
 */

#pragma once

#include <stdint.h>
#include <aff3ct-core.hpp>

namespace ftr_mgn {
    enum class tsk : size_t { compute, SIZE };
    namespace sck {
        enum class compute : size_t { in_img, in_labels, in_RoIs_basic, in_n_RoIs, out_RoIs_magn, status };
    }
}

class Features_magnitude : public aff3ct::module::Module {
protected:
    const int i0, i1, j0, j1;
    const int b;
    const size_t max_RoIs_size;
public:
    Features_magnitude(const int i0, const int i1, const int j0, const int j1, const int b, const size_t max_RoIs_size);
    virtual ~Features_magnitude();
    virtual Features_magnitude* clone() const;
    inline aff3ct::runtime::Task& operator[](const ftr_mgn::tsk t);
    inline aff3ct::runtime::Socket& operator[](const ftr_mgn::sck::compute s);
    inline aff3ct::runtime::Socket& operator[](const std::string &tsk_sck);
};

#include "fmdt/aff3ct_wrapper/Features/Features_magnitude.hxx"
