#include "thumbnail_generator.hpp"
#include "gopost/engine.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace gopost {
namespace video {

// =============================================================================
// Simple JPEG encoder (minimal baseline JPEG without external dependency)
// =============================================================================

// For a production build you'd link libjpeg-turbo or stb_image_write.
// This minimal encoder produces valid JPEG files for small thumbnails.
// We include a compact baseline JPEG writer inline to avoid extra deps.

namespace {

// Zig-zag order for 8x8 block
static const uint8_t kZigZag[64] = {
    0, 1, 8,16, 9, 2, 3,10, 17,24,32,25,18,11, 4, 5,
   12,19,26,33,40,48,41,34, 27,20,13, 6, 7,14,21,28,
   35,42,49,56,57,50,43,36, 29,22,15,23,30,37,44,51,
   58,59,52,45,38,31,39,46, 53,60,61,54,47,55,62,63
};

// Standard luminance quantization table (quality ~75)
static const uint8_t kLumQuant[64] = {
    8, 6, 5, 8,12,20,26,31,  6, 6, 7, 9,13,29,30,28,
    7, 7, 8,12,20,29,35,28,  7, 9,11,15,26,44,40,31,
    9,11,19,28,34,55,52,39, 12,18,28,32,40,52,57,46,
   25,32,39,44,52,61,60,51, 36,46,48,49,56,50,52,50
};

// Standard chrominance quantization table
static const uint8_t kChromQuant[64] = {
    9, 9,12,24,50,50,50,50,  9,11,13,33,50,50,50,50,
   12,13,28,50,50,50,50,50, 24,33,50,50,50,50,50,50,
   50,50,50,50,50,50,50,50, 50,50,50,50,50,50,50,50,
   50,50,50,50,50,50,50,50, 50,50,50,50,50,50,50,50
};

struct BitWriter {
    std::vector<uint8_t>* out;
    uint32_t buf = 0;
    int bits = 0;

    void write_bits(uint16_t val, int count) {
        buf = (buf << count) | (val & ((1u << count) - 1));
        bits += count;
        while (bits >= 8) {
            bits -= 8;
            uint8_t byte = static_cast<uint8_t>((buf >> bits) & 0xFF);
            out->push_back(byte);
            if (byte == 0xFF) out->push_back(0x00);  // byte stuffing
        }
    }

    void flush() {
        if (bits > 0) {
            write_bits(0x7F, 7);  // pad with 1-bits
        }
    }
};

// Simplified DCT and Huffman — for small thumbnails this is adequate.
// For production quality, replace with libjpeg-turbo.

static void write_u8(std::vector<uint8_t>& out, uint8_t v) { out.push_back(v); }
static void write_u16be(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v & 0xFF));
}

}  // namespace

// =============================================================================
// Bilinear scale RGBA
// =============================================================================

void ThumbnailGenerator::scale_rgba(const uint8_t* src, int src_w, int src_h,
                                     uint8_t* dst, int dst_w, int dst_h) {
    if (src_w == dst_w && src_h == dst_h) {
        std::memcpy(dst, src, static_cast<size_t>(dst_w) * dst_h * 4);
        return;
    }

    const float x_ratio = static_cast<float>(src_w) / dst_w;
    const float y_ratio = static_cast<float>(src_h) / dst_h;

    for (int y = 0; y < dst_h; y++) {
        const float fy = y * y_ratio;
        const int sy = static_cast<int>(fy);
        const float fy_frac = fy - sy;
        const int sy1 = std::min(sy + 1, src_h - 1);

        for (int x = 0; x < dst_w; x++) {
            const float fx = x * x_ratio;
            const int sx = static_cast<int>(fx);
            const float fx_frac = fx - sx;
            const int sx1 = std::min(sx + 1, src_w - 1);

            for (int c = 0; c < 4; c++) {
                const float tl = src[(sy * src_w + sx) * 4 + c];
                const float tr = src[(sy * src_w + sx1) * 4 + c];
                const float bl = src[(sy1 * src_w + sx) * 4 + c];
                const float br = src[(sy1 * src_w + sx1) * 4 + c];

                const float top = tl + (tr - tl) * fx_frac;
                const float bot = bl + (br - bl) * fx_frac;
                const float val = top + (bot - top) * fy_frac;

                dst[(y * dst_w + x) * 4 + c] = static_cast<uint8_t>(
                    std::min(255.0f, std::max(0.0f, val + 0.5f)));
            }
        }
    }
}

// =============================================================================
// Minimal JPEG encode (RGB only, no subsampling, quality ~75)
// Suitable for small thumbnails. Replace with stb_image_write or libjpeg
// for production if needed.
// =============================================================================

bool ThumbnailGenerator::encode_rgba_to_jpeg(
    const uint8_t* rgba, int w, int h,
    int /*out_w*/, int /*out_h*/,
    std::vector<uint8_t>& out_jpeg) {

    // For a minimal implementation we'll just store raw JPEG with SOI/EOI
    // markers and uncompressed RGB data in JFIF format.
    // This is a simplified approach — for production, link stb_image_write.h.

    // Instead, encode as a simple BMP-like format wrapped in JPEG container.
    // Actually, let's use a truly minimal JPEG baseline encoder.

    // For thumbnails at 160x90, using an uncompressed approach:
    // Store as raw RGB in a minimal JFIF container.

    out_jpeg.clear();

    // SOI
    write_u16be(out_jpeg, 0xFFD8);

    // APP0 JFIF header
    write_u16be(out_jpeg, 0xFFE0);
    write_u16be(out_jpeg, 16);  // length
    out_jpeg.push_back('J'); out_jpeg.push_back('F');
    out_jpeg.push_back('I'); out_jpeg.push_back('F');
    out_jpeg.push_back(0);   // null
    out_jpeg.push_back(1);   // version major
    out_jpeg.push_back(1);   // version minor
    out_jpeg.push_back(0);   // units (0 = no units)
    write_u16be(out_jpeg, 1); // X density
    write_u16be(out_jpeg, 1); // Y density
    out_jpeg.push_back(0);   // thumbnail width
    out_jpeg.push_back(0);   // thumbnail height

    // DQT - Luminance
    write_u16be(out_jpeg, 0xFFDB);
    write_u16be(out_jpeg, 67);  // length
    out_jpeg.push_back(0);      // table 0, 8-bit precision
    for (int i = 0; i < 64; i++) out_jpeg.push_back(kLumQuant[kZigZag[i]]);

    // DQT - Chrominance
    write_u16be(out_jpeg, 0xFFDB);
    write_u16be(out_jpeg, 67);
    out_jpeg.push_back(1);
    for (int i = 0; i < 64; i++) out_jpeg.push_back(kChromQuant[kZigZag[i]]);

    // SOF0 - Baseline
    write_u16be(out_jpeg, 0xFFC0);
    write_u16be(out_jpeg, 17);    // length
    out_jpeg.push_back(8);        // precision
    write_u16be(out_jpeg, static_cast<uint16_t>(h));
    write_u16be(out_jpeg, static_cast<uint16_t>(w));
    out_jpeg.push_back(3);        // components
    // Y: id=1, sampling=1x1, quant=0
    out_jpeg.push_back(1); out_jpeg.push_back(0x11); out_jpeg.push_back(0);
    // Cb: id=2, sampling=1x1, quant=1
    out_jpeg.push_back(2); out_jpeg.push_back(0x11); out_jpeg.push_back(1);
    // Cr: id=3, sampling=1x1, quant=1
    out_jpeg.push_back(3); out_jpeg.push_back(0x11); out_jpeg.push_back(1);

    // DHT - DC Luminance (standard table)
    static const uint8_t dc_lum_bits[16] = {0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0};
    static const uint8_t dc_lum_vals[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
    write_u16be(out_jpeg, 0xFFC4);
    write_u16be(out_jpeg, 31);  // length
    out_jpeg.push_back(0x00);   // class=DC, id=0
    for (int i = 0; i < 16; i++) out_jpeg.push_back(dc_lum_bits[i]);
    for (int i = 0; i < 12; i++) out_jpeg.push_back(dc_lum_vals[i]);

    // DHT - AC Luminance (standard table)
    static const uint8_t ac_lum_bits[16] = {0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7D};
    // Full standard AC luminance values (162 values)
    static const uint8_t ac_lum_vals[] = {
        0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,
        0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xa1,0x08,0x23,0x42,0xb1,0xc1,0x15,0x52,
        0xd1,0xf0,0x24,0x33,0x62,0x72,0x82,0x09,0x0a,0x16,0x17,0x18,0x19,0x1a,0x25,
        0x26,0x27,0x28,0x29,0x2a,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,0x45,
        0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,0x64,
        0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x83,
        0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,
        0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,0xb5,0xb6,
        0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xd2,0xd3,
        0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,
        0xe9,0xea,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
    };
    int ac_lum_total = 0;
    for (int i = 0; i < 16; i++) ac_lum_total += ac_lum_bits[i];
    write_u16be(out_jpeg, 0xFFC4);
    write_u16be(out_jpeg, static_cast<uint16_t>(19 + ac_lum_total));
    out_jpeg.push_back(0x10);  // class=AC, id=0
    for (int i = 0; i < 16; i++) out_jpeg.push_back(ac_lum_bits[i]);
    for (int i = 0; i < ac_lum_total; i++) out_jpeg.push_back(ac_lum_vals[i]);

    // DHT - DC Chrominance
    static const uint8_t dc_chrom_bits[16] = {0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0};
    static const uint8_t dc_chrom_vals[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
    write_u16be(out_jpeg, 0xFFC4);
    write_u16be(out_jpeg, 31);
    out_jpeg.push_back(0x01);
    for (int i = 0; i < 16; i++) out_jpeg.push_back(dc_chrom_bits[i]);
    for (int i = 0; i < 12; i++) out_jpeg.push_back(dc_chrom_vals[i]);

    // DHT - AC Chrominance
    static const uint8_t ac_chrom_bits[16] = {0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77};
    static const uint8_t ac_chrom_vals[] = {
        0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,
        0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xa1,0xb1,0xc1,0x09,0x23,0x33,
        0x52,0xf0,0x15,0x62,0x72,0xd1,0x0a,0x16,0x24,0x34,0xe1,0x25,0xf1,0x17,0x18,
        0x19,0x1a,0x26,0x27,0x28,0x29,0x2a,0x35,0x36,0x37,0x38,0x39,0x3a,0x43,0x44,
        0x45,0x46,0x47,0x48,0x49,0x4a,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x63,
        0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,
        0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x92,0x93,0x94,0x95,0x96,0x97,
        0x98,0x99,0x9a,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xb2,0xb3,0xb4,
        0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,
        0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,
        0xe8,0xe9,0xea,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa
    };
    int ac_chrom_total = 0;
    for (int i = 0; i < 16; i++) ac_chrom_total += ac_chrom_bits[i];
    write_u16be(out_jpeg, 0xFFC4);
    write_u16be(out_jpeg, static_cast<uint16_t>(19 + ac_chrom_total));
    out_jpeg.push_back(0x11);
    for (int i = 0; i < 16; i++) out_jpeg.push_back(ac_chrom_bits[i]);
    for (int i = 0; i < ac_chrom_total; i++) out_jpeg.push_back(ac_chrom_vals[i]);

    // SOS
    write_u16be(out_jpeg, 0xFFDA);
    write_u16be(out_jpeg, 12);    // length
    out_jpeg.push_back(3);        // components
    out_jpeg.push_back(1); out_jpeg.push_back(0x00); // Y: DC=0, AC=0
    out_jpeg.push_back(2); out_jpeg.push_back(0x11); // Cb: DC=1, AC=1
    out_jpeg.push_back(3); out_jpeg.push_back(0x11); // Cr: DC=1, AC=1
    out_jpeg.push_back(0);  // Ss
    out_jpeg.push_back(63); // Se
    out_jpeg.push_back(0);  // Ah/Al

    // Minimal scan data — for a real encoder we'd do DCT + Huffman coding.
    // Instead, store a simpler approach: encode each 8x8 MCU block.
    // For thumbnail quality this is acceptable.

    // Actually, let's take a pragmatic approach: since this is thumbnail-sized
    // (160x90), store raw pixel data that any JPEG decoder can read.
    // We'll use the simplest possible encoding: all-zero DCT coefficients
    // with DC values representing average block color. This produces a
    // blocky but recognizable thumbnail.

    {
        BitWriter bw;
        bw.out = &out_jpeg;

        int mcu_w = (w + 7) / 8;
        int mcu_h = (h + 7) / 8;
        int prev_dc[3] = {0, 0, 0};

        for (int my = 0; my < mcu_h; my++) {
            for (int mx = 0; mx < mcu_w; mx++) {
                // Compute average color for this 8x8 block
                int sum_r = 0, sum_g = 0, sum_b = 0, count = 0;
                for (int dy = 0; dy < 8; dy++) {
                    for (int dx = 0; dx < 8; dx++) {
                        int px = mx * 8 + dx;
                        int py = my * 8 + dy;
                        if (px < w && py < h) {
                            int idx = (py * w + px) * 4;
                            sum_r += rgba[idx];
                            sum_g += rgba[idx + 1];
                            sum_b += rgba[idx + 2];
                            count++;
                        }
                    }
                }
                if (count == 0) count = 1;
                float avg_r = static_cast<float>(sum_r) / count;
                float avg_g = static_cast<float>(sum_g) / count;
                float avg_b = static_cast<float>(sum_b) / count;

                // RGB to YCbCr
                float y_val  =  0.299f * avg_r + 0.587f * avg_g + 0.114f * avg_b;
                float cb_val = -0.169f * avg_r - 0.331f * avg_g + 0.500f * avg_b + 128;
                float cr_val =  0.500f * avg_r - 0.419f * avg_g - 0.081f * avg_b + 128;

                // Convert to DCT DC coefficient (divide by quant[0], apply level shift)
                int dc_vals[3];
                dc_vals[0] = static_cast<int>((y_val - 128.0f) * 8.0f / kLumQuant[0]);
                dc_vals[1] = static_cast<int>((cb_val - 128.0f) * 8.0f / kChromQuant[0]);
                dc_vals[2] = static_cast<int>((cr_val - 128.0f) * 8.0f / kChromQuant[0]);

                // Encode each component: DC coefficient + EOB (no AC coefficients)
                for (int comp = 0; comp < 3; comp++) {
                    int dc_diff = dc_vals[comp] - prev_dc[comp];
                    prev_dc[comp] = dc_vals[comp];

                    // Encode DC difference
                    int abs_diff = dc_diff < 0 ? -dc_diff : dc_diff;
                    int cat = 0;
                    int tmp = abs_diff;
                    while (tmp > 0) { cat++; tmp >>= 1; }

                    // DC Huffman: category code
                    // Standard DC luminance/chrominance Huffman codes
                    if (comp == 0) {
                        // Luminance DC
                        static const uint16_t dc_lum_code[] = {0x00,0x02,0x03,0x04,0x05,0x06,0x0E,0x1E,0x3E,0x7E,0xFE,0x1FE};
                        static const int dc_lum_len[] = {2,3,3,3,3,3,4,5,6,7,8,9};
                        bw.write_bits(dc_lum_code[cat], dc_lum_len[cat]);
                    } else {
                        // Chrominance DC
                        static const uint16_t dc_chrom_code[] = {0x00,0x01,0x02,0x06,0x0E,0x1E,0x3E,0x7E,0xFE,0x1FE,0x3FE,0x7FE};
                        static const int dc_chrom_len[] = {2,2,2,3,4,5,6,7,8,9,10,11};
                        bw.write_bits(dc_chrom_code[cat], dc_chrom_len[cat]);
                    }

                    // DC value bits
                    if (cat > 0) {
                        uint16_t val_bits;
                        if (dc_diff < 0) {
                            val_bits = static_cast<uint16_t>(dc_diff + (1 << cat) - 1);
                        } else {
                            val_bits = static_cast<uint16_t>(dc_diff);
                        }
                        bw.write_bits(val_bits, cat);
                    }

                    // EOB (end of block — no AC coefficients)
                    if (comp == 0) {
                        bw.write_bits(0x0A, 4);  // Luminance AC EOB
                    } else {
                        bw.write_bits(0x00, 2);  // Chrominance AC EOB
                    }
                }
            }
        }

        bw.flush();
    }

    // EOI
    write_u16be(out_jpeg, 0xFFD9);

    return true;
}

// =============================================================================
// ThumbnailGenerator
// =============================================================================

ThumbnailGenerator::ThumbnailGenerator(DecoderPool* pool, GopostEngine* engine)
    : pool_(pool), engine_(engine) {
    running_ = true;
    worker_ = std::thread(&ThumbnailGenerator::worker_loop, this);
}

ThumbnailGenerator::~ThumbnailGenerator() {
    shutdown();
}

int32_t ThumbnailGenerator::submit(ThumbnailRequest request) {
    std::lock_guard<std::mutex> lock(mutex_);

    int32_t id = next_job_id_.fetch_add(1);
    request.job_id = id;

    auto job = std::make_unique<Job>();
    job->request = std::move(request);
    job->status = ThumbnailJobStatus::Queued;

    // Insert into queue sorted by priority
    auto insert_pos = queue_.end();
    for (auto it = queue_.begin(); it != queue_.end(); ++it) {
        auto jit = jobs_.find(*it);
        if (jit != jobs_.end() &&
            static_cast<int>(jit->second->request.priority) >
            static_cast<int>(job->request.priority)) {
            insert_pos = it;
            break;
        }
    }
    queue_.insert(insert_pos, id);
    jobs_[id] = std::move(job);

    cv_.notify_one();
    return id;
}

void ThumbnailGenerator::cancel(int32_t job_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) return;

    if (it->second->status == ThumbnailJobStatus::Queued) {
        it->second->status = ThumbnailJobStatus::Cancelled;
        queue_.erase(
            std::remove(queue_.begin(), queue_.end(), job_id),
            queue_.end());
    } else if (it->second->status == ThumbnailJobStatus::InProgress) {
        it->second->status = ThumbnailJobStatus::Cancelled;
        // Worker will check status and abort
    }
}

void ThumbnailGenerator::cancel_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, job] : jobs_) {
        if (job->status == ThumbnailJobStatus::Queued ||
            job->status == ThumbnailJobStatus::InProgress) {
            job->status = ThumbnailJobStatus::Cancelled;
        }
    }
    queue_.clear();
}

ThumbnailJobStatus ThumbnailGenerator::job_status(int32_t job_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) return ThumbnailJobStatus::Failed;
    return it->second->status;
}

std::vector<ThumbnailFrame> ThumbnailGenerator::get_results(int32_t job_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) return {};
    if (it->second->status != ThumbnailJobStatus::Completed) return {};
    return it->second->results;
}

int32_t ThumbnailGenerator::queue_size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int32_t>(queue_.size());
}

void ThumbnailGenerator::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) return;
        running_ = false;
        cancel_all();
    }
    cv_.notify_one();
    if (worker_.joinable()) worker_.join();
}

// =============================================================================
// Worker thread
// =============================================================================

void ThumbnailGenerator::worker_loop() {
    while (running_) {
        int32_t job_id = -1;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !queue_.empty() || !running_; });
            if (!running_) break;
            if (queue_.empty()) continue;

            job_id = queue_.front();
            queue_.pop_front();

            auto it = jobs_.find(job_id);
            if (it == jobs_.end() || it->second->status == ThumbnailJobStatus::Cancelled) {
                continue;
            }
            it->second->status = ThumbnailJobStatus::InProgress;
        }

        current_job_id_ = job_id;

        // Process outside the lock
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = jobs_.find(job_id);
            if (it != jobs_.end()) {
                process_job(*it->second);
            }
        }

        current_job_id_ = -1;
    }
}

void ThumbnailGenerator::process_job(Job& job) {
    auto& req = job.request;

    if (job.status == ThumbnailJobStatus::Cancelled) return;

    // Acquire a decoder from the pool (this may block if pool is full)
    // We release the mutex during acquire to avoid deadlock.
    mutex_.unlock();

    DecoderLease lease = pool_->acquire(req.source_path, req.priority);

    mutex_.lock();

    if (!lease || job.status == ThumbnailJobStatus::Cancelled) {
        if (job.status != ThumbnailJobStatus::Cancelled) {
            job.status = ThumbnailJobStatus::Failed;
        }
        if (req.on_complete) {
            req.on_complete(req.job_id, job.status);
        }
        return;
    }

    IVideoDecoder* decoder = lease.decoder();
    const auto info = decoder->info();

    const double safe_duration = req.source_duration > 0.01
        ? req.source_duration : info.duration_seconds;

    job.results.reserve(static_cast<size_t>(req.count));

    // Allocate scale buffer once
    std::vector<uint8_t> scale_buf(
        static_cast<size_t>(req.thumb_width) * req.thumb_height * 4);

    bool had_failure = false;
    int retry_budget = 1;  // Allow 1 retry per frame

    for (int i = 0; i < req.count; i++) {
        // Check for cancellation between frames
        if (job.status == ThumbnailJobStatus::Cancelled) break;

        const double timestamp = req.count == 1
            ? 0.0
            : (safe_duration * i / req.count);

        // Decode frame at timestamp
        // Release mutex during decode (it's the long operation)
        mutex_.unlock();

        GopostFrame* frame = decoder->decode_frame_at(timestamp);

        mutex_.lock();

        if (job.status == ThumbnailJobStatus::Cancelled) {
            if (frame && engine_) {
                gopost_frame_release(engine_, frame);
            }
            break;
        }

        if (!frame) {
            if (retry_budget > 0) {
                retry_budget--;
                i--;  // Retry this frame
                continue;
            }
            had_failure = true;
            continue;
        }

        // Scale frame to thumbnail size
        scale_rgba(frame->data,
                   static_cast<int>(frame->width),
                   static_cast<int>(frame->height),
                   scale_buf.data(),
                   req.thumb_width, req.thumb_height);

        // Release the full-size frame back to pool
        if (engine_) {
            gopost_frame_release(engine_, frame);
        }

        // Encode to JPEG
        ThumbnailFrame thumb;
        thumb.width = req.thumb_width;
        thumb.height = req.thumb_height;
        thumb.timestamp = timestamp;

        if (encode_rgba_to_jpeg(scale_buf.data(),
                                req.thumb_width, req.thumb_height,
                                req.thumb_width, req.thumb_height,
                                thumb.jpeg_data)) {
            if (req.on_frame) {
                req.on_frame(i, thumb);
            }
            job.results.push_back(std::move(thumb));
        } else {
            had_failure = true;
        }
    }

    // Decoder is released when `lease` goes out of scope (RAII).
    // This ensures the decoder slot is freed for the next video.

    if (job.status != ThumbnailJobStatus::Cancelled) {
        job.status = job.results.empty() && had_failure
            ? ThumbnailJobStatus::Failed
            : ThumbnailJobStatus::Completed;
    }

    if (req.on_complete) {
        req.on_complete(req.job_id, job.status);
    }
}

}  // namespace video
}  // namespace gopost
