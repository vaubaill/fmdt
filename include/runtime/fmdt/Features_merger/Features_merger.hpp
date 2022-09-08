#pragma once

#include <stdint.h>
#include <aff3ct.hpp>

#include "fmdt/features.h"

namespace ftr_mrg {
    enum class tsk : size_t { merge, SIZE };
    namespace sck {
        enum class merge : size_t { in_img1, in_img2, in_ROI_id, in_ROI_xmin, in_ROI_xmax, in_ROI_ymin,
                                    in_ROI_ymax, in_ROI_S, in_ROI_Sx, in_ROI_Sy, in_ROI_x, in_ROI_y, in_n_ROI,
                                    out_ROI_id, out_ROI_xmin, out_ROI_xmax, out_ROI_ymin, out_ROI_ymax, out_ROI_S,
                                    out_ROI_Sx, out_ROI_Sy, out_ROI_x, out_ROI_y, out_n_ROI, out_img, status };
    }
}

class Features_merger : public aff3ct::module::Module {
protected:
    const int i0, i1, j0, j1;
    const int b;
    const int S_min, S_max;
    const size_t max_ROI_size;
    const uint32_t** in_img1;
    const uint8_t** in_img2;
    uint8_t** out_img;
public:
    Features_merger(const int i0, const int i1, const int j0, const int j1, const int b, const uint32_t S_min,
                    const uint32_t S_max, const size_t max_ROI_size)
    : Module(), i0(i0), i1(i1), j0(j0), j1(j1), b(b), S_min(S_min), S_max(S_max), max_ROI_size(max_ROI_size),
      in_img1(nullptr), in_img2(nullptr), out_img(nullptr)  {
        const std::string name = "Features_merger";
        this->set_name(name);
        this->set_short_name(name);

        this->init_data();

        auto socket_img_size = ((i1 - i0) + 1 + 2 * b) * ((j1 - j0) + 1 + 2 * b);

        auto &p = this->create_task("merge");
        auto ps_in_img1 = this->template create_socket_in<uint32_t>(p, "in_img1", socket_img_size);
        auto ps_in_img2 = this->template create_socket_in<uint8_t>(p, "in_img2", socket_img_size);

        auto ps_in_ROI_id = this->template create_socket_in<uint16_t>(p, "in_ROI_id", max_ROI_size);
        auto ps_in_ROI_xmin = this->template create_socket_in<uint16_t>(p, "in_ROI_xmin", max_ROI_size);
        auto ps_in_ROI_xmax = this->template create_socket_in<uint16_t>(p, "in_ROI_xmax", max_ROI_size);
        auto ps_in_ROI_ymin = this->template create_socket_in<uint16_t>(p, "in_ROI_ymin", max_ROI_size);
        auto ps_in_ROI_ymax = this->template create_socket_in<uint16_t>(p, "in_ROI_ymax", max_ROI_size);
        auto ps_in_ROI_S = this->template create_socket_in<uint32_t>(p, "in_ROI_S", max_ROI_size);
        auto ps_in_ROI_Sx = this->template create_socket_in<uint32_t>(p, "in_ROI_Sx", max_ROI_size);
        auto ps_in_ROI_Sy = this->template create_socket_in<uint32_t>(p, "in_ROI_Sy", max_ROI_size);
        auto ps_in_ROI_x = this->template create_socket_in<float>(p, "in_ROI_x", max_ROI_size);
        auto ps_in_ROI_y = this->template create_socket_in<float>(p, "in_ROI_y", max_ROI_size);
        auto ps_in_n_ROI = this->template create_socket_in<uint32_t>(p, "in_n_ROI", 1);

        auto ps_out_ROI_id = this->template create_socket_out<uint16_t>(p, "out_ROI_id", max_ROI_size);
        auto ps_out_ROI_xmin = this->template create_socket_out<uint16_t>(p, "out_ROI_xmin", max_ROI_size);
        auto ps_out_ROI_xmax = this->template create_socket_out<uint16_t>(p, "out_ROI_xmax", max_ROI_size);
        auto ps_out_ROI_ymin = this->template create_socket_out<uint16_t>(p, "out_ROI_ymin", max_ROI_size);
        auto ps_out_ROI_ymax = this->template create_socket_out<uint16_t>(p, "out_ROI_ymax", max_ROI_size);
        auto ps_out_ROI_S = this->template create_socket_out<uint32_t>(p, "out_ROI_S", max_ROI_size);
        auto ps_out_ROI_Sx = this->template create_socket_out<uint32_t>(p, "out_ROI_Sx", max_ROI_size);
        auto ps_out_ROI_Sy = this->template create_socket_out<uint32_t>(p, "out_ROI_Sy", max_ROI_size);
        auto ps_out_ROI_x = this->template create_socket_out<float>(p, "out_ROI_x", max_ROI_size);
        auto ps_out_ROI_y = this->template create_socket_out<float>(p, "out_ROI_y", max_ROI_size);
        auto ps_out_n_ROI = this->template create_socket_out<uint32_t>(p, "out_n_ROI", 1);

        auto ps_out_img = this->template create_socket_out<uint8_t>(p, "out_img", socket_img_size);

        this->create_codelet(p, [ps_in_img1, ps_in_img2, ps_in_ROI_id, ps_in_ROI_xmin, ps_in_ROI_xmax, ps_in_ROI_ymin,
                                 ps_in_ROI_ymax, ps_in_ROI_S, ps_in_ROI_Sx, ps_in_ROI_Sy, ps_in_ROI_x, ps_in_ROI_y,
                                 ps_in_n_ROI, ps_out_ROI_id, ps_out_ROI_xmin, ps_out_ROI_xmax, ps_out_ROI_ymin,
                                 ps_out_ROI_ymax, ps_out_ROI_S, ps_out_ROI_Sx, ps_out_ROI_Sy, ps_out_ROI_x,
                                 ps_out_ROI_y, ps_out_n_ROI, ps_out_img]
                             (aff3ct::module::Module &m, aff3ct::module::Task &t, const size_t frame_id) -> int {
            auto &mrg = static_cast<Features_merger&>(m);
            const uint32_t* m_in_img1 = static_cast<const uint32_t*>(t[ps_in_img1].get_dataptr());
            const uint8_t* m_in_img2 = static_cast<const uint8_t*>(t[ps_in_img2].get_dataptr());
            uint8_t* m_out_img = static_cast<uint8_t*>(t[ps_out_img].get_dataptr());
            mrg.in_img1[mrg.i0 - mrg.b] = m_in_img1 - (mrg.j0 - mrg.b);
            mrg.in_img2[mrg.i0 - mrg.b] = m_in_img2 - (mrg.j0 - mrg.b);
            mrg.out_img[mrg.i0 - mrg.b] = m_out_img - (mrg.j0 - mrg.b);
            for (int i = mrg.i0 - mrg.b + 1; i <= mrg.i1 + mrg.b; i++) {
                mrg.in_img1[i] = mrg.in_img1[i - 1] + ((mrg.j1 - mrg.j0) + 1 + 2 * mrg.b);
                mrg.in_img2[i] = mrg.in_img2[i - 1] + ((mrg.j1 - mrg.j0) + 1 + 2 * mrg.b);
                mrg.out_img[i] = mrg.out_img[i - 1] + ((mrg.j1 - mrg.j0) + 1 + 2 * mrg.b);
            }

            const uint32_t in_n_ROI = *static_cast<const uint32_t*>(t[ps_in_n_ROI].get_dataptr());

            std::copy_n(static_cast<const uint32_t*>(t[ps_in_ROI_S].get_dataptr()), in_n_ROI,
                        static_cast<uint32_t*>(t[ps_out_ROI_S].get_dataptr()));

            _features_merge_HI_CCL_v2(mrg.in_img1, mrg.in_img2, mrg.out_img, mrg.i0, mrg.i1, mrg.j0, mrg.j1,
                                      static_cast<const uint16_t*>(t[ps_in_ROI_id].get_dataptr()),
                                      static_cast<const uint16_t*>(t[ps_in_ROI_xmin].get_dataptr()),
                                      static_cast<const uint16_t*>(t[ps_in_ROI_xmax].get_dataptr()),
                                      static_cast<const uint16_t*>(t[ps_in_ROI_ymin].get_dataptr()),
                                      static_cast<const uint16_t*>(t[ps_in_ROI_ymax].get_dataptr()),
                                      static_cast<uint32_t*>(t[ps_out_ROI_S].get_dataptr()),
                                      in_n_ROI,
                                      mrg.S_min, mrg.S_max);

            size_t out_n_ROI = _features_shrink_ROI_array(static_cast<const uint16_t*>(t[ps_in_ROI_id].get_dataptr()),
                                                          static_cast<const uint16_t*>(t[ps_in_ROI_xmin].get_dataptr()),
                                                          static_cast<const uint16_t*>(t[ps_in_ROI_xmax].get_dataptr()),
                                                          static_cast<const uint16_t*>(t[ps_in_ROI_ymin].get_dataptr()),
                                                          static_cast<const uint16_t*>(t[ps_in_ROI_ymax].get_dataptr()),
                                                          static_cast<const uint32_t*>(t[ps_out_ROI_S].get_dataptr()),
                                                          static_cast<const uint32_t*>(t[ps_in_ROI_Sx].get_dataptr()),
                                                          static_cast<const uint32_t*>(t[ps_in_ROI_Sy].get_dataptr()),
                                                          static_cast<const float*>(t[ps_in_ROI_x].get_dataptr()),
                                                          static_cast<const float*>(t[ps_in_ROI_y].get_dataptr()),
                                                          in_n_ROI,
                                                          static_cast<uint16_t*>(t[ps_out_ROI_id].get_dataptr()),
                                                          static_cast<uint16_t*>(t[ps_out_ROI_xmin].get_dataptr()),
                                                          static_cast<uint16_t*>(t[ps_out_ROI_xmax].get_dataptr()),
                                                          static_cast<uint16_t*>(t[ps_out_ROI_ymin].get_dataptr()),
                                                          static_cast<uint16_t*>(t[ps_out_ROI_ymax].get_dataptr()),
                                                          static_cast<uint32_t*>(t[ps_out_ROI_S].get_dataptr()),
                                                          static_cast<uint32_t*>(t[ps_out_ROI_Sx].get_dataptr()),
                                                          static_cast<uint32_t*>(t[ps_out_ROI_Sy].get_dataptr()),
                                                          static_cast<float*>(t[ps_out_ROI_x].get_dataptr()),
                                                          static_cast<float*>(t[ps_out_ROI_y].get_dataptr()));

            *static_cast<uint32_t*>(t[ps_out_n_ROI].get_dataptr()) = (uint32_t)out_n_ROI;

            return aff3ct::module::status_t::SUCCESS;
        });
    }

    virtual ~Features_merger() {
        free(this->in_img1 + (this->i0 - this->b));
        free(this->in_img2 + (this->i0 - this->b));
        free(this->out_img + (this->i0 - this->b));
    }

    virtual Features_merger* clone() const {
        auto m = new Features_merger(*this);
        m->deep_copy(*this);
        return m;
    }

    void deep_copy(const Features_merger &m)
    {
        Module::deep_copy(m);
        this->init_data();
    }

    inline uint8_t** get_out_img() {
        return this->out_img;
    }

    inline aff3ct::module::Task& operator[](const ftr_mrg::tsk t) {
        return aff3ct::module::Module::operator[]((size_t)t);
    }

    inline aff3ct::module::Socket& operator[](const ftr_mrg::sck::merge s) {
        return aff3ct::module::Module::operator[]((size_t)ftr_mrg::tsk::merge)[(size_t)s];
    }

protected:
    void init_data() {
        this->in_img1 = (const uint32_t**)malloc((size_t)(((i1 - i0) + 1 + 2 * b) * sizeof(const uint32_t*)));
        this->in_img2 = (const uint8_t**)malloc((size_t)(((i1 - i0) + 1 + 2 * b) * sizeof(const uint8_t*)));
        this->out_img = (uint8_t**)malloc((size_t)(((i1 - i0) + 1 + 2 * b) * sizeof(uint8_t*)));
        this->in_img1 -= i0 - b;
        this->in_img2 -= i0 - b;
        this->out_img -= i0 - b;
    }
};