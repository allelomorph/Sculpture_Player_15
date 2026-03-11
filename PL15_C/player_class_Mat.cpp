
#include <iostream>

#include "globals.h"
#include "function_misc.h"
#include "player_class_Mat.h"
#include "fs_utils.h"
#include "globals.h"
#include "image_processor.h"

using namespace std;
// using namespace cv;

// type alias available everywhere in this file
using Clock = std::chrono::steady_clock;

// general terminology: _F is float _U is UMAt _FU is both _M is Mat

//
Video_Player_With_Processing::Video_Player_With_Processing(void)
    : Player_Proc(SCREEN_IMAGE_ROWS, SCREEN_IMAGE_COLS) // <-- construct the ImageProcessor here
{
  // KEY:   F -> float type (vs unsigned char)     U ->  UMat (vs Mat)
  // ImageInNew_U.create(IMAGE_ROWS, IMAGE_COLS, CV_8UC(3));
  // ImageInOld_U.create(IMAGE_ROWS, IMAGE_COLS, CV_8UC(3));

  Temp_Read_Movie.create(SCREEN_IMAGE_ROWS, SCREEN_IMAGE_COLS, CV_8UC(3));

  // Ones_FU.create(IMAGE_ROWS, IMAGE_COLS, CV_32FC(3));
  // Ones_FU = cv::Scalar(255.0, 255.0, 255.0);

  ImageInNew_M.create(SCREEN_IMAGE_ROWS, SCREEN_IMAGE_COLS, CV_8UC(3));
  ImageInOld_M.create(SCREEN_IMAGE_ROWS, SCREEN_IMAGE_COLS, CV_8UC(3));

  Ones_FM.create(SCREEN_IMAGE_ROWS, SCREEN_IMAGE_COLS, CV_32FC(3));
  Ones_FM = cv::Scalar(255.0, 255.0, 255.0);

  Frame_Interp_Type = 1;
}

// fix last file thing etc  Last_Good_Filename
std::string Video_Player_With_Processing::Open_Image_File_Mat(const string &Image_File_Name_In, const string &Name_In) // no parameters (load parameters after)
{

  Player_Params = Player_Params_Defaults;

  player_pause = false;
  display_name = Name_In;

  Location_Accum = 0;
  Location_Diff = 0;
  Current_Frame = 0;

  cout << endl;

  // FILE DOES NOT EXIST AT ALL  !!!!!
  if (!std::filesystem::exists(Image_File_Name_In))
  {
    cout << endl
         << "File  " << Image_File_Name_In << " does NOT EXIST (Loaded Previous File) " << Last_Good_Filename << " --  ";
    cout << "  Last_Good_Filename  " << Last_Good_Filename << endl
         << endl;
    capMain.open(Last_Good_Filename);
    Duration_Frames = static_cast<int>(capMain.get(cv::CAP_PROP_FRAME_COUNT));
    return Last_Good_Filename;
  }

  // WRONG FILE EXTENSIOM !!!!!
  else if (!((Image_File_Name_In.substr(Image_File_Name_In.size() - 3) == "mov") ||
             (Image_File_Name_In.substr(Image_File_Name_In.size() - 3) == "mp4") ||
             (Image_File_Name_In.substr(Image_File_Name_In.size() - 3) == "avi")))
  {
    cout << endl
         << "File  " << Image_File_Name_In << " is the WRONG FILETYPE (Loaded Previous File) " << Last_Good_Filename << " --  ";
    capMain.open(Last_Good_Filename);
    Duration_Frames = static_cast<int>(capMain.get(cv::CAP_PROP_FRAME_COUNT));
    return Last_Good_Filename;
  }
  else
  {
    // OPEN FILE !!!!!
    bool Check_File_Type = capMain.open(Image_File_Name_In);

    // FILE NOT UNDERSTOOD BY CODEC  !!!!!
    if (Check_File_Type == 0)
    {
      cout << endl
           << "File  " << Image_File_Name_In << " is CORRUPT (Loaded Previous File) " << Last_Good_Filename << " Check_File_Type  " << Check_File_Type << "  ";
      capMain.open(Last_Good_Filename);
      Duration_Frames = static_cast<int>(capMain.get(cv::CAP_PROP_FRAME_COUNT));
      return Last_Good_Filename;
    }
    else if( static_cast<int>(capMain.get(cv::CAP_PROP_FRAME_COUNT)) <= 0 )
    {
      capMain.open(Last_Good_Filename);
      Duration_Frames = static_cast<int>(capMain.get(cv::CAP_PROP_FRAME_COUNT));
      return Last_Good_Filename;
    }

    // FILE OPENED AND VALID THOUGH COULD BE WRONG SIZE !!!!!
    else
    {
      cout << "File  " << Image_File_Name_In << " Opened  "
           << "  --  ";
      Last_Good_Filename = Image_File_Name_In;
      Duration_Frames = static_cast<int>(capMain.get(cv::CAP_PROP_FRAME_COUNT));
    }

    capWidth = capMain.get(CAP_PROP_FRAME_WIDTH);
    capHeight = capMain.get(CAP_PROP_FRAME_HEIGHT);
    capLength = capMain.get(cv::CAP_PROP_FRAME_COUNT);

    if ((capWidth == SCREEN_IMAGE_COLS) && (capHeight == SCREEN_IMAGE_ROWS))
      Scale_Video = false;
    else
      Scale_Video = true;

    // FILE WRONG SIZE SCALE IT !!!!!!
    if (Scale_Video)
    {
      cout << "File is the wrong Image Size: Auto Scaled" << endl
           << endl;
      capMain.read(ImageInUnscaled_M); // increments current frame
      resize(ImageInUnscaled_M, ImageInOld_M, ImageInOld_M.size(), 0, 0, 1);
      capMain.read(ImageInUnscaled_M); // increments current frame
      resize(ImageInUnscaled_M, ImageInNew_M, ImageInNew_M.size(), 0, 0, 1);
      return Image_File_Name_In;
    }

    // EVERYTHING CORRECT  !!!!!
    else
    {
      cout << "Correct Image Size " << endl
           << endl;
      capMain.read(ImageInOld_M); // increments current frame
      capMain.read(ImageInNew_M); // increments current frame
      return Image_File_Name_In;
    }
  }
}



void Video_Player_With_Processing::Close_Image_File(void)
{
  capMain.release();
}

void Video_Player_With_Processing::Change_Param(const int &Param_Index, const int Value)
{
  Player_Params[Param_Index] = std::clamp(Value, Player_Default_Limits[Param_Index][LOWER_LIMIT], Player_Default_Limits[Param_Index][UPPER_LIMIT]);
}

// Change_Param(const int & Param_Index, const string & Value);
void Video_Player_With_Processing::Change_All_Params(std::vector<int> params_in)
{
  for (std::size_t i = 0; i < params_in.size() && i < Player_Default_Limits.size(); ++i)
  {
    Player_Params[i] = std::clamp(params_in[i], Player_Default_Limits[i][LOWER_LIMIT], Player_Default_Limits[i][UPPER_LIMIT]);
  }
}

// includes slo-mo
inline void Video_Player_With_Processing::Grab_Frame_From_File_Mat(cv::Mat &ImageOut,
                                                                   const int &Frame_Interp_Type)
{
    if (!player_pause)
    {
        while (Location_Accum >= 100)
        {
            const int jump_value = 100;
            double current_frame = capMain.get(cv::CAP_PROP_POS_FRAMES);

            if ((Player_Params[FAST_FORWARD] == 1) && (current_frame + jump_value < capLength))
            {
                capMain.set(cv::CAP_PROP_POS_FRAMES, current_frame + jump_value);
                Player_Params[FAST_FORWARD] = 0;
            }

            if (Player_Params[REWIND] == 1)
            {
                if (current_frame > jump_value) capMain.set(cv::CAP_PROP_POS_FRAMES, current_frame - jump_value);
                else                            capMain.set(cv::CAP_PROP_POS_FRAMES, 0);
                Player_Params[REWIND] = 0;
            }

            Location_Accum -= 100;
            ImageInNew_M.copyTo(ImageInOld_M);

            capMain >> ImageInUnscaled_M;
            if (ImageInUnscaled_M.empty())
            {
                capMain.set(cv::CAP_PROP_POS_FRAMES, 0);
                capMain >> ImageInUnscaled_M;
            }

            if (Scale_Video)
                resize(ImageInUnscaled_M, ImageInNew_M, ImageInNew_M.size(), 0, 0, 1);
            else
                ImageInNew_M = ImageInUnscaled_M;

            // keep frame counter meaningful for GUI: the frame we just loaded into ImageInNew_M
            Current_Frame = (int)capMain.get(cv::CAP_PROP_POS_FRAMES) - 1;
            if (Current_Frame < 0) Current_Frame = 0;

            // optional loop safety
            if (capMain.get(cv::CAP_PROP_POS_FRAMES) >= capLength)
                capMain.set(cv::CAP_PROP_POS_FRAMES, 0);
        }

        Location_Diff = Location_Accum % 100;

        if (Frame_Interp_Type == 0)
        {
            if ((1.0f - (float)Location_Diff / 100.0f) > 0.5f)
                ImageInOld_M.copyTo(ImageOut);
            else
                ImageInNew_M.copyTo(ImageOut);
        }
        else
        {
            addWeighted(ImageInOld_M, 1.0f - (float)Location_Diff / 100.0f,
                        ImageInNew_M, (float)Location_Diff / 100.0f,
                        0.0, ImageOut);
        }

        Location_Accum += Player_Params[SPEED];
    }

    // If paused, Current_Frame stays where it was last advanced (good)
}

void Video_Player_With_Processing::H_Shift_Rotate(const cv::Mat &ImageIn_U,
                                                  cv::Mat &ImageOut_U,
                                                  double Location_px,
                                                  bool Force_Location,
                                                  int Step_px)
{
  CV_Assert(ImageIn_U.type() == CV_8UC3 || ImageIn_U.type() == CV_32FC3);

  if (ImageIn_U.empty())
  {
    ImageOut_U.release();
    return;
  }
  const int W = ImageIn_U.cols;
  if (W <= 0)
  {
    ImageOut_U.release();
    return;
  }

  if (Force_Location)
    HShift_Offset_px_ = 0.0;
  else if (Step_px != 0)
    HShift_Offset_px_ += (double)(Step_px) / 10.0;

  const double Loc_Total = Location_px + HShift_Offset_px_;

  int Final_Shift = static_cast<int>(std::llround(Loc_Total)) % W;
  if (Final_Shift < 0)
    Final_Shift += W;

  if (Final_Shift == 0)
  {
    if (ImageIn_U.data != ImageOut_U.data)
      ImageIn_U.copyTo(ImageOut_U);
    return;
  }
  h_shift(ImageIn_U, ImageOut_U, Final_Shift);

  ImageOut_U =   ImageIn_U.clone();

}

void Video_Player_With_Processing::Process_New_Frame_Ext_Process(void)
{
  if (Player_Params[PAUSE_TOGGLE] == 0)
    Grab_Frame_From_File_Mat(ImageIn_M, 1);

  // Player_Params[H_ROTATE] = 100;  need to add rotate to the gui
  // Player_Params[ROTATE] = 10;

  H_Shift_Rotate(ImageIn_M, ImageIn_M, Player_Params[H_SHIFT], false, Player_Params[ROTATE]); // Player_Params[H_ROTATE]);

  // class general version    8 bits in float out
  ImageMain_FM = Player_Proc.Process_Image(ImageIn_M, Player_Params); // 3.1 - 4 ms

  // non class general version    8 bits in float out
  // ImageMain_FM = process_image(ImageIn_M, Player_Params);  // 3.1 - 4 ms

  // convert for displaying
  ImageMain_FM.convertTo(VideoDisplay, CV_8UC3);
}
