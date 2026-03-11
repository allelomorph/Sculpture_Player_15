
#pragma once

#include <filesystem>
#include <opencv2/opencv.hpp>
#include "measure2.h"
#include "image_processor.h"

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

class Video_Player_With_Processing
{
private:
  Image_Processor Player_Proc; // <-- stateful processor (buffer reuse)

  // inline void Grab_Frame_From_File(UMat &ImageOut, const int &Frame_Interp_Type);
  inline void Grab_Frame_From_File_Mat(Mat &ImageOut, const int &Frame_Interp_Type);

  uint32_t capWidth;
  uint32_t capHeight;
  uint32_t capLength;

  bool Scale_Video;

  double HShift_Offset_px_ = 0.0; // owned by the class

  // general terminology: _F is float _U is UMAt _FU is both

  Mat ImageInUnscaled_M;
  Mat ImageIn_M;
  Mat ImageInNew_M;
  Mat ImageInOld_M;
  Mat ImageShifted_M;
  Mat Image_M;

  Mat Temp_Read_Movie;

  Mat Ones_FM;

  // For Gamma Correction
  UMat Image_squared;

  bool Check_File_Type;

  inline static const vector<int> Player_Params_Defaults{
      100, // GAIN
      0,   // BLACK_LEVEL
      100, // COLOR_GAIN
      0,   // COLOR_HUE
      25,  // IMAGE_GAMMA
      512, // H_SHIFT
      0,   // H_ROTATE
      100, // SPEED
      3,   // FILTER_TYPE

      0, // PAUSE_TOGGLE
      0, // FAST_FORWARD
      0, // REWIND
      0  // QUIT

  };

  inline static const vector<vector<int>>
      Player_Default_Limits{
          {100, -100, 200}, // GAIN
          {0, -100, 100},   // BLACK_LEVEL
          {100, 0, 200},    // COLOR_GAIN
          {0, -180, 180},   // COLOR_HUE
          {25, 0, 100},     // IMAGE_GAMMA
          {512, 0, 1023},   // H_SHIFT
          {0, -100, 100},   // ROTATE
          {100, 0, 300},    // SPEED
          {3, 0, 10},       // FILTER_TYPE

          {0, 0, 1}, // PAUSE_TOGGLE
          {0, 0, 1}, // FAST_FORWARD
          {0, 0, 1}, // REWIND
          {0, 0, 1}, // QUIT

      };

public:
  // move back to private
  VideoCapture capMain;

  Video_Player_With_Processing(void);

  std::string Open_Image_File_Mat(const string &Image_File_Name_In, const string &Name_In);

  void Close_Image_File(void);

  std::string Check_Image_File(const string &Image_File_Name_In, const string &Name_In);

  void Change_Param(const int &Param_Index, const std::string Value);
  void Change_Param(const int &Param_Index, const int Value);
  void Change_All_Params(std::vector<int> Value);

  static int wrap_mod_U(int s, int W);

  void H_Shift_Rotate(const cv::Mat &ImageIn_U,
                      cv::Mat &ImageOut_U,
                      double Location_px,
                      bool Force_Location = true,
                      int Step_px = 0);

  void Process_New_Frame_Ext_Process(void);

  string Last_Good_Filename;

  vector<int> Player_Params;

  string display_name;

  string Default_Main_Movie_Path;

  string Default_Movie_With_Path;

  Mat VideoDisplay;

  Mat ImageMain_FM;

  bool player_pause;

  int Frame_Interp_Type;

  int Location_Accum;
  int Current_Frame;
  int Location_Diff;
  int Duration_Frames;

  bool display_on;

  // for program optimizing
  Prog_Durations Time_Delay[10];
};
