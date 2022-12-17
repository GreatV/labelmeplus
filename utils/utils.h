#ifndef UTILS_H
#define UTILS_H
#include <opencv2/opencv.hpp>
#include <string>

namespace labelme
{
/// @brief shape type
enum labeled_shape_type {

};

void lblsave(const std::string& filename, const cv::Mat& lbl);
}
#endif
