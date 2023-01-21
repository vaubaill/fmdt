#include "fmdt/tools.h"

#include "fmdt/CCL_LSL/CCL_LSL.hpp"

CCL_LSL::CCL_LSL(const int i0, const int i1, const int j0, const int j1, const int b, const enum ccl_impl_e impl)
: Module(), i0(i0), i1(i1), j0(j0), j1(j1), b(b), data(nullptr), in_img(nullptr) {
    const std::string name = "CCL_LSL";
    this->set_name(name);
    this->set_short_name(name);

    this->init_data(impl);

    const size_t i_socket_size = ((i1 - i0) + 1 + 2 * b) * ((j1 - j0) + 1 + 2 * b);

    auto &p = this->create_task("apply");
    auto ps_in_img = this->template create_socket_in<uint8_t>(p, "in_img", i_socket_size);
    auto ps_out_labels = this->template create_socket_out<uint32_t>(p, "out_labels", i_socket_size);
    auto ps_out_n_ROI = this->template create_socket_out<uint32_t>(p, "out_n_ROI", 1);

    this->create_codelet(p, [ps_in_img, ps_out_labels, ps_out_n_ROI]
                         (aff3ct::module::Module &m, aff3ct::runtime::Task &t, const size_t frame_id) -> int {
        auto &lsl = static_cast<CCL_LSL&>(m);
        const uint8_t* m_in_img = static_cast<const uint8_t*>(t[ps_in_img].get_dataptr());
        uint32_t* m_out_labels = static_cast<uint32_t*>(t[ps_out_labels].get_dataptr());

        tools_linear_2d_nrc_ui8matrix(m_in_img, lsl.i0 - lsl.b, lsl.i1 + lsl.b, lsl.j0 - lsl.b, lsl.j1 + lsl.b, 
                                      lsl.in_img);
        tools_linear_2d_nrc_ui32matrix((const uint32_t*)m_out_labels, lsl.i0 - lsl.b, lsl.i1 + lsl.b, lsl.j0 - lsl.b,
                                       lsl.j1 + lsl.b, (const uint32_t**)lsl.out_labels);

        uint32_t* m_out_n_ROI = static_cast<uint32_t*>(t[ps_out_n_ROI].get_dataptr());
        *m_out_n_ROI = CCL_apply(lsl.data, lsl.in_img, lsl.out_labels);
        return aff3ct::runtime::status_t::SUCCESS;
    });
}

void CCL_LSL::init_data(const enum ccl_impl_e impl) {
    this->data = CCL_alloc_and_init_data(impl, i0, i1, j0, j1);
    this->in_img = (const uint8_t**)malloc((size_t)(((i1 - i0) + 1 + 2 * b) * sizeof(const uint8_t*)));
    this->out_labels = (uint32_t**)malloc((size_t)(((i1 - i0) + 1 + 2 * b) * sizeof(uint32_t*)));
    this->in_img -= i0 - b;
    this->out_labels -= i0 - b;
}

CCL_LSL::~CCL_LSL() {
    free(this->in_img + (this->i0 - this->b));
    free(this->out_labels + (this->i0 - this->b));
    CCL_free_data(this->data);
}

CCL_LSL* CCL_LSL::clone() const {
    auto m = new CCL_LSL(*this);
    m->deep_copy(*this);
    return m;
}

void CCL_LSL::deep_copy(const CCL_LSL &m) {
    Module::deep_copy(m);
    this->init_data(this->data->impl);
}
