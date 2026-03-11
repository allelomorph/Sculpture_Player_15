#include <string>
#include <sstream>
#include <iostream>

#include "string_utils.h"
#include "function_misc.h"

using namespace std;


std::string Convert_Hours_Mins_To_Str(const int &Hours_In, const int &Minutes_In)
{
  std::string Time_Str = "";
  if ((Hours_In <= 23) && (Minutes_In <= 59) )
  {
    Time_Str =  ( ( Hours_In < 10   ) ?  " "  : "" ) + std::to_string(Hours_In) + ":"   ;
    Time_Str += ( ( Minutes_In < 10 ) ?  "0"  : "" ) + std::to_string(Minutes_In) ;
  }
    return Time_Str;
}


std::string Convert_Hours_Mins_Seconds_To_Str(const int &Hours_In, const int &Minutes_In,  const int &Seconds_In )
{
  std::string Time_Str = "";
  if ((Hours_In <= 23) && (Minutes_In <= 59) &&  (Seconds_In <= 59) )
  {
    Time_Str =  ( ( Hours_In < 10   ) ?  " "  : "" ) + std::to_string(Hours_In) + ":"   ;
    Time_Str += ( ( Minutes_In < 10 ) ?  "0"  : "" ) + std::to_string(Minutes_In) + ":" ;
    Time_Str += ( ( Seconds_In < 10 ) ?  "0"  : "" ) + std::to_string(Seconds_In) ;
  }
    return Time_Str;
}


// // Hours / Minutes / Total
std::vector<int> Convert_Str_Time_To_Int_Vec(const std::string &Time_In)
{
  std::string Hour_Temp, Min_Temp;

  std::vector Time_Out{100, 100, 100000};

  int loc_colon = Time_In.find_last_of(":");

  if ((loc_colon != -1) && (loc_colon <= 2) && (loc_colon != 0))
  {
    Hour_Temp = Check_Numeric(Time_In.substr(0, loc_colon), "-1");
    Min_Temp = Check_Numeric(Time_In.substr(loc_colon + 1), "-1");

    if ((Hour_Temp != "-1") && (Min_Temp != "-1"))
      if ((stoi(Hour_Temp) <= 23) && (stoi(Min_Temp) <= 59))
        Time_Out = {stoi(Hour_Temp), stoi(Min_Temp), stoi(Hour_Temp) * 3600 + stoi(Min_Temp)};
  }
  return Time_Out;
}


std::string Convert_RGB_To_Hex_String(const std::vector<int> &RGB_In)
{
  std::stringstream ss;

  if (RGB_In[0] == 0)
    ss << "00";
  else
  {
    if (RGB_In[0] <= 15)
      ss << "0";
    ss << hex << RGB_In[0];
  }

  if (RGB_In[1] == 0)
    ss << "00";
  else
  {
    if (RGB_In[1] <= 15)
      ss << "0";
    ss << hex << RGB_In[1];
  }

  if (RGB_In[2] == 0)
    ss << "00";
  else
  {
    if (RGB_In[2] <= 15)
      ss << "0";
    ss << hex << RGB_In[2];
  }

  return ss.str();
}
