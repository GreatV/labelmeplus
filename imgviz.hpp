// https://github.com/wkentaro/imgviz-cpp
#ifndef IMGVIZ_IMGVIZ_H_
#define IMGVIZ_IMGVIZ_H_

#include <cmath>
#include <opencv2/opencv.hpp>

namespace imgviz {

inline cv::Mat isfinite(cv::Mat mat) { return mat == mat; }

inline std::string typeToStr(int type) {
  std::string r;

  unsigned depth = type & CV_MAT_DEPTH_MASK;
  unsigned chans = 1 + (type >> CV_CN_SHIFT);

  switch (depth) {
    case CV_8U:
      r = "8U";
      break;
    case CV_8S:
      r = "8S";
      break;
    case CV_16U:
      r = "16U";
      break;
    case CV_16S:
      r = "16S";
      break;
    case CV_32S:
      r = "32S";
      break;
    case CV_32F:
      r = "32F";
      break;
    case CV_64F:
      r = "64F";
      break;
    default:
      r = "User";
      break;
  }

  r += "C";
  r += (chans + '0');

  return r;
}

inline void printMat(const std::string &name, const cv::Mat &mat) {
  double min;
  double max;
  cv::minMaxLoc(mat, &min, &max);
  fprintf(stderr,
          "[%s] height=%d, width=%d, channels=%d, type=%s, min=%lf, max=%lf\n",
          name.c_str(), mat.rows, mat.cols, mat.channels(),
          typeToStr(mat.type()).c_str(), min, max);
}

// ----------------------------------------------------------------------------

inline cv::Mat normalize(cv::Mat src, double min_val, double max_val) {
  assert(src.type() == CV_32FC1);
  return (src - min_val) / (max_val - min_val);
}

inline cv::Mat normalize(cv::Mat src) {
  assert(src.type() == CV_32FC1);

  double min_val;
  double max_val;
  cv::minMaxLoc(src, &min_val, &max_val);

  return normalize(src, min_val, max_val);
}

inline cv::Mat depthToBgr(cv::Mat depth, double min_val, double max_val) {
  assert(depth.type() == CV_32FC1);

  cv::Mat depth_bgr;
  depth_bgr = normalize(depth, min_val, max_val);
  depth_bgr = depth_bgr * 255;
  depth_bgr.convertTo(depth_bgr, CV_8UC1);

  cv::applyColorMap(depth_bgr, depth_bgr, cv::COLORMAP_JET);
  depth_bgr.setTo(0, ~isfinite(depth));

  return depth_bgr;
}

inline cv::Mat centerize(const cv::Mat &src, const cv::Size &size) {
  if (src.size() == size) {
    return src;
  }

  cv::Mat dst = cv::Mat::zeros(size, src.type());

  unsigned height = src.rows;
  unsigned width = src.cols;
  double scale = fmin(static_cast<double>(size.height) / height,
                      static_cast<double>(size.width) / width);

  cv::Mat src_resized;
  cv::resize(src, src_resized, cv::Size(0, 0), /*fx=*/scale, /*fy=*/scale);

  cv::Size size_resized = src_resized.size();
  cv::Point pt1 = cv::Point((size.width - size_resized.width) / 2,
                            (size.height - size_resized.height) / 2);
  cv::Point pt2 = pt1 + cv::Point(size_resized.width, size_resized.height);
  cv::Rect roi = cv::Rect(pt1, pt2);
  src_resized.copyTo(dst(roi));

  return dst;
}

inline cv::Vec2i getTileShape(unsigned size, double hw_ratio = 1) {
  unsigned rows = static_cast<unsigned>(round(sqrt(size / hw_ratio)));
  unsigned cols = 0;
  while (rows * cols < size) {
    cols++;
  }
  while ((rows - 1) * cols >= size) {
    rows--;
  }
  return cv::Vec2i(rows, cols);
}

inline cv::Mat tile(const std::vector<cv::Mat> images,
                    cv::Vec2i shape = cv::Vec2i(0, 0),
                    const unsigned border_width = 5,
                    const cv::Scalar border_color = cv::Scalar(255, 255, 255)) {
  auto height = images[0].rows;
  auto width = images[0].cols;
  for (size_t i = 1; i < images.size(); i++) {
    if (images[i].rows > height) {
      height = images[i].rows;
    }
    if (images[i].cols > width) {
      width = images[i].cols;
    }
  }

  if (shape[0] * shape[1] == 0) {
    shape = getTileShape(
        /*size=*/static_cast<unsigned int>(images.size()),
        /*hw_ratio=*/static_cast<double>(height) / static_cast<double>(width));
  }
  auto rows = shape[0];
  auto cols = shape[1];

  cv::Mat dst;
  for (size_t j = 0; j < rows; j++) {
    cv::Mat dst_row;
    for (size_t i = 0; i < cols; i++) {
      size_t index = j * cols + i;

      cv::Mat image;
      if (index < images.size()) {
        image = images[index];
        image = centerize(image, cv::Size(width, height));
      } else {
        image = cv::Mat::zeros(height, width, images[0].type());
      }

      if (image.type() == CV_8UC1) {
        cv::cvtColor(image, image, cv::COLOR_GRAY2BGR);
      }
      assert(image.type() == CV_8UC3);

      if (i == 0) {
        dst_row = image;
      } else {
        if (border_width != 0) {
          cv::Mat border = cv::Mat(dst_row.rows, border_width, CV_8UC3);
          border.setTo(border_color);
          cv::hconcat(dst_row, border, dst_row);
        }
        cv::hconcat(dst_row, image, dst_row);
      }
    }
    if (j == 0) {
      dst = dst_row;
    } else {
      if (border_width != 0) {
        cv::Mat border = cv::Mat(border_width, dst.cols, CV_8UC3);
        border.setTo(border_color);
        cv::vconcat(dst, border, dst);
      }
      cv::vconcat(dst, dst_row, dst);
    }
  }

  return dst;
}

inline cv::Mat textInRectangle(const cv::Mat src, const std::string text,
                               const std::string loc = "lt+",
                               const int font_face = cv::FONT_HERSHEY_SIMPLEX,
                               const double font_scale = 1,
                               const int thickness = 2) {
  assert(loc == "lt+");  // TODO(wkentaro): support other loc

  cv::Mat dst = src.clone();
  if (dst.type() == CV_8UC1) {
    cv::cvtColor(dst, dst, cv::COLOR_GRAY2BGR);
  }
  assert(dst.type() == CV_8UC3);

  int baseline;
  cv::Size text_size =
      cv::getTextSize(text, font_face, font_scale, thickness, &baseline);
  baseline += thickness;

  cv::Point text_org = cv::Point(0, text_size.height + thickness * 2);

  cv::Point aabb1 = text_org + cv::Point(0, baseline - thickness * 2);
  cv::Point aabb2 =
      text_org + cv::Point(text_size.width, -text_size.height - thickness * 2);

  cv::Mat extender = cv::Mat::zeros(aabb1.y - aabb2.y, src.cols, CV_8UC3);
  extender.setTo(255);
  cv::vconcat(extender, dst, dst);

  cv::rectangle(dst, aabb1, aabb2,
                /*color=*/cv::Scalar(255, 255, 255),
                /*thickness=*/cv::FILLED);
  cv::putText(dst, text, /*org=*/text_org,
              /*fontFace=*/font_face,
              /*fontScale=*/font_scale, /*color=*/cv::Scalar(0, 0, 0),
              /*thickness=*/thickness);

  return dst;
}

inline cv::Vec3b getLabelColor(const uint8_t label) {
  // VOC label colormap
  uint8_t id = label;
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  for (size_t i = 0; i < 8; ++i) {
    r |= ((id & (1 << 0)) != 0) << (7 - i);
    g |= ((id & (1 << 1)) != 0) << (7 - i);
    b |= ((id & (1 << 2)) != 0) << (7 - i);
    id = id >> 3;
  }
  return cv::Vec3b(b, g, r);
}

inline cv::Mat labelToBgr(const cv::Mat label, const cv::Mat bgr,
                          const double alpha = 0.5) {
  cv::Mat bgr_ = bgr.clone();
  if (bgr.type() == CV_8UC1) {
    cv::cvtColor(bgr, bgr_, cv::COLOR_GRAY2BGR);
  }

  assert(label.type() == CV_32SC1);
  assert(bgr_.type() == CV_8UC3);

  cv::Mat label_bgr = cv::Mat::zeros(label.size(), CV_8UC3);

  for (size_t j = 0; j < label.rows; j++) {
    for (size_t i = 0; i < label.cols; i++) {
      cv::Vec3b bgr_color = bgr_.at<cv::Vec3b>((int)j, (int)i);
      cv::Vec3b label_color =
          getLabelColor(label.at<int32_t>((int)j, (int)i) % 256);
      label_bgr.at<cv::Vec3b>((int)j, (int)i) =
          alpha * label_color + (1 - alpha) * bgr_color;
    }
  }
  return label_bgr;
}

}  // namespace imgviz

#endif /* IMGVIZ_IMGVIZ_H_ */
