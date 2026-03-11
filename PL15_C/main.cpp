#include "kv_io.h"
#include "measure2.h"
#include "timing.h"
#include "globals.h"
#include "sculpture.h"

#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "fs_utils.h"


using Clock = std::chrono::steady_clock;
using namespace kvio;


// ---------- Main ----------
int main(int argc, char **argv)
{
    static auto loop_start = Clock::now();

    init_gui(); // qualify with namespace

    start_external_gui();

    // Sculpture SC("trees.mov", "bars");
    Sculpture SC("bars_etc.mp4", "bars");


    std::array<cv::Mat, 4> Main_Display;

    Main_Display[0].create(SCREEN_IMAGE_ROWS, SCREEN_IMAGE_COLS, CV_8UC3);
    Main_Display[1].create(SCREEN_IMAGE_ROWS, SCREEN_IMAGE_COLS, CV_8UC3);
    Main_Display[2].create(SCREEN_IMAGE_ROWS, SCREEN_IMAGE_COLS, CV_8UC3);
    Main_Display[3].create(SCREEN_IMAGE_ROWS / 4, SCREEN_IMAGE_COLS / 4, CV_8UC3);
    long main_loop = 0;
    while (true)
    {

            auto main_start = Clock::now();

        bool Valid_Advance = SC.Advance(Main_Display);
        if(!Valid_Advance)
            break;

        if (!Main_Display[0].empty())
            cv::imshow("bars", Main_Display[0]);
        // if (!Main_Display[1].empty())
        //     cv::imshow("bars2", Main_Display[1]);
        // if (!Main_Display[2].empty())
        //     cv::imshow("mix", Main_Display[2]);

        

        if (!Main_Display[3].empty())
            cv::imshow("mini", Main_Display[3]);


        auto main_delta = GetDeltaTime(main_start);

    if (main_loop % 30 == 0)
        cout << "main_delta  " << main_delta << endl;        

        regulate_main_loop_timing(loop_start, main_loop_target_frame_us);

        auto wait_key_start = Clock::now();
        int key = cv::waitKey(1);
        if (key == 27)
            break; // ESC to quit
        auto wait_key_delta = GetDeltaTime(wait_key_start);

        main_loop++;
    }

    stop_external_gui();

    return 0;
}
