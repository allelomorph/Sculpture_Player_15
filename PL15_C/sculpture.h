#pragma once
#include "player_class_Mat.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <array>
#include <vector>

class Sculpture
{
public:
  Sculpture();
  Sculpture(const std::string &filename, const std::string &tag);

  // Return a const ref to an internal buffer
  const cv::Mat &Cross_Fade(const cv::Mat &A, const cv::Mat &B, bool toA, float fade_length_sec);

  bool Advance(std::array<cv::Mat, 4> &outputs);


void Save_Samples_Grid_To_Buffer_RGB(
    const cv::Mat& sampler_f,   // CV_32FC3, source image
    cv::Mat& out,               // CV_32FC3, SCULPTURE_IMAGE_ROWS x SCULPTURE_IMAGE_COLS
    bool auto_gap = true,
    int H_gap = 100,                  // used if !auto_gap (pixels)
    int V_gap = 100);                  // used if !auto_gap (pixels)


void Save_Staggered_Samples_Grid_To_Buffer_RGB(
    const cv::Mat& sampler_f,   // CV_32FC3, source image
    cv::Mat& out,               // CV_32FC3, SCULPTURE_IMAGE_ROWS x SCULPTURE_IMAGE_COLS
    bool auto_gap = true,
    int H_gap = 100,                  // used if !auto_gap (pixels)
    int V_gap = 100,
    int Stagger = 0);                  // used if !auto_gap (pixels)
    
    

  Video_Player_With_Processing VP[3];

  std::vector<int> Mixer_Params;

  Mat Cross_Faded_F;
  Mat Cross_Faded_M;

  Mat Cross_Faded_F_Sampled;
  Mat Cross_Faded_M_Sampled;

  int loop_counter;

private:
  cv::Mat image_mixed_; // internal reusable buffer
  cv::Mat image_mixed_F;
  float fade_level_ = 0.f; // 0 = fully B, 1 = fully A

  vector<vector<int>> Sample_Points_Map;
  vector<vector<int>> Sculpture_Map;

  inline static const vector<int> Mixer_Params_Defaults{
      100, // GAIN
      0,   // BLACK_LEVEL
      100, // COLOR_GAIN
      0,   // COLOR_HUE
      50,  // IMAGE_GAMMA
      512, // H_SHIFT
      0,   // SPEED
      0,  // H_ROTATE      
      3,   // FILTER_TYPE

      0, // PAUSE_TOGGLE
      0, // FAST_FORWARD
      0, // REWIND
      0  // QUIT

  };
};