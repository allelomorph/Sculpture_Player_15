
#pragma once

#include <filesystem>

#include <opencv2/opencv.hpp>

#include "measure2.h"

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

class Video_Player_With_Processing
{
private:
  
  inline void Grab_Frame_From_File(UMat &ImageOut, const int &Frame_Interp_Type);
  inline void Shift_Horizontal_Location(UMat &Image_In_Out, int &Initial_Location);
  inline void Shift_Horizontal_Location(const UMat &Image_In, UMat &Image_Shifted, int &Initial_Location);
  inline void RGB_Gamma_Correction(UMat &src, const int &Gamma_Correction);
  inline void RGB_Hue_Saturation_Correction(const UMat &src, const int &Color_Gain_In, const int &Angle_In);
  inline void MultiFilter(UMat &Image, const int &FilterType);





  uint32_t capWidth;
  uint32_t capHeight;
  uint32_t capLength;

  bool Scale_Video;

  // general terminology: _F is float _U is UMAt _FU is both

  UMat ImageInUnscaled_U;
  UMat ImageIn_U;
  UMat ImageInNew_U;
  UMat ImageInOld_U;
  UMat ImageShifted_U;
  UMat ImageTemp_U;




  Mat Temp_Read_Movie;

  // For Hue Saturation Corrections
  UMat BGR_Separate[3];
  UMat Y_RG_Partial_FU1;
  UMat Y_From_RGB_FU1;
  UMat RY_From_Scratch_FU1;
  UMat BY_From_Scratch_FU1;
  UMat RY_New_FU1;
  UMat BY_New_FU1;
  UMat GY_New_FU1;
  UMat BGR_Sep_FU[3];
  Mat BGR_Sep_F[3];
  Mat BGR_Corrected_F;
  UMat BGR_Corrected_FU;

  UMat Ones_FU;

  // For Gamma Correction
  UMat Image_squared;

  bool Check_File_Type;

  const vector<vector<int>> PL_Default_Limits{
      {100, -100, 200},  // GAIN
      {0, -100, 100}, // BLACK_LEVEL
      {100, 0, 200},  // COLOR_GAIN
      {0, -180, 180}, // COLOR_HUE
      {25, 0, 100},   // IMAGE_GAMMA
      {512, 0, 1023}, // H_SHIFT
      {100, 0, 300},  // SPEED
      {3, 0, 10}      // FILTER_TYPE
  };

public:

  // move back to private
  VideoCapture capMain;

  Video_Player_With_Processing(void);

  // std::string Setup(const string &Default_Movie);

  std::string Open_Image_File(const string &Image_File_Name_In, const string &Name_In);
  std::string Open_Image_File_Rev2(const string &Image_File_Name_In, const string &Name_In);  
  std::string Open_Image_File_And_Grab_Still(const string &Image_File_Name_In, const string &Name_In, Mat &Still, int32_t Still_Width);
  std::string Open_Image_File_And_Grab_Still_Rev2(const string &Image_File_Name_In, const string &Name_In, Mat &Still, int32_t Still_Width);
  std::string Open_Image_File_And_Grab_Still_Rev3(const string &Image_File_Name_In, const string &Name_In, Mat &Still, int32_t Still_Width);

  void Close_Image_File(void);

  std::string Check_Image_File(const string &Image_File_Name_In, const string &Name_In);

  string Last_Good_Filename;
  vector<int> PL_Params;

  string display_name;

  string Default_Main_Movie_Path;

  string Default_Movie_With_Path;



  void FusedNonSpatialPass(void);

  bool Process(bool select = true);






  // inline void Grab_Frame_From_File_Print(cv::UMat &ImageOut, const int &Frame_Interp_Type);

  UMat ImageMain_FU;
  Mat VideoDisplay;

  bool player_pause;

  enum Default_Index
  {
    DEFAULT = 0,
    LOWER_LIMIT = 1,
    UPPER_LIMIT = 2,
  };

  void Change_Param(const int &Param_Index, const std::string Value);
  void Change_Param(const int &Param_Index, const int Value);
  void Change_All_Params(std::vector<int> Value);

  int Frame_Interp_Type;

  int Location_Accum;
  int Current_Frame;
  int Location_Diff;

  bool display_on;

  // for program optimizing
  Prog_Durations Time_Delay[10];
};
