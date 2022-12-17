#include <opencv2/opencv.hpp>
#include <vector>
#include <glog/logging.h>


cv::Mat polygons_to_mask()
{

}

cv::Mat shape_to_mask(const cv::Size& img_size, const std::vector<cv::Point>& points, int shape_type = 0, int line_width = 10, int point_size = 5)
{
    cv::Mat mask = cv::Mat::zeros(img_size, CV_8UC1);
    return cv::Mat();
}
