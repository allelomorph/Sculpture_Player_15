// fs_utils.h
#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>


struct Path_Parsed {
    std::string Path_Only;
    std::string Filename;
    std::string Filename_No_Ext;
    std::string Ext;
    int errors;   
};

// Declarations (if you want to separate .cpp/.h)
Path_Parsed Parse_Filename_Path(const std::string& Full_Path_In);
std::string Find_File(const std::string& initial_path, const std::string& filename);


// inlines in .h

// helper: check if line starts with '#'
inline bool is_full_line_comment(const std::string &s) {
    auto i = s.find_first_not_of(" \t\r");
    return (i != std::string::npos && s[i] == '#');
}


// templates in .h

// Reads 2D or 1D csv or text   works with float or int 
// Skips empty lines, skips non-numeric tokens, ignores lines starting with #.
// End result is 2D file
template <typename T>
bool Read_1D_Number(std::vector<T> &out, const std::string &file_name)
{
    out.clear();
    std::ifstream file(file_name);
    if (!file.is_open()) {
        std::cout << "\nError opening file " << file_name << std::endl;
        return false;
    }

    std::string line;
    T val{};

    while (std::getline(file, line))
    {
        if (is_full_line_comment(line)) continue;  // skip comments

        // normalize commas to spaces
        std::replace_if(line.begin(), line.end(),
                        [](char c){ return c == ','; }, ' ');

        std::istringstream iss(line);

        while (true) {
            if (iss >> val) {
                out.push_back(val);              // ✅ store value
            }
            else if (iss.fail()) {               // ❌ non-numeric token
                iss.clear();
                std::string dummy;
                if (!(iss >> dummy)) break;      // nothing left
                // else skip bad token
            }
            else {
                break;                           // finished line
            }
        }
    }
    return true;
}

// Reads 2D csv or text   works with float or int
// Skips empty lines, skips non-numeric tokens, ignores lines starting with #.
// End result is long 1D file
template <typename T>
bool Read_2D_Number(std::vector<std::vector<T>> &out, const std::string &file_name)
{
    out.clear();
    std::ifstream file(file_name);
    if (!file.is_open()) {
        std::cout << "\nError opening file " << file_name << std::endl;
        return false;
    }

    std::string line;
    std::vector<T> row;
    T val{};

    while (std::getline(file, line))
    {
        if (is_full_line_comment(line)) continue;  // only lines that start with '#'

        // normalize commas to spaces (tabs/spaces already OK as delimiters)
        std::replace_if(line.begin(), line.end(),
                        [](char c){ return c == ','; }, ' ');

        std::istringstream iss(line);
        row.clear();

        while (true) {
            if (iss >> val) {
                row.push_back(val);                 // parsed a number of type T
            } else if (iss.fail()) {
                iss.clear();
                std::string dummy;
                if (!(iss >> dummy)) break;        // no more tokens
                // else: skipped a non-numeric token (e.g., "EnclosureA-S")
            } else {
                break;                              // end of line
            }
        }

        if (!row.empty())
            out.push_back(row);                     // skip empty/header-only lines
    }
    return true;
}



void start_external_gui(void);

void stop_external_gui(void);