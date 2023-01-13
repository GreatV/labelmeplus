#include <glog/logging.h>

#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>
#include <vector>

#include "utils/utils.h"

cv::Mat shape_to_mask(const cv::Size& img_size,
                      const std::vector<cv::Point>& points,
                      const int shape_type = shape_polygon,
                      const int line_width = 10, const int point_size = 5) {
  cv::Mat draw = cv::Mat::zeros(img_size, CV_8UC3);

  switch (shape_type) {
    case shape_circle: {
      assert((points.size() == 2 &&
              "Shape of shape_type=circle must have 2 points"));
      auto cx = points[0].x;
      auto cy = points[0].y;
      auto px = points[1].x;
      auto py = points[1].y;
      auto d = sqrt((cx - px) * (cx - px) + (cy - py) * (cy - py));
      cv::circle(draw, points[0], d, cv::Scalar(255, 255, 255), -1);
      break;
    }
    case shape_rectangle: {
      assert((points.size() == 2 &&
              "Shape of shape_type=rectangle must have 2 points"));
      auto rect = cv::Rect(points[0], points[1]);
      cv::rectangle(draw, rect, cv::Scalar(255, 255, 255), -1);
      break;
    }
    case shape_line: {
      assert((points.size() == 2 &&
              "Shape of shape_type=line must have 2 points"));
      cv::line(draw, points[0], points[1], cv::Scalar(255, 255, 255),
               line_width);
      break;
    }
    case shape_linestrip: {
      for (auto i = 0; i < points.size() - 1; ++i) {
        cv::line(draw, points[i], points[i + 1], cv::Scalar(255, 255, 255),
                 line_width);
      }
      break;
    }
    case shape_point: {
      assert((points.size() == 1 &&
              "Shape of shape_type=point must have 2 points"));
      auto r = point_size;
      cv::circle(draw, points[0], r, cv::Scalar(255, 255, 255), -1);
      break;
    }
    default: {
      assert((points.size() > 2 && "Polygon must have points more than 2"));
      cv::polylines(draw, points, true, cv::Scalar(255, 255, 255), -1);
      break;
    }
  }

  cv::cvtColor(draw, draw, cv::COLOR_BGR2GRAY);
  cv::Mat mask;
  cv::threshold(draw, mask, 20, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

  return mask;
}

cv::Mat polygons_to_mask(const cv::Size& img_shape,
                         const std::vector<cv::Point>& polygons,
                         const marked_shape_type shape_type = shape_polygon) {
  LOG(WARNING) << "The 'polygons_to_mask' function is deprecated, use "
                  "'shape_to_mask' instead.";
  return shape_to_mask(img_shape, polygons, shape_type);
}

void shapes_to_label(cv::Mat& cls, cv::Mat& ins, const cv::Size img_shape,
                     const nlohmann::json& shapes,
                     const std::map<std::string, int>& label_name_to_value) {
  cls = cv::Mat::zeros(img_shape, CV_8UC3);
  ins = cv::Mat::zeros(img_shape, CV_8UC3);
}

void labelme_shapes_to_label(cv::Mat& cls, cv::Mat& ins, cv::Size img_shape,
                             const nlohmann::json& shapes) {
  LOG(WARNING)
      << "labelme_shapes_to_label is deprecated, so please use shapes_to_label";

  std::map<std::string, int> label_name_to_value{{"_background_", 0}};
}

void masks_to_bboxes(const cv::Mat& masks) {}
