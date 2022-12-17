#include "app.h"

#include <glog/logging.h>

#include "imgviz.hpp"

const auto& LABEL_COLORMAP = imgviz::getLabelColor;

typedef enum zoom_method {
  FIT_WINDOW,
  FIT_WIDTH,
  MANUAL_ZOOM,
} zoom_method;

app::app(const std::string& config, const std::string& filename,
         const std::string& output, const std::string& output_file,
         const std::string& output_dir, QWidget* parent)
    : QMainWindow(parent) {
  if (!output.empty()) {
    LOG(WARNING) << "argument output is deprecated, use output_file instead";

    if (output_file.empty()) {
      output_file_ = output;
    }
  }

  // see labelme/config/default_config.yaml for valid configuration
  if (config.empty()) {
    config_ = get_config();
  } else {
    config_ = config;
  }

  // set default shape colors

}

app::~app() {}

std::string app::get_config() { return std::string(); }
