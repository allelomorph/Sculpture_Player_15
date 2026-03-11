
#include <iostream>

#include "defines.h"
#include "function_misc.h"
#include "player_class_UMat.h"
#include "file_io_2.h"
#include "globals.h"

using namespace std;
// using namespace cv;

// type alias available everywhere in this file
using Clock = std::chrono::steady_clock;

// for checking different program speeds and memory leaks
#define Speed_On 1
#define Shift_On 1
#define Gain_Black_On 1
#define Gamma_On 1
#define Sat_Hue_On 1
#define Filters_On 1

enum PL_Param_Index
{
  GAIN = 0,
  BLACK_LEVEL = 1,
  COLOR_GAIN = 2,
  COLOR_HUE = 3,
  IMAGE_GAMMA = 4,
  H_SHIFT = 5,
  SPEED = 6,
  FILTER_TYPE = 7
};

enum Default_Index
{
  DEFAULT = 0,
  LOWER_LIMIT = 1,
  UPPER_LIMIT = 2,
};

// general terminology: _F is float _U is UMAt _FU is both

//
Video_Player_With_Processing::Video_Player_With_Processing(void)
{
  // KEY:   F -> float type (vs unsigned char)     U ->  UMat (vs Mat)
  ImageInNew_U.create(IMAGE_ROWS, IMAGE_COLS, CV_8UC(3));
  ImageInOld_U.create(IMAGE_ROWS, IMAGE_COLS, CV_8UC(3));
  Temp_Read_Movie.create(IMAGE_ROWS, IMAGE_COLS, CV_8UC(3));

  Ones_FU.create(IMAGE_ROWS, IMAGE_COLS, CV_32FC(3));
  Ones_FU = cv::Scalar(255.0, 255.0, 255.0);

  Frame_Interp_Type = 1;
}

// fix last file thing etc  Last_Good_Filename
std::string Video_Player_With_Processing::Open_Image_File(const string &Image_File_Name_In, const string &Name_In) // no parameters (load parameters after)
{
  PL_Params =
      {
          100, // GAIN
          0,   // BLACK_LEVEL
          100, // COLOR_GAIN
          0,   // COLOR_HUE
          25,  // IMAGE_GAMMA
          512, // H_SHIFT
          100, // SPEED
          3    // FILTER_TYPE
      };

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
      return Last_Good_Filename;
    }

    // FILE OPENED AND VALID THOUGH COULD BE WRONG SIZE !!!!!
    else
    {
      cout << "File  " << Image_File_Name_In << " Opened  "
           << "  --  ";
      Last_Good_Filename = Image_File_Name_In;
    }

    capWidth = capMain.get(CAP_PROP_FRAME_WIDTH);
    capHeight = capMain.get(CAP_PROP_FRAME_HEIGHT);
    capLength = capMain.get(cv::CAP_PROP_FRAME_COUNT);

    if ((capWidth == IMAGE_COLS) && (capHeight == IMAGE_ROWS))
      Scale_Video = false;
    else
      Scale_Video = true;

    // FILE WRONG SIZE SCALE IT !!!!!!
    if (Scale_Video)
    {
      cout << "File is the wrong Image Size: Auto Scaled" << endl
           << endl;
      capMain.read(ImageInUnscaled_U); // increments current frame
      resize(ImageInUnscaled_U, ImageInOld_U, ImageInOld_U.size(), 0, 0, 1);
      capMain.read(ImageInUnscaled_U); // increments current frame
      resize(ImageInUnscaled_U, ImageInNew_U, ImageInNew_U.size(), 0, 0, 1);
      return Image_File_Name_In;
    }

    // EVERYTHING CORRECT  !!!!!
    else
    {
      cout << "Correct Image Size " << endl
           << endl;
      capMain.read(ImageInOld_U); // increments current frame
      capMain.read(ImageInNew_U); // increments current frame
      return Image_File_Name_In;
    }
  }
}



std::string Video_Player_With_Processing::Open_Image_File_Rev2(const string &Image_File_Name_In, const string &Name_In) // no parameters (load parameters after)
{

  string File_To_Open;

  PL_Params =
      {
          100, // GAIN
          0,   // BLACK_LEVEL
          100, // COLOR_GAIN
          0,   // COLOR_HUE
          25,  // IMAGE_GAMMA
          512, // H_SHIFT
          100, // SPEED
          3    // FILTER_TYPE
      };

  player_pause = false;
  display_name = Name_In;

  Location_Accum = 0;
  Location_Diff = 0;
  Current_Frame = 0;

  cout << endl;

  // FILE DOES NOT EXIST AT ALL  !!!!!
  if (!std::filesystem::exists(Image_File_Name_In))
  {

    Path_Parsed Current_Path = Parse_Filname_Path(Image_File_Name_In);
    string File_New_Location = Find_File(Current_Path.Path_Only, Current_Path.Filename);

    if (File_New_Location == "")
    {
      cout << endl
           << "File  " << Image_File_Name_In << " does NOT EXIST (Loaded Default Movie) " << Default_Movie_With_Path << " --  " << endl;
      File_To_Open = Default_Movie_With_Path;
    }
    else
    {
      cout << endl
           << "File  " << Image_File_Name_In << " Found in another folder  " << File_New_Location << " --  " << endl;
      File_To_Open = File_New_Location;
    }

    // cout << " File_New_Location  " << File_New_Location << endl;

    capMain.open(File_To_Open);
    return File_To_Open;
  }

  // WRONG FILE EXTENSIOM !!!!!
  else if (!((Image_File_Name_In.substr(Image_File_Name_In.size() - 3) == "mov") ||
             (Image_File_Name_In.substr(Image_File_Name_In.size() - 3) == "mp4") ||
             (Image_File_Name_In.substr(Image_File_Name_In.size() - 3) == "avi")))
  {
    cout << endl
         << "File  " << Image_File_Name_In << " is the WRONG FILETYPE  (Loaded Default Movie) " << Default_Movie_With_Path << " --  ";
    capMain.open(Default_Movie_With_Path);
    return Default_Movie_With_Path;
  }
  else
  {
    // OPEN FILE !!!!!
    bool Check_File_Type = capMain.open(Image_File_Name_In);

    // FILE NOT UNDERSTOOD BY CODEC  !!!!!
    if (Check_File_Type == 0)
    {
      cout << endl
           << "File  " << Image_File_Name_In << " is CORRUPT (Loaded Default Movie) " << Default_Movie_With_Path << " Check_File_Type  " << Check_File_Type << "  ";
      capMain.open(Default_Movie_With_Path);
      return Default_Movie_With_Path;
    }

    // FILE OPENED AND VALID THOUGH COULD BE WRONG SIZE !!!!!
    else
    {
      cout << "File  " << Image_File_Name_In << " Opened  "
           << "  --  ";
    }

    capWidth = capMain.get(CAP_PROP_FRAME_WIDTH);
    capHeight = capMain.get(CAP_PROP_FRAME_HEIGHT);
    capLength = capMain.get(cv::CAP_PROP_FRAME_COUNT);

    if ((capWidth == IMAGE_COLS) && (capHeight == IMAGE_ROWS))
      Scale_Video = false;
    else
      Scale_Video = true;

    // FILE WRONG SIZE SCALE IT !!!!!!
    if (Scale_Video)
    {
      cout << "File is the wrong Image Size: Auto Scaled" << endl
           << endl;
      capMain.read(ImageInUnscaled_U); // increments current frame
      resize(ImageInUnscaled_U, ImageInOld_U, ImageInOld_U.size(), 0, 0, 1);
      capMain.read(ImageInUnscaled_U); // increments current frame
      resize(ImageInUnscaled_U, ImageInNew_U, ImageInNew_U.size(), 0, 0, 1);
      return Image_File_Name_In;
    }

    // EVERYTHING CORRECT  !!!!!
    else
    {
      cout << "Correct Image Size " << endl
           << endl;
      capMain.read(ImageInOld_U); // increments current frame
      capMain.read(ImageInNew_U); // increments current frame
      return Image_File_Name_In;
    }
  }
}

// fix last file thing etc  Last_Good_Filename
std::string Video_Player_With_Processing::Open_Image_File_And_Grab_Still(const string &Image_File_Name_In, const string &Name_In, Mat &Still, int32_t Still_Width)
{
  PL_Params =
      {
          100, // GAIN
          0,   // BLACK_LEVEL
          100, // COLOR_GAIN
          0,   // COLOR_HUE
          25,  // IMAGE_GAMMA
          512, // H_SHIFT
          100, // SPEED
          3    // FILTER_TYPE
      };

  Size size1(Still_Width, (Still_Width * 280) / 1024);

  player_pause = false;
  display_name = Name_In;

  Location_Accum = 0;
  Location_Diff = 0;
  Current_Frame = 0;

  // FILE DOES NOT EXIST AT ALL  !!!!!
  if (!std::filesystem::exists(Image_File_Name_In))
  {
    cout << endl
         << "File  " << Image_File_Name_In << " does NOT EXIST (Loaded Previous File) " << Last_Good_Filename << " --  ";
    cout << "  Last_Good_Filename  " << Last_Good_Filename << endl
         << endl;
    capMain.open(Last_Good_Filename);
    capMain.read(Still);
    resize(Still, Still, size1);
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
    capMain.read(Still);
    resize(Still, Still, size1);
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
      capMain.read(Still);
      resize(Still, Still, size1);
      return Last_Good_Filename;
    }

    // FILE OPENED AND VALID THOUGH COULD BE WRONG SIZE !!!!!
    else
    {
      cout << "File  " << Image_File_Name_In << " Opened  "
           << "  --  ";
      Last_Good_Filename = Image_File_Name_In;
    }

    capWidth = capMain.get(CAP_PROP_FRAME_WIDTH);
    capHeight = capMain.get(CAP_PROP_FRAME_HEIGHT);
    capLength = capMain.get(cv::CAP_PROP_FRAME_COUNT);

    if ((capWidth == IMAGE_COLS) && (capHeight == IMAGE_ROWS))
      Scale_Video = false;
    else
      Scale_Video = true;

    // FILE WRONG SIZE SCALE IT !!!!!!
    if (Scale_Video)
    {
      cout << "File is the wrong Image Size: Auto Scaled" << endl
           << endl;
      capMain.read(ImageInUnscaled_U); // increments current frame
      resize(ImageInUnscaled_U, ImageInOld_U, ImageInOld_U.size(), 0, 0, 1);
      capMain.read(ImageInUnscaled_U); // increments current frame
      resize(ImageInUnscaled_U, ImageInNew_U, ImageInNew_U.size(), 0, 0, 1);

      ImageInNew_U.copyTo(Still);
      resize(Still, Still, size1);

      return Image_File_Name_In;
    }

    // EVERYTHING CORRECT  !!!!!
    else
    {
      cout << "Correct Image Size " << endl
           << endl;
      capMain.read(ImageInOld_U); // increments current frame
      capMain.read(ImageInNew_U); // increments current frame
                                  //  Still = ImageInNew_U.clone();
      ImageInNew_U.copyTo(Still);
      resize(Still, Still, size1);

      return Image_File_Name_In;
    }
  }
}

std::string Video_Player_With_Processing::Open_Image_File_And_Grab_Still_Rev2(
    const string &Image_File_Name_In,
    const string &Name_In,
    Mat &Still,
    int32_t Still_Width)
{
  PL_Params =
      {
          100, // GAIN
          0,   // BLACK_LEVEL
          100, // COLOR_GAIN
          0,   // COLOR_HUE
          25,  // IMAGE_GAMMA
          512, // H_SHIFT
          100, // SPEED
          3    // FILTER_TYPE
      };

  Size size1(Still_Width, (Still_Width * 280) / 1024);

  string File_To_Open = "";

  player_pause = false;
  display_name = Name_In;

  Location_Accum = 0;
  Location_Diff = 0;
  Current_Frame = 0;

  // FILE DOES NOT EXIST AT ALL  !!!!!
  if (!std::filesystem::exists(Image_File_Name_In))
  {

    // look for it starting with current path
    Path_Parsed Current_Path = Parse_Filname_Path(Image_File_Name_In);
    string File_New_Location = Find_File(Current_Path.Path_Only, Current_Path.Filename);

    if (File_New_Location == "")
    {
      // LOOK FOR IT from the  TOP LEVEL MOVIE FOLDER
      File_New_Location = Find_File(Default_Main_Movie_Path, Current_Path.Filename);
      if (File_New_Location == "")
      {
        cout << endl
             << "File  " << Image_File_Name_In << " does NOT EXIST (Loaded Default Movie) " << Default_Movie_With_Path << " --  " << endl;
        File_To_Open = Default_Movie_With_Path;
      }
      else
      {
        cout << endl
             << "File  " << Image_File_Name_In << " Found in another folder  " << File_New_Location << " --  " << endl;
        File_To_Open = File_New_Location;
      }
    }
    else
    {
      cout << endl
           << "File  " << Image_File_Name_In << " Found in another folder  " << File_New_Location << " --  " << endl;
      File_To_Open = File_New_Location;
    }

    // cout << " File_New_Location  " << File_New_Location << endl;
    capMain.open(File_To_Open);
    capMain.read(Still);
    resize(Still, Still, size1);
    return File_To_Open;
  }

  // WRONG FILE EXTENSIOM !!!!!
  else if (!((Image_File_Name_In.substr(Image_File_Name_In.size() - 3) == "mov") ||
             (Image_File_Name_In.substr(Image_File_Name_In.size() - 3) == "mp4") ||
             (Image_File_Name_In.substr(Image_File_Name_In.size() - 3) == "avi")))
  {
    cout << endl
         << "File  " << Image_File_Name_In << " is the WRONG FILETYPE  (Loaded Default Movie) " << Default_Movie_With_Path << " --  ";
    capMain.open(Default_Movie_With_Path);
    capMain.read(Still);
    resize(Still, Still, size1);
    return Default_Movie_With_Path;
  }
  else
  {
    // OPEN FILE !!!!!
    bool Check_File_Type = capMain.open(Image_File_Name_In);

    // FILE NOT UNDERSTOOD BY CODEC  !!!!!
    if (Check_File_Type == 0)
    {
      cout << endl
           << "File  " << Image_File_Name_In << " is CORRUPT (Loaded Default Movie) " << Default_Movie_With_Path << " Check_File_Type  " << Check_File_Type << "  ";
      capMain.open(Default_Movie_With_Path);
      capMain.read(Still);
      resize(Still, Still, size1);
      return Default_Movie_With_Path;
    }

    // FILE OPENED AND VALID THOUGH COULD BE WRONG SIZE !!!!!
    else
    {
      cout << "File  " << Image_File_Name_In << " Opened  "
           << "  --  ";
    }

    capWidth = capMain.get(CAP_PROP_FRAME_WIDTH);
    capHeight = capMain.get(CAP_PROP_FRAME_HEIGHT);
    capLength = capMain.get(cv::CAP_PROP_FRAME_COUNT);

    if ((capWidth == IMAGE_COLS) && (capHeight == IMAGE_ROWS))
      Scale_Video = false;
    else
      Scale_Video = true;

    // FILE WRONG SIZE SCALE IT !!!!!!
    if (Scale_Video)
    {
      cout << "File is the wrong Image Size: Auto Scaled" << endl
           << endl;
      capMain.read(ImageInUnscaled_U); // increments current frame
      resize(ImageInUnscaled_U, ImageInOld_U, ImageInOld_U.size(), 0, 0, 1);
      capMain.read(ImageInUnscaled_U); // increments current frame
      resize(ImageInUnscaled_U, ImageInNew_U, ImageInNew_U.size(), 0, 0, 1);

      ImageInNew_U.copyTo(Still);
      resize(Still, Still, size1);

      return Image_File_Name_In;
    }

    // EVERYTHING CORRECT  !!!!!
    else
    {
      cout << "Correct Image Size " << endl
           << endl;
      capMain.read(ImageInOld_U); // increments current frame
      capMain.read(ImageInNew_U); // increments current frame
                                  //  Still = ImageInNew_U.clone();
      ImageInNew_U.copyTo(Still);
      resize(Still, Still, size1);

      return Image_File_Name_In;
    }
  }
}

std::string Video_Player_With_Processing::Open_Image_File_And_Grab_Still_Rev3(
    const std::string &Image_File_Name_In,
    const std::string &Name_In,
    cv::Mat &Still,
    int32_t Still_Width)
{
  // ---- Init state (same as before) ----
  PL_Params = {100, 0, 100, 0, 25, 512, 100, 3};
  const cv::Size thumb_size(Still_Width, (Still_Width * 280) / 1024);
  player_pause = false;
  display_name = Name_In;
  Location_Accum = 0;
  Location_Diff = 0;
  Current_Frame = 0;

  // ---- Helpers ----
  auto open_and_thumb = [&](const std::string &path)
  {
    capMain.open(path);
    capMain.read(Still);
    cv::resize(Still, Still, thumb_size);
    return path;
  };

  auto relocate_if_missing = [&](const std::string &original) -> std::string
  {
    Path_Parsed cur = Parse_Filname_Path(original);
    std::string found = Find_File(Default_Main_Movie_Path, cur.Filename); // SINGLE SEARCH
    if (!found.empty())
    {
      std::cout << "\nFile  " << original << " Found in another folder  " << found << " --\n";
      return found;
    }
    std::cout << "\nFile  " << original << " does NOT EXIST (Loaded Default Movie) "
              << Default_Movie_With_Path << " --\n";
    return Default_Movie_With_Path;
  };

  // ---- Choose candidate ----
  std::string candidate = Image_File_Name_In;
  if (!std::filesystem::exists(candidate))
  {
    candidate = relocate_if_missing(candidate);
    return open_and_thumb(candidate);
  }

  // ---- Try to open; fallback to default if codec can’t open ----
  if (!capMain.open(candidate))
  {
    std::cout << "\nFile  " << candidate << " is CORRUPT (Loaded Default Movie) "
              << Default_Movie_With_Path << " Check_File_Type  0  ";
    return open_and_thumb(Default_Movie_With_Path);
  }
  std::cout << "File  " << candidate << " Opened  --  ";

  // ---- Size handling ----
  capWidth = static_cast<int>(capMain.get(cv::CAP_PROP_FRAME_WIDTH));
  capHeight = static_cast<int>(capMain.get(cv::CAP_PROP_FRAME_HEIGHT));
  capLength = static_cast<int>(capMain.get(cv::CAP_PROP_FRAME_COUNT));

  Scale_Video = !((capWidth == IMAGE_COLS) && (capHeight == IMAGE_ROWS));

  if (Scale_Video)
  {
    std::cout << "File is the wrong Image Size: Auto Scaled\n\n";

    capMain.read(ImageInUnscaled_U); // frame 0
    cv::resize(ImageInUnscaled_U, ImageInOld_U, ImageInOld_U.size(), 0, 0, 1);

    capMain.read(ImageInUnscaled_U); // frame 1
    cv::resize(ImageInUnscaled_U, ImageInNew_U, ImageInNew_U.size(), 0, 0, 1);

    ImageInNew_U.copyTo(Still);
    cv::resize(Still, Still, thumb_size);
  }
  else
  {
    std::cout << "Correct Image Size \n\n";

    capMain.read(ImageInOld_U); // frame 0
    capMain.read(ImageInNew_U); // frame 1
    ImageInNew_U.copyTo(Still);
    cv::resize(Still, Still, thumb_size);
  }

  return candidate;
}

void Video_Player_With_Processing::Close_Image_File(void)
{
  capMain.release();
}

void Video_Player_With_Processing::Change_Param(const int &Param_Index, const int Value)
{
  PL_Params[Param_Index] = std::clamp(Value, PL_Default_Limits[Param_Index][LOWER_LIMIT], PL_Default_Limits[Param_Index][UPPER_LIMIT]);
}

// Change_Param(const int & Param_Index, const string & Value);
void Video_Player_With_Processing::Change_All_Params(std::vector<int> Value)
{
  for (std::size_t i = 0; i < Value.size() && i < PL_Default_Limits.size(); ++i)
  {
    PL_Params[i] = std::clamp(Value[i], PL_Default_Limits[i][LOWER_LIMIT], PL_Default_Limits[i][UPPER_LIMIT]);
  }
}

inline void Video_Player_With_Processing::Grab_Frame_From_File(cv::UMat &ImageOut, const int &Frame_Interp_Type)
{

  // 100 (percent) is normal speed
#ifdef Speed_On

  if (!player_pause)
  {
    while (Location_Accum >= 100)
    {
      Location_Accum -= 100;
      ImageInNew_U.copyTo(ImageInOld_U);

      if (Scale_Video)
      {
        capMain >> ImageInUnscaled_U; // increments current frame

        if (ImageInUnscaled_U.empty())
        {
          capLength = capMain.get(cv::CAP_PROP_POS_FRAMES); // change the length to the last good frame of corrupt file
          capMain.set(cv::CAP_PROP_POS_FRAMES, 0);
          capMain >> ImageInUnscaled_U;
        }
        resize(ImageInUnscaled_U, ImageInNew_U, ImageInNew_U.size(), 0, 0, 1);
      }
      else
      {
        capMain >> ImageInNew_U; // increments current frame
        if (ImageInNew_U.empty())
        {
          capLength = capMain.get(cv::CAP_PROP_POS_FRAMES); // change the length to the last good frame of corrupt file
          capMain.set(cv::CAP_PROP_POS_FRAMES, 0);
          capMain >> ImageInNew_U;
        }
      }

      if (capMain.get(cv::CAP_PROP_POS_FRAMES) >= capLength)
        capMain.set(cv::CAP_PROP_POS_FRAMES, 0);
    }

    Location_Diff = Location_Accum % 100;
    if (Frame_Interp_Type == 0)
    {
      if ((1 - (float)(Location_Diff) / 100) > .5)
        ImageInOld_U.copyTo(ImageOut);
      else
        ImageInNew_U.copyTo(ImageOut);
    }
    else
      addWeighted(ImageInOld_U, 1 - (float)(Location_Diff) / 100, ImageInNew_U, (float)(Location_Diff) / 100, 0.0, ImageOut);

    Location_Accum += PL_Params[SPEED];
  }

#else

  if (!player_pause)
  {

    ImageInNew_U.copyTo(ImageInOld_U);

    if (Scale_Video)
    {
      capMain >> ImageInUnscaled_U; // increments current frame

      if (ImageInUnscaled_U.empty())
      {
        capLength = capMain.get(cv::CAP_PROP_POS_FRAMES); // change the length to the last good frame of corrupt file
        capMain.set(cv::CAP_PROP_POS_FRAMES, 0);
        capMain >> ImageInUnscaled_U;
      }
      resize(ImageInUnscaled_U, ImageInNew_U, ImageInNew_U.size(), 0, 0, 1);
    }
    else
    {
      capMain >> ImageInNew_U; // increments current frame
      if (ImageInNew_U.empty())
      {
        capLength = capMain.get(cv::CAP_PROP_POS_FRAMES); // change the length to the last good frame of corrupt file
        capMain.set(cv::CAP_PROP_POS_FRAMES, 0);
        capMain >> ImageInNew_U;
      }
    }

    if (capMain.get(cv::CAP_PROP_POS_FRAMES) >= capLength)
      capMain.set(cv::CAP_PROP_POS_FRAMES, 0);

    ImageInNew_U.copyTo(ImageOut);
  }

#endif
}

inline void Video_Player_With_Processing::Shift_Horizontal_Location(cv::UMat &Image_In_Out, int &Initial_Location)
{

  // note by incrementing the location the movie can shift around in real time.

  int Location_Wrap = Initial_Location % IMAGE_COLS;

  int ROI_Width_1 = IMAGE_COLS - Location_Wrap;
  int ROI_Height = IMAGE_ROWS;
  int ROI_Width_2 = IMAGE_COLS - ROI_Width_1;

  Rect Rect_Before_1(0, 0, ROI_Width_1, ROI_Height);
  Rect Rect_Before_2(ROI_Width_1, 0, ROI_Width_2, ROI_Height);

  Rect Rect_After_1(ROI_Width_2, 0, ROI_Width_1, ROI_Height);
  Rect Rect_After_2(0, 0, ROI_Width_2, ROI_Height);

  UMat ROI_Before_1 = Image_In_Out(Rect_Before_1).clone();
  UMat ROI_Before_2 = Image_In_Out(Rect_Before_2).clone();

  UMat ROI_After_1 = Image_In_Out(Rect_After_2); // Get the header to the destination position
  UMat ROI_After_2 = Image_In_Out(Rect_After_1); // Get the header to the destination position

  ROI_Before_1.copyTo(ROI_After_2);
  ROI_Before_2.copyTo(ROI_After_1);
}

inline void Video_Player_With_Processing::Shift_Horizontal_Location(const cv::UMat &Image_In, cv::UMat &Image_Shifted, int &Initial_Location)
{
  // note by incrementing the location the movie can shift around in real time.
  // need to figure out how to do this.  Think I need an image class or structure

  Image_In.copyTo(Image_Shifted);

  int Location_Wrap = Initial_Location % IMAGE_COLS;

  int ROI_Width_1 = IMAGE_COLS - Location_Wrap;
  int ROI_Height = IMAGE_ROWS;
  int ROI_Width_2 = IMAGE_COLS - ROI_Width_1;

  Rect Rect_Before_1(0, 0, ROI_Width_1, ROI_Height);
  Rect Rect_Before_2(ROI_Width_1, 0, ROI_Width_2, ROI_Height);

  Rect Rect_After_1(ROI_Width_2, 0, ROI_Width_1, ROI_Height);
  Rect Rect_After_2(0, 0, ROI_Width_2, ROI_Height);

  UMat ROI_Before_1 = Image_Shifted(Rect_Before_1).clone();
  UMat ROI_Before_2 = Image_Shifted(Rect_Before_2).clone();

  UMat ROI_After_1 = Image_Shifted(Rect_After_2); // Get the header to the destination position
  UMat ROI_After_2 = Image_Shifted(Rect_After_1); // Get the header to the destination position

  ROI_Before_1.copyTo(ROI_After_2);
  ROI_Before_2.copyTo(ROI_After_1);
}

inline void Video_Player_With_Processing::MultiFilter(cv::UMat &Image, const int &FilterTypeIn)
{
  string FilterTypeStr = "";
  if (FilterTypeIn == 0)
    FilterTypeStr = "";
  else if (FilterTypeIn == 1)
    FilterTypeStr = "3";
  else if (FilterTypeIn == 2)
    FilterTypeStr = "5";
  else if (FilterTypeIn == 3)
    FilterTypeStr = "35";
  else if (FilterTypeIn == 4)
    FilterTypeStr = "235";
  else if (FilterTypeIn == 5)
    FilterTypeStr = "2357";
  else if (FilterTypeIn == 6)
    FilterTypeStr = "23457";
  else
    FilterTypeStr = "234567";

  if (FilterTypeStr.find('2') != std::string::npos)
    blur(Image, Image, Size(2, 2));
  if (FilterTypeStr.find('3') != std::string::npos)
    blur(Image, Image, Size(3, 3));
  if (FilterTypeStr.find('4') != std::string::npos)
    blur(Image, Image, Size(4, 4));
  if (FilterTypeStr.find('5') != std::string::npos)
    blur(Image, Image, Size(5, 5));
  if (FilterTypeStr.find('6') != std::string::npos)
    blur(Image, Image, Size(6, 6));
  if (FilterTypeStr.find('7') != std::string::npos)
    blur(Image, Image, Size(7, 7));
}

void Video_Player_With_Processing::FusedNonSpatialPass()
{
  cv::Mat m = ImageMain_FU.getMat(cv::ACCESS_RW); // map UMat once

  // Precompute flags and scalars so the inner loop stays lean
  const bool do_gain_black = (PL_Params[GAIN] != 100) || (PL_Params[BLACK_LEVEL] != 0);
  const bool invert = PL_Params[GAIN] < 0;
  const float gain_abs = std::abs(PL_Params[GAIN]) / 100.0f;                 // your scale
  const float black_add = 2.0f * static_cast<float>(PL_Params[BLACK_LEVEL]); // your offset in 0..255 domain

  const bool do_gamma = (PL_Params[IMAGE_GAMMA] != 0);
  // Your current gamma is a blend: out = (1 - t)*x + t*(x^2 / 255)
  const float t = static_cast<float>(PL_Params[IMAGE_GAMMA]) / 100.0f; // 0.0–1.0

  const bool do_color = (PL_Params[COLOR_GAIN] != 100) || (PL_Params[COLOR_HUE] != 0);
  const float hue = static_cast<float>(PL_Params[COLOR_HUE]) * (3.1415926535f / 180.0f);
  const float cg = static_cast<float>(PL_Params[COLOR_GAIN]) / 100.0f;
  const float ccos = cg * std::cos(hue);
  const float ssin = cg * std::sin(hue);

  // G-from-RY/BY reconstruction constants you used
  const float kGY_RY = -0.5093f;
  const float kGY_BY = -0.1942f;

  cv::parallel_for_(cv::Range(0, m.rows), [&](const cv::Range &r)
                    {
        for (int y = r.start; y < r.end; ++y)
        {
            cv::Vec3f* row = m.ptr<cv::Vec3f>(y);
            for (int x = 0; x < m.cols; ++x)
            {
                float B = row[x][0];
                float G = row[x][1];
                float R = row[x][2];

                // Gain/black (and optional invert) in 0..255 domain
                // if (do_gain_black)
                {
                    if (invert) { R = 255.f - R; G = 255.f - G; B = 255.f - B; }
                    R = R * gain_abs + black_add;
                    G = G * gain_abs + black_add;
                    B = B * gain_abs + black_add;
                }

                // Gamma (your square+blend curve)
                // if (do_gamma)
                {
                    // x^2/255
                    float R2 = (R * R) * (1.0f / 255.0f);
                    float G2 = (G * G) * (1.0f / 255.0f);
                    float B2 = (B * B) * (1.0f / 255.0f);
                    R = (1.0f - t) * R + t * R2;
                    G = (1.0f - t) * G + t * G2;
                    B = (1.0f - t) * B + t * B2;
                }

                // Hue/Sat via Y, RY, BY rotation
                // if (do_color)
                {
                    // Luma
                    float Y  = 0.299f * R + 0.587f * G + 0.114f * B;
                    float RY = R - Y;
                    float BY = B - Y;

                    // rotate + gain
                    float RYp = ccos * RY +  ssin * BY;
                    float BYp = ccos * BY + (-ssin) * RY;

                    // reconstruct R, B
                    float Rp = Y + RYp;
                    float Bp = Y + BYp;

                    // reconstruct G via GY then add Y
                    float GYp = kGY_RY * RYp + kGY_BY * BYp;
                    float Gp  = Y + GYp;

                    R = Rp; G = Gp; B = Bp;
                }

                // Clamp and store (still 0..255 float)
                R = std::min(255.0f, std::max(0.0f, R));
                G = std::min(255.0f, std::max(0.0f, G));
                B = std::min(255.0f, std::max(0.0f, B));

                row[x] = cv::Vec3f(B, G, R);
            }
        } });
}

// inline void Video_Player_With_Processing::RGB_Gamma_Correction(cv::UMat &Image, const int &Gamma_Correction)
// {
//   multiply(Image, Image, Image_squared); // square
//   addWeighted(Image, 1 - (float)Gamma_Correction / 100, Image_squared, .003921 * (float)Gamma_Correction / 100, 0.0, Image);
// }

// inline void Video_Player_With_Processing::RGB_Hue_Saturation_Correction(const cv::UMat &Image, const int &Color_Gain_In, const int &Color_Hue_In)
// {
//   // note  tried about 10 different methods and this was the fastest and lowest leakage
//   // general terminology: _F is float _U is UMAt _FU is both

//   // hue and sat coefs
//   float Color_Hue = (float)Color_Hue_In * (3.1416 / 180);
//   float Color_Cos_Mul = (float)Color_Gain_In / 100 * cosf32(Color_Hue);
//   float Color_Sin_Mul = (float)Color_Gain_In / 100 * sinf32(Color_Hue);

//   // convert to Y RY(R-Y) BY(B-Y)
//   // rotate RY and BY
//   // add New RY and BY to Y to get new R and B
//   // generate new G-Y
//   // add new GY to Y to get new G

//   // calculate Y
//   extractChannel(Image, BGR_Separate[2], 2);
//   extractChannel(Image, BGR_Separate[1], 1);
//   extractChannel(Image, BGR_Separate[0], 0);
//   addWeighted(BGR_Separate[2], .299, BGR_Separate[1], .587, 0.0, Y_RG_Partial_FU1);
//   addWeighted(Y_RG_Partial_FU1, 1, BGR_Separate[0], .114, 0.0, Y_From_RGB_FU1);

//   // calculate color difference RY & BY
//   subtract(BGR_Separate[2], Y_From_RGB_FU1, RY_From_Scratch_FU1);
//   subtract(BGR_Separate[0], Y_From_RGB_FU1, BY_From_Scratch_FU1);

//   // Hue Shift and Chroma Gain
//   addWeighted(RY_From_Scratch_FU1, Color_Cos_Mul, BY_From_Scratch_FU1, Color_Sin_Mul, 0.0, RY_New_FU1);
//   addWeighted(BY_From_Scratch_FU1, Color_Cos_Mul, RY_From_Scratch_FU1, -Color_Sin_Mul, 0.0, BY_New_FU1);

//   // Create BGR again
//   // calculate new R
//   addWeighted(RY_New_FU1, 1, Y_From_RGB_FU1, 1, 0.0, BGR_Sep_FU[2]);
//   // calculate new B
//   addWeighted(BY_New_FU1, 1, Y_From_RGB_FU1, 1, 0.0, BGR_Sep_FU[0]);
//   // // calculate GY and then new G
//   addWeighted(RY_New_FU1, -.5093, BY_New_FU1, -.1942, 0.0, GY_New_FU1);
//   addWeighted(GY_New_FU1, 1, Y_From_RGB_FU1, 1, 0.0, BGR_Sep_FU[1]);

//   // convert UMat's to Mat's for merging
//   BGR_Sep_FU[0].copyTo(BGR_Sep_F[0]);
//   BGR_Sep_FU[1].copyTo(BGR_Sep_F[1]);
//   BGR_Sep_FU[2].copyTo(BGR_Sep_F[2]);
//   merge(BGR_Sep_F, 3, BGR_Corrected_F);
//   // convert merged back to UMat
//   BGR_Corrected_F.copyTo(ImageMain_FU);
// }


bool Video_Player_With_Processing::Process(bool para_version)
{

  bool PrintOn = false;

  static int loop = 0;

  auto total_start = Clock::now();

  auto grab_start = Clock::now();

  // general terminology: _F is float _U is UMAt _FU is both
  Grab_Frame_From_File(ImageIn_U, 1);

#ifdef Shift_On
  if (PL_Params[H_SHIFT] != 0)
  {
    Shift_Horizontal_Location(ImageIn_U, ImageShifted_U, PL_Params[H_SHIFT]);
    // convert to float for more accuracy
    ImageShifted_U.convertTo(ImageMain_FU, CV_32FC3);
  }
  else
    ImageIn_U.convertTo(ImageMain_FU, CV_32FC3); // convert to float for more accuracy
#else
  ImageIn_U.copyTo(ImageShifted_U);
#endif

  auto grab_delta = GetDeltaTime(grab_start);
  auto proc_start = Clock::now();

  // gain and black level
  // if (!para_version)

  // {
  //   if (PL_Params[GAIN] < 0)
  //     subtract(Ones_FU, ImageMain_FU, ImageMain_FU);

  //   ImageMain_FU.convertTo(ImageMain_FU, -1, ((float)abs(PL_Params[GAIN])) / 100, 2 * ((float)PL_Params[BLACK_LEVEL]));

  //   RGB_Gamma_Correction(ImageMain_FU, PL_Params[IMAGE_GAMMA]);

  //   RGB_Hue_Saturation_Correction(ImageMain_FU, PL_Params[COLOR_GAIN], PL_Params[COLOR_HUE]);
  // }

  // else
  FusedNonSpatialPass();

  // auto end = Clock::now();
  // auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

  auto proc_delta = GetDeltaTime(proc_start);
  auto filt_start = Clock::now();

// include tap number in type  37 or 73 for example is a 3 tap and a 7 tap
// filter last to clean up artifacts from other functions
#ifdef Filters_On
  if (PL_Params[FILTER_TYPE] != 0)
    MultiFilter(ImageMain_FU, PL_Params[FILTER_TYPE]);
#endif

  auto filt_delta = GetDeltaTime(filt_start);
  auto convert_start = Clock::now();

  // convert back to 8 bits unsigned integer  for the display
  ImageMain_FU.convertTo(ImageTemp_U, CV_8UC3);
  // make a copy for the display in 8 bit Mat vs UMat
  ImageTemp_U.copyTo(VideoDisplay);

  auto convert_delta = GetDeltaTime(convert_start);

  auto total_delta = GetDeltaTime(total_start);

  loop++;
  if (loop % 20 == 0)
  {
    cout << "\ngrab_delta " << grab_delta
         << "   proc_delta " << proc_delta
         << "   filt_delta " << filt_delta
         << "   convert " << convert_delta
         << "   total " << total_delta << endl;
    PrintOn = true;
  }
  else
    PrintOn = false;

  return PrintOn;
}
