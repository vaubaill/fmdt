#pragma once

#include "fmdt/aff3ct_wrapper/Features/Features_merger_CCL_HI_v2.hpp"

uint32_t** Features_merger_CCL_HI_v2::get_out_labels() {
    return this->out_labels;
}

aff3ct::runtime::Task& Features_merger_CCL_HI_v2::operator[](const ftr_mrg2::tsk t) {
    return aff3ct::module::Module::operator[]((size_t)t);
}

aff3ct::runtime::Socket& Features_merger_CCL_HI_v2::operator[](const ftr_mrg2::sck::merge s) {
    return aff3ct::module::Module::operator[]((size_t)ftr_mrg2::tsk::merge)[(size_t)s];
}

aff3ct::runtime::Socket& Features_merger_CCL_HI_v2::operator[](const std::string &tsk_sck) {
    return aff3ct::module::Module::operator[](tsk_sck);
}
