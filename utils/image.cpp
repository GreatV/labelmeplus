#include "utils/utils.h"

#include <libexif/exif-data.h>
#include <glog/logging.h>


cv::Mat apply_exif_orientation(const std::string& image_filepath, const cv::Mat& raw_image)
{
  ExifData *exifData = exif_data_new_from_file(image_filepath.c_str());

  int orientation = 0;
  if (exifData)
  {
    ExifByteOrder byteOrder = exif_data_get_byte_order(exifData);
    ExifEntry *exifEntry = exif_data_get_entry(exifData,
                                               EXIF_TAG_ORIENTATION);

    if (exifEntry)
    {
      orientation = exif_get_short(exifEntry->data, byteOrder);
      LOG(INFO) << "Current image orientation: " << orientation;
    }
      exif_data_free(exifData);
  }

  if (orientation == 0)
  {
      // do noting
      return raw_image;
  }

  // top-left
  if (orientation == 1)
  {
      // do noting
      return raw_image;
  }

  // top-right
  // left-to-right mirror
  if (orientation == 2)
  {
      cv::Mat image;
      cv::flip(raw_image, image, 0);
      return image;
  }

  // bottom-right
  // rotate 180
  if (orientation == 3)
  {
      cv::Mat image;
      cv::rotate(raw_image, image, cv::ROTATE_180);
      return image;
  }

  // bottom-left
  // top-to-bottom mirror
  if (orientation == 4)
  {
      cv::Mat image;
      cv::flip(raw_image, image, 1);
      return image;
  }

  // left-top
  // top-to-left mirror
  if (orientation == 5)
  {
      cv::Mat image;
      cv::rotate(raw_image, image, cv::ROTATE_90_COUNTERCLOCKWISE);
      cv::flip(image, image, 0);
      return image;
  }

  // right-top
  // rotate 270
  if (orientation == 6)
  {
      cv::Mat image;
      cv::rotate(raw_image, image, cv::ROTATE_90_COUNTERCLOCKWISE);
      return image;
  }

  // right-bottom
  // top-to-right mirror
  if (orientation == 7)
  {
      cv::Mat image;
      cv::rotate(raw_image, image, cv::ROTATE_90_CLOCKWISE);
      cv::flip(image, image, 0);
      return image;
  }

  // left-bottom
  // rotate 90
  if (orientation == 8)
  {
      cv::Mat image;
      cv::rotate(raw_image, image, cv::ROTATE_90_CLOCKWISE);
      return image;
  }

  return raw_image;
}
