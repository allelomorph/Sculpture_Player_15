
#pragma once

#include <vector>


std::string Convert_Hours_Mins_To_Str(const int &Hours_In, const int &Minutes_In  );

std::string Convert_Hours_Mins_Seconds_To_Str(const int &Hours_In, const int &Minutes_In,  const int &Seconds_In );

std::vector <int> Convert_Str_Time_To_Int_Vec(const std::string &Time_In );

std::string Convert_RGB_To_Hex_String(const std::vector<int> &RGB_In);

