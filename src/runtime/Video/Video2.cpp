#include "fmdt/video.h"

#include "fmdt/Video/Video2.hpp"

Video2::Video2(const std::string filename, const size_t frame_start, const size_t frame_end, const size_t frame_skip,
             const size_t n_ffmpeg_threads, const int b)
: Module(), i0(0), i1(0), j0(0), j1(0), b(b), video(nullptr), out_img(nullptr), done(false) {
    const std::string name = "Video2";
    this->set_name(name);
    this->set_short_name(name);

    this->video = video_init_from_file(filename.c_str(), frame_start, frame_end, frame_skip, n_ffmpeg_threads,
                                       &this->i0, &this->i1, &this->j0, &this->j1);

    this->out_img = (uint8_t**)malloc((size_t)(((i1 - i0) + 1 + 2 * b) * sizeof(uint8_t*)));
    this->out_img -= i0 - b;

    auto socket_size = ((i1 - i0) + 1 + 2 * b) * ((j1 - j0) + 1 + 2 * b);

    auto &p = this->create_task("generate");
    auto ps_out_img = this->template create_socket_out<uint8_t>(p, "out_img", socket_size);
    auto ps_out_frame = this->template create_socket_out<uint32_t>(p, "out_frame", 1);

    this->create_codelet(p, [ps_out_img, ps_out_frame]
                            (aff3ct::module::Module &m, aff3ct::module::Task &t,const size_t frame_id) -> int {
        auto &vid2 = static_cast<Video2&>(m);
        uint8_t* m_out_img = static_cast<uint8_t*>(t[ps_out_img].get_dataptr());
        // uint8_t* m_out_img0 = static_cast<uint8_t*>(t[ps_out_img0].get_dataptr());
        // uint8_t* m_out_img1 = static_cast<uint8_t*>(t[ps_out_img1].get_dataptr());

        // auto tmp = m_out_img0;
        // m_out_img0 = m_out_img1;
        // m_out_img1 = tmp;

        vid2.out_img[vid2.i0 - vid2.b] = m_out_img - (vid2.j0 - vid2.b);
        for (int i = vid2.i0 - vid2.b + 1; i <= vid2.i1 + vid2.b; i++)
            vid2.out_img[i] = vid2.out_img[i - 1] + ((vid2.j1 - vid2.j0) + 1 + 2 * vid2.b);

        *static_cast<uint32_t*>(t[ps_out_frame].get_dataptr()) = vid2.video->frame_current;
        int ret = video_get_next_frame(vid2.video, vid2.out_img);
        vid2.done = ret ? false : true;

        if (vid2.done)
            throw aff3ct::tools::processing_aborted(__FILE__, __LINE__, __func__);
        return ret ? aff3ct::module::status_t::SUCCESS : aff3ct::module::status_t::FAILURE_STOP;
    });
}



// Video2::Video2(const std::string filename, const size_t frame_start, const size_t frame_end, const size_t frame_skip,
//              const size_t n_ffmpeg_threads, const int b)
// : Module(), i0(0), i1(0), j0(0), j1(0), b(b), video(nullptr), out_img(nullptr), done(false) {
//     const std::string name = "Video2";
//     this->set_name(name);
//     this->set_short_name(name);

//     this->video = video_init_from_file(filename.c_str(), frame_start, frame_end, frame_skip, n_ffmpeg_threads,
//                                        &this->i0, &this->i1, &this->j0, &this->j1);

//     this->out_img = (uint8_t**)malloc((size_t)(((i1 - i0) + 1 + 2 * b) * sizeof(uint8_t*)));
//     this->out_img -= i0 - b;

//     auto socket_size = ((i1 - i0) + 1 + 2 * b) * ((j1 - j0) + 1 + 2 * b);

//     auto &p = this->create_task("generate");
//     auto ps_out_img0 = this->template create_socket_out<uint8_t>(p, "out_img0", socket_size);
//     auto ps_out_img1 = this->template create_socket_out<uint8_t>(p, "out_img1", socket_size);
//     auto ps_out_frame = this->template create_socket_out<uint32_t>(p, "out_frame", 1);

//     this->create_codelet(p, [ps_out_img0, ps_out_img1, ps_out_frame]
//                             (aff3ct::module::Module &m, aff3ct::module::Task &t,const size_t frame_id) -> int {
//         auto &vid2 = static_cast<Video2&>(m);
//         uint8_t* m_out_img0 = static_cast<uint8_t*>(t[ps_out_img0].get_dataptr());
//         uint8_t* m_out_img1 = static_cast<uint8_t*>(t[ps_out_img1].get_dataptr());

//         auto tmp = m_out_img0;
//         m_out_img0 = m_out_img1;
//         m_out_img1 = tmp;

//         vid2.out_img[vid2.i0 - vid2.b] = m_out_img1 - (vid2.j0 - vid2.b);
//         for (int i = vid2.i0 - vid2.b + 1; i <= vid2.i1 + vid2.b; i++)
//             vid2.out_img[i] = vid2.out_img[i - 1] + ((vid2.j1 - vid2.j0) + 1 + 2 * vid2.b);

//         *static_cast<uint32_t*>(t[ps_out_frame].get_dataptr()) = vid2.video->frame_current;
//         int ret = video_get_next_frame(vid2.video, vid2.out_img);
//         vid2.done = ret ? false : true;

//         if (vid2.done)
//             throw aff3ct::tools::processing_aborted(__FILE__, __LINE__, __func__);
//         return ret ? aff3ct::module::status_t::SUCCESS : aff3ct::module::status_t::FAILURE_STOP;
//     });
// }

Video2::~Video2() {
    free(this->out_img + this->i0 - this->b);
    video_free(this->video);
}

bool Video2::is_done() const {
    return this->done;
}
