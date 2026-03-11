#include "image_processor.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include "globals.h"
#include "measure2.h"

static inline int odd_or_next(int k) { return (k >= 3) ? (k | 1) : 1; }


// -------- Shift (no double-copy, handles in-place safely) --------
void h_shift(const cv::Mat &ImageIn_U, cv::Mat &ImageOut_U, int shift_px)
{
  CV_Assert(ImageIn_U.type() == CV_8UC3 || ImageIn_U.type() == CV_32FC3);

  if (ImageIn_U.empty()) {
    ImageOut_U.release();
    return;
  }

  const int W = ImageIn_U.cols;
  const int H = ImageIn_U.rows;
  if (W <= 0) {
    ImageOut_U.release();
    return;
  }

  // Normalize shift into [0, W)
  int s = shift_px % W;
  if (s < 0) s += W;

  // Fast path for zero shift
  if (s == 0) {
    if (ImageIn_U.data != ImageOut_U.data)
      ImageIn_U.copyTo(ImageOut_U);
    return;
  }

  // Detect alias *before* create()
  const bool alias = (ImageOut_U.data == ImageIn_U.data);

  // Ensure dst has right size/type
  ImageOut_U.create(ImageIn_U.size(), ImageIn_U.type());

  // If aliasing, take a safe copy of the source
  cv::Mat SafeSrc;
  const cv::Mat &Src = alias ? (ImageIn_U.copyTo(SafeSrc), SafeSrc) : ImageIn_U;

  // Left and right source regions
  const cv::Rect rLeft (0,     0, W - s, H);
  const cv::Rect rRight(W - s, 0, s,     H);

  // Place right chunk at the left, left chunk after it
  Src(rRight).copyTo(ImageOut_U(cv::Rect(0, 0, s, H)));
  Src(rLeft ).copyTo(ImageOut_U(cv::Rect(s, 0, W - s, H)));
}


// -------- Blur (unchanged API, tiny guard) --------
void gauss_blur_inplace(cv::Mat &ImageInOut_F, int k)
{
  int ks =  (k >= 3) ? (k | 1) : 1;
  if (ks <= 1 || ImageInOut_F.empty())
    return;
  cv::GaussianBlur(ImageInOut_F, ImageInOut_F, cv::Size(ks, ks), 0);
}

// -------- Fused non-spatial (with inv255 + clamp) --------
void process_pixels(cv::Mat &ImageIn_F, const std::vector<int> &local_params)
{
  CV_Assert(ImageIn_F.type() == CV_32FC3);

  const bool invert = local_params[GAIN] < 0;
  const float gain_abs = std::abs(local_params[GAIN]) / 100.0f;
  const float black_add = 2.0f * static_cast<float>(local_params[BLACK_LEVEL]);
  const float t = static_cast<float>(local_params[IMAGE_GAMMA]) / 100.0f;
  const float hue = static_cast<float>(local_params[COLOR_HUE]) * (float)CV_PI / 180.0f;
  const float cg = static_cast<float>(local_params[COLOR_GAIN]) / 100.0f;
  const float ccos = cg * std::cos(hue);
  const float ssin = cg * std::sin(hue);
  const float kGY_RY = -0.5093f;
  const float kGY_BY = -0.1942f;
  const float inv255 = 1.0f / 255.0f;

  cv::parallel_for_(cv::Range(0, ImageIn_F.rows), [&](const cv::Range &r)
                    {
    for (int y = r.start; y < r.end; ++y) {
      cv::Vec3f* row = ImageIn_F.ptr<cv::Vec3f>(y);
      for (int x = 0; x < ImageIn_F.cols; ++x) {
        float B=row[x][0], G=row[x][1], R=row[x][2];

        if (invert){ R=255.f-R; G=255.f-G; B=255.f-B; }
        R = R * gain_abs + black_add;
        G = G * gain_abs + black_add;
        B = B * gain_abs + black_add;

        const float R2 = (R*R) * inv255;
        const float G2 = (G*G) * inv255;
        const float B2 = (B*B) * inv255;
        R = (1.f - t)*R + t*R2;
        G = (1.f - t)*G + t*G2;
        B = (1.f - t)*B + t*B2;

        const float Y  = 0.299f*R + 0.587f*G + 0.114f*B;
        const float RY = R - Y;
        const float BY = B - Y;

        const float RYp = ccos*RY +  ssin*BY;
        const float BYp = ccos*BY + (-ssin)*RY;

        const float Rp  = Y + RYp;
        const float Bp  = Y + BYp;
        const float GYp = kGY_RY*RYp + kGY_BY*BYp;
        const float Gp  = Y + GYp;

        R = std::clamp(Rp, 0.f, 255.f);
        G = std::clamp(Gp, 0.f, 255.f);
        B = std::clamp(Bp, 0.f, 255.f);

        row[x] = cv::Vec3f(B, G, R);
      }
    } });
}

// -------- Stateless pipeline (single Mat in/out) --------
// standalone processor (non-class)
// use for mixer out etc  not the player
// 8-bit or 32F in, always 32F out (non-class, with optional shift)
cv::Mat process_image_with_shift(const cv::Mat &ImageIn,
                                 const std::vector<int> &local_params)
{
    CV_Assert(ImageIn.type() == CV_8UC3 || ImageIn.type() == CV_32FC3);

    // Optional shift first, using the input's native type
    const cv::Mat* src = &ImageIn;
    cv::Mat shifted;
    if (local_params[H_SHIFT] != 0) {
        h_shift(ImageIn, shifted, local_params[H_SHIFT]); // alias-safe
        src = &shifted;
    }

    // Ensure float working buffer
    cv::Mat workF;
    if (src->type() == CV_8UC3) {
        src->convertTo(workF, CV_32FC3);
    } else {
        // Safer: clone so we don't mutate caller's buffer
        workF = src->clone();
        // If you prefer zero-copy and don't mind mutating input: workF = *src;
    }

    // Pixel math in 32F
    process_pixels(workF, local_params);

    // Optional blur in 32F
    if (local_params[FILTER_TYPE])
        gauss_blur_inplace(workF, local_params[FILTER_TYPE]);

    return workF; // CV_32FC3
}



// 8 bits in float out  non class no shifter
cv::Mat process_image(const cv::Mat &ImageIn_U,
                       const std::vector<int> &local_params)
{
  CV_Assert(ImageIn_U.type() == CV_8UC3);

  cv::Mat Image_Processed_F_Local;
  Image_Processed_F_Local.create(ImageIn_U.size(), CV_32FC3); // ensures shape/type
  ImageIn_U.convertTo(Image_Processed_F_Local, CV_32FC3);     // reuses buffer if same size


  auto process_start = Clock::now();

  process_pixels(Image_Processed_F_Local, local_params);

   auto process_delta = GetDeltaTime(process_start);
  std::cout << "Elapsed Para: " << process_delta << " us\n";

  if (local_params[FILTER_TYPE])
    gauss_blur_inplace(Image_Processed_F_Local, local_params[FILTER_TYPE]);

  return Image_Processed_F_Local;
}



// class version faster  (checked it...not much faster 200 usecs maybe)
Image_Processor::Image_Processor(int rows, int cols)
{
  ShiftedTemp.create(rows, cols, CV_8UC3);
  Image_Processed_F.create(rows, cols, CV_32FC3);
}


// 8 bits in float out  for use in player class only
cv::Mat &Image_Processor::Process_Image(const cv::Mat &ImageIn_U,
                                       const std::vector<int> &local_params)
{
  CV_Assert(ImageIn_U.type() == CV_8UC3);

  // 8U -> 32F
  ImageIn_U.convertTo(Image_Processed_F, CV_32FC3);

  auto process_start = Clock::now();
  // pixel math (in-place on 32F)
  process_pixels(Image_Processed_F, local_params);
   auto process_delta = GetDeltaTime(process_start);
  // std::cout << "Elapsed Para: " << process_delta << " us\n";

  // optional blur on 32F
  if (local_params[FILTER_TYPE])
    gauss_blur_inplace(Image_Processed_F, local_params[FILTER_TYPE]);

  // export 8U
  // Image_Processed_F.convertTo(ImageOut_U, CV_8UC3);

  // export 32F (shallow header alias; valid until next call on THIS instance)
  // ImageOut_F = Image_Processed_F;

  return Image_Processed_F;
}




// the class is barely faster (maybe better for threading ?? !!!

// chat:

// the class is faster and more predictable in a video loop. Here’s why:

// What the class buys you (vs the free function)
//   No per-frame allocations
//   1080p CV_32FC3 scratch ≈ 24 MB. Allocating/freeing that every frame at 60 fps is ~1.4 GB/s of allocator traffic + fragmentation.
//   In the class you allocate ShiftedTemp (8U) and Image_Processed_F (32F) once and reuse them. convertTo writes into your pre-created buffer, so no hidden re-alloc.

// Stable frame times (low jitter)
//   Heap allocs are spiky. Reusing buffers makes frame time consistent, which matters for smooth playback.

// Cache friendliness
//   Reusing the same buffers improves cache locality vs constantly getting fresh pages.

// Easy multi-threading
//   One ImageProcessor per thread → no shared state. The free function is thread-safe too, but the class keeps each thread’s scratch separate without passing temporaries around.

// A place for extras later
//   You can stash precomputed LUTs, separable kernels, GPU buffers, or switch the internals to UMat/GpuMat without touching the call sites.

// When to use which
//   Video player path (every frame): use the class process_image(...).
//   One-off tools / tests / offline scripts: the free function is fine.

// So, called from your video player, the class version is faster (less allocator work), smoother (less jitter), and scales better. The free function is perfect for convenience elsewhere.