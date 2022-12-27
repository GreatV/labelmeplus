#ifndef UTILS_H
#define UTILS_H
#include <QRectF>
#include <opencv2/opencv.hpp>
#include <string>

/// @brief shape type
enum marked_shape_type {
  shape_circle = 0,
  shape_rectangle,
  shape_line,
  shape_linestrip,
  shape_point,
  shape_polygon
};

void lblsave(const std::string& filename, const cv::Mat& lbl);

double distance(const QPointF& p);

double distance_to_line(const QPointF& point, const std::vector<QPointF>& line);

cv::Mat apply_exif_orientation(const std::string& image_filepath,
                               const cv::Mat& raw_image);
#endif
