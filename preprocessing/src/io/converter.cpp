// Copyright (c) 2014 Bauhaus-Universitaet Weimar
// This Software is distributed under the Modified BSD License, see license.txt.
//
// Virtual Reality and Visualization Research Group 
// Faculty of Media, Bauhaus-Universitaet Weimar
// http://www.uni-weimar.de/medien/vr

#include <lamure/pre/io/converter.h>
#include <thread>
#include <cmath>

namespace lamure {
namespace pre {

void Converter::
Convert(const std::string& input_filename,
        const std::string& output_filename)
{
    discarded_ = 0;
    flush_ready_ = false;
    flush_done_ = false;

    auto buf_callback = [&](SurfelVector& surfels) {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [this]{return flush_ready_;});

        surfels = buffer_;
        flush_ready_ = false;
        bool has_data = !buffer_.empty();

        // notify main thread
        flush_done_ = true;
        lk.unlock();
        cv_.notify_one();
        return has_data;
    };

    // output thread
    std::thread tr([&] {
        out_format_.Write(output_filename, buf_callback);
    });

    // read input
    in_format_.Read(input_filename,
                    [&](const Surfel& s) { this->AppendSurfel(s); });

    FlushBuffer();
    {
        std::lock_guard<std::mutex> lk(mtx_);
        flush_ready_ = true;
    }
    cv_.notify_one();
    tr.join();

    if (discarded_ > 0) {
        LOGGER_WARN("Discarded degenerate surfels: " << 
                                discarded_);
    }
}

void Converter::
AppendSurfel(const Surfel& surfel)
{
    if (IsDegenerate(surfel)) {
        ++discarded_;
        return;
    }

    Surfel s(surfel);
    bool keep = true;

    if (surfel_callback_)
        surfel_callback_(s, keep);

    if (keep) {
        if (scale_factor_ != 1.0) {
            s.pos() *= scale_factor_;
            s.radius() *= scale_factor_;
        }

        if (translation_ != vec3r(0.0)) {
            s.pos() += translation_;
        }

        if (override_radius_)
            s.radius() = new_radius_;

        if (override_color_)
            s.color() = new_color_;

        //LOGGER_DEBUG("Pos: " << s.pos());

        buffer_.push_back(s);

        if (buffer_.size() > surfels_in_buffer_)
            FlushBuffer();
    }
}


void Converter::
FlushBuffer()
{
    if (!buffer_.empty()) {
        LOGGER_TRACE("Flush buffer to disk. Buffer size: " << 
                        buffer_.size() << " surfels");
        {
            std::lock_guard<std::mutex> lk(mtx_);
            flush_ready_ = true;
        }
        cv_.notify_one();
        {
            std::unique_lock<std::mutex> lk(mtx_);
            // get back result
            cv_.wait(lk, [this] { return flush_done_; });
            flush_done_ = false;
            buffer_.clear();
        }
    }
}

const bool Converter::
IsDegenerate(const Surfel& s) const
{
    return !std::isfinite(s.pos().x) 
        || !std::isfinite(s.pos().y)
        || !std::isfinite(s.pos().z);
}


} // namespace pre
} // namespace lamure
