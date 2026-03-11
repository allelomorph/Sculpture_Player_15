#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

// ---- Free functions (single-source pipeline) ----

// Shift src horizontally with wrap-around into dst. src==dst allowed.
void h_shift(const cv::Mat &src, cv::Mat &dst, int shift_px);

// Gaussian blur with odd kernel; no-op if k<=1. Works on 8U or 32F.
void gauss_blur_inplace(cv::Mat &img_8u_or_32f3, int k);

// Fused non-spatial math. Expects CV_32FC3 BGR in [0..255], in-place.
void process_pixels(cv::Mat &bgr32f, const std::vector<int> &local_params);

// One-shot pipeline: (optional shift) -> 32F -> fused -> (optional blur) -> 8U.
cv::Mat process_image(const cv::Mat &ImageIn_U_or_F,
                       const std::vector<int> &local_params);

cv::Mat process_image_with_shift(const cv::Mat &ImageIn_U_or_F,  const std::vector<int> &local_params);



// ---- Stateful fast path (buffer reuse) ----
class Image_Processor
{
public:
  Image_Processor(int rows, int cols); // preallocs internal buffers

  // Single-source pipeline.
  cv::Mat &Process_Image(const cv::Mat &ImageIn_U, const std::vector<int> &local_params);

private:
  cv::Mat ShiftedTemp;       // reused 8U tmp for shifting
  cv::Mat Image_Processed_F; // reused 32F working buffer
};
