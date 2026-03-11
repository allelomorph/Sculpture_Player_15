#include "kv_io.h"
#include "sculpture.h"
#include "measure2.h"
#include "globals.h"
#include "image_processor.h"
#include "fs_utils.h"

#include <chrono>
#include <thread>
#include <algorithm> // for std::clamp

using Clock = std::chrono::steady_clock;
using namespace kvio;

Sculpture::Sculpture() = default;

Sculpture::Sculpture(const std::string &filename, const std::string &tag)
{
    VP[0].Open_Image_File_Mat(filename, tag);
    VP[1].Open_Image_File_Mat(filename, tag);
    // image_mixed_ will be sized lazily in Cross_Fade based on A/B

    Read_2D_Number(Sample_Points_Map, "Day_For_Night_Sample_Map.csv");

    // Read_2D_Number(Sample_Points_Map, "test1.csv");

    // // Read_2D_Number(Sculpture_Map, "Day_For_Night_Sample_Map.csv");

    // for (int i = 0; i < Sample_Points_Map.size(); i++)
    // {
    //     for (int j = 0; j < Sample_Points_Map[i].size(); j++)
    //         std::cout << Sample_Points_Map[i][j] << " ";
    //     std::cout << std::endl;
    // }

    // Read_2D_Number(Sample_Points_Map, "test2.csv");

    // std::cout << std::endl;
    // std::cout << std::endl;

    // // Read_2D_Number(Sculpture_Map, "Day_For_Night_Sample_Map.csv");

    // for (int i = 0; i < Sample_Points_Map.size(); i++)
    // {
    //     for (int j = 0; j < Sample_Points_Map[i].size(); j++)
    //         std::cout << Sample_Points_Map[i][j] << " ";
    //     std::cout << std::endl;
    // }

    // exit(0);

    Mixer_Params = Mixer_Params_Defaults;
}

const cv::Mat &Sculpture::Cross_Fade(const cv::Mat &A, const cv::Mat &B, bool toA, float fade_length_sec)
{
    CV_Assert(A.size() == B.size() && A.type() == B.type());

    // ensure buffer shape/type
    image_mixed_.create(A.size(), A.type());

    // advance fade toward target
    constexpr float kAssumedFps = 30.0f; // or compute dt with steady_clock for frame-rate independence
    const float step = (fade_length_sec > 0.f) ? (1.0f / (fade_length_sec * kAssumedFps)) : 1.0f;

    fade_level_ += toA ? +step : -step;
    fade_level_ = std::clamp(fade_level_, 0.0f, 1.0f);

    cv::addWeighted(A, fade_level_, B, 1.0f - fade_level_, 0.0, image_mixed_);
    return image_mixed_;
}

bool Sculpture::Advance(std::array<cv::Mat, 4> &outputs)
{
    static bool direc = true;

    auto process_start = Clock::now();

    check_and_load_new_kv_from_gui_dual(GUI_Player_Names, VP[0].Player_Params, VP[1].Player_Params);

    if (loop_counter % 30 == 0)
    {
        send_duration_to_gui_mmss(VP[0].Duration_Frames, 30);
        send_frame_to_gui_mmss(VP[0].Current_Frame, 30);
    }

    if ((VP[0].Player_Params[GUI_Player_Names.size() - 1] == 1) ||
        (VP[1].Player_Params[GUI_Player_Names.size() - 1] == 1))
        return false;

    // VP[0].Process_New_Frame_Ext_Process();
    // VP[1].Process_New_Frame_Ext_Process();
    // thread the 2 players
    std::jthread t1([&]
                    { VP[0].Process_New_Frame_Ext_Process(); });
    std::jthread t2([&]
                    { VP[1].Process_New_Frame_Ext_Process(); });
    t1.join();
    t2.join(); // ⬅️ ensure both finished

    Cross_Faded_F = Cross_Fade(VP[0].ImageMain_FM, VP[1].ImageMain_FM, direc, 5);

    // for display
    Cross_Faded_F.convertTo(Cross_Faded_M, CV_8UC3);

    auto process2_start = Clock::now();
    Save_Staggered_Samples_Grid_To_Buffer_RGB(Cross_Faded_F, Cross_Faded_F_Sampled, 0, 4, 4, 4);

    // temp sampler
    // resize(Cross_Faded_F, Cross_Faded_F_Sampled, cv::Size(), 0.25, 0.25, cv::INTER_NEAREST);
    auto process2_delta = GetDeltaTime(process2_start);

    // sub sample map

    // image map

    // send to ftdi driver

    Mixer_Params[H_SHIFT] = 100;
    Cross_Faded_F_Sampled = process_image_with_shift(Cross_Faded_F_Sampled, Mixer_Params);

    // for display
    Cross_Faded_F_Sampled.convertTo(Cross_Faded_M_Sampled, CV_8UC3);

    outputs[0] = VP[0].VideoDisplay; // shallow copy
    outputs[1] = VP[1].VideoDisplay; // shallow copy

    outputs[2] = Cross_Faded_M;
    outputs[3] = Cross_Faded_M_Sampled;

    auto process_delta = GetDeltaTime(process_start);

    if (loop_counter % 30 == 0)
        cout << "process_delta  " << process_delta << "    "; // "     process2_delta " << process2_delta << "    " ;

    if ((loop_counter % 180) == 0)
        direc = !direc;

    loop_counter++;
    return true;
}

// RGB float in -> RGB float out (both CV_32FC3)
// simplified: no stagger

void Sculpture::Save_Samples_Grid_To_Buffer_RGB(
    const cv::Mat &ImageIn_F,   // CV_32FC3, source
    cv::Mat &ImageSubSampled_F, // CV_32FC3, SCULPTURE_IMAGE_ROWS x SCULPTURE_IMAGE_COLS
    bool auto_gap,
    int H_gap, // used if !auto_gap (pixels)
    int V_gap) // used if !auto_gap (pixels)
{
    CV_Assert(ImageIn_F.type() == CV_32FC3);

    const int src_rows = ImageIn_F.rows;
    const int src_cols = ImageIn_F.cols;

    // Compute sampling gaps in source space
    const float h_gap = auto_gap
                            ? float(src_cols) / float(SCULPTURE_IMAGE_COLS)
                            : float(H_gap);

    const float v_gap = auto_gap
                            ? float(src_rows) / float(SCULPTURE_IMAGE_ROWS)
                            : float(V_gap);

    // Allocate/reuse output
    ImageSubSampled_F.create(SCULPTURE_IMAGE_ROWS, SCULPTURE_IMAGE_COLS, CV_32FC3);

    // Centered sampling
    float row_f = v_gap * 0.5f;

    for (int row_out = 0; row_out < SCULPTURE_IMAGE_ROWS; ++row_out)
    {
        const int row_in = int(row_f + 0.5f); // guaranteed in-range by your contract

        const cv::Vec3f *src_row = ImageIn_F.ptr<cv::Vec3f>(row_in);
        cv::Vec3f *dst_row = ImageSubSampled_F.ptr<cv::Vec3f>(row_out);

        float col_f = h_gap * 0.5f;

        for (int col_out = 0; col_out < SCULPTURE_IMAGE_COLS; ++col_out)
        {
            const int col_in = int(col_f + 0.5f); // guaranteed in-range
            dst_row[col_out] = src_row[col_in];
            // cout <<  "col_in " <<  col_in  << endl;
            col_f += h_gap;
        }
        row_f += v_gap;

        cout << "col_in " << row_in << endl;
    }
}
void Sculpture::Save_Staggered_Samples_Grid_To_Buffer_RGB(
    const cv::Mat &ImageIn_F,
    cv::Mat &ImageSubSampled_F,
    bool auto_gap,
    int H_gap,
    int V_gap,
    int Stagger)
{
    CV_Assert(ImageIn_F.type() == CV_32FC3);

    const int src_rows = ImageIn_F.rows;
    const int src_cols = ImageIn_F.cols;

    const float h_gap = auto_gap ? float(src_cols) / float(SCULPTURE_IMAGE_COLS) : float(H_gap);
    const float v_gap = auto_gap ? float(src_rows) / float(SCULPTURE_IMAGE_ROWS) : float(V_gap);

    ImageSubSampled_F.create(SCULPTURE_IMAGE_ROWS, SCULPTURE_IMAGE_COLS, CV_32FC3);

    auto clampi = [](int v, int lo, int hi)
    { return (v < lo) ? lo : (v > hi) ? hi
                                      : v; };

    if (Stagger <= 2)
    {
        float row_f = v_gap * 0.5f;

        for (int row_out = 0; row_out < SCULPTURE_IMAGE_ROWS; ++row_out)
        {
            int row_in = int(row_f + 0.5f);
            row_in = clampi(row_in, 0, src_rows - 1);

            const cv::Vec3f *src_row = ImageIn_F.ptr<cv::Vec3f>(row_in);
            cv::Vec3f *dst_row = ImageSubSampled_F.ptr<cv::Vec3f>(row_out);

            float h_phase = (Stagger == 0) ? 0.5f : (Stagger == 1) ? 0.25f
                                                                   : 0.75f;
            if (row_out & 1)
                h_phase = 1.0f - h_phase;

            float col_f = h_phase * h_gap;

            for (int col_out = 0; col_out < SCULPTURE_IMAGE_COLS; ++col_out)
            {
                int col_in = int(col_f + 0.5f);
                col_in = clampi(col_in, 0, src_cols - 1);

                dst_row[col_out] = src_row[col_in];
                if ((row_in >= 276) || ( row_in <= 2) )
                // cout << " row_in " << row_in << "   col_in " << col_in << endl;
                col_f += h_gap;
            }
            row_f += v_gap;
        }
    }
    else
    {
        // Stagger == 3 or 4: vertical stagger by column
        // Pick a base phase (0.25 or 0.75) and flip for odd columns.
        float v_phase = (Stagger == 3) ? 0.25f : 0.75f;

        for (int row_out = 0; row_out < SCULPTURE_IMAGE_ROWS; ++row_out)
        {
            cv::Vec3f *dst_row = ImageSubSampled_F.ptr<cv::Vec3f>(row_out);

            // Center of this output row in source space
            float row_center_f = (row_out + 0.5f) * v_gap;

            // Horizontal: normal centered sampling
            float col_f = h_gap * 0.5f;

            for (int col_out = 0; col_out < SCULPTURE_IMAGE_COLS; ++col_out)
            {
                float v_phase_local = v_phase;
                if (col_out & 1)
                    v_phase_local = 1.0f - v_phase_local;

                // Convert phase into an offset around the center:
                // phase 0.25 => offset -0.25*v_gap
                // phase 0.75 => offset +0.25*v_gap
                float row_f = row_center_f + (v_phase_local - 0.5f) * v_gap;

                int row_in = int(row_f + 0.5f);
                row_in = clampi(row_in, 0, src_rows - 1);

                int col_in = int(col_f + 0.5f);
                col_in = clampi(col_in, 0, src_cols - 1);

                const cv::Vec3f *src_row = ImageIn_F.ptr<cv::Vec3f>(row_in);
                dst_row[col_out] = src_row[col_in];

                // if( ((row_in >= 276) || ( row_in <= 3) ) && ((col_in >= 986) || ( col_in <= 10) ) )
                //                 cout << " row_in " << row_in << "   col_in " << col_in << endl;

                col_f += h_gap;
            }
        }
    }
}