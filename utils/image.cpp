#include "utils/utils.h"

#include <exiv2/exiv2.hpp>


cv::Mat apply_exif_orientation(const std::string& image_filepath, const cv::Mat& raw_image)
{
//  Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(image_filepath);

  return raw_image;
}
