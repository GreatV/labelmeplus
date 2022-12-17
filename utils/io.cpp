#include "utils/utils.h"
#include "imgviz.hpp"
#include <filesystem>

void lblsave(const std::string& filename, const cv::Mat& lbl)
{
    // std::string output_filename = filename;
    // auto ext = std::filesystem::path(filename).extension();
    // if (ext != ".png")
    // {
    //     output_filename = filename + ".png";
    // }

    // // Assume label ranses [-1, 254] for int32,
    // // and [0, 255] for uint8 as VOC.
    // cv::imwrite(filename, lbl);
}