
#pragma once


using namespace std;


constexpr int main_loop_target_frame_us = 33333; // 30

constexpr int fps = 30; // 30

const int SCREEN_IMAGE_ROWS = 280;
const int SCREEN_IMAGE_COLS = 1024;

const int SCULPTURE_IMAGE_ROWS = 70;
const int SCULPTURE_IMAGE_COLS = 256;



enum Player_Param_Index
{
  GAIN = 0,
  BLACK_LEVEL = 1,
  COLOR_GAIN = 2,
  COLOR_HUE = 3,
  IMAGE_GAMMA = 4,
  H_SHIFT = 5,
  ROTATE = 6,  
  SPEED = 7,
  FILTER_TYPE = 8,

  PAUSE_TOGGLE = 9,
  FAST_FORWARD = 10,
  REWIND = 11

};

  enum Default_Limits_Index
  {
    DEFAULT = 0,
    LOWER_LIMIT = 1,
    UPPER_LIMIT = 2,
  };









