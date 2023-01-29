#include "app.h"

#include <glog/logging.h>

#include "imgviz.hpp"
#include "init.h"
#include "shape.h"

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
  this->setWindowTitle(QString::fromStdString(__appname__));

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
  Shape shape;
  auto color = config_["shape"]["line_color"];
  shape.line_color = getYamlColor(color);
  color = config_["shape"]["fill_color"];
  shape.fill_color = getYamlColor(color);
  color = config_["shape"]["select_line_color"];
  shape.select_line_color = getYamlColor(color);
  color = config_["shape"]["select_fill_color"];
  shape.select_fill_color = getYamlColor(color);
  color = config_["shape"]["vertex_fill_color"];
  shape.vertex_fill_color = getYamlColor(color);
  color = config_["shape"]["hvertex_fill_color"];
  shape.hvertex_fill_color = getYamlColor(color);

  // set point size from config file
  auto point_size = config_["shape"]["point_size"];
  if (point_size.Type() == YAML::NodeType::Scalar) {
    shape.point_size = point_size.as<int>();
  }

  dirty_ = false;
  _noSelectionSlot = false;
  //  _copied_shapes =

  // Main widgets and related state
  std::map<std::string, bool> fit_to_content;
  if (config_["fit_to_content"].IsMap()) {
    fit_to_content =
        config_["fit_to_content"].as<std::map<std::string, bool>>();
  }

  std::map<std::string, std::vector<std::string>> flags;
  if (config_["label_flags"].IsMap()) {
    flags = config_["label_flags"]
                .as<std::map<std::string, std::vector<std::string>>>();
  }

  QStringList labels;
  if (config_["labels"].IsSequence()) {
    for (auto& item : config_["labels"].as<std::vector<std::string>>()) {
      labels.emplace_back(QString::fromStdString(item));
    }
  }

  bool sort_labels = false;
  if (config_["sort_labels"].IsScalar()) {
    sort_labels = config_["sort_labels"].as<bool>();
  }

  bool show_text_field = false;
  if (config_["show_label_text_field"].IsScalar()) {
    show_text_field = config_["show_label_text_field"].as<bool>();
  }

  std::string completion{"startswith"};
  if (config_["label_completion"].IsScalar()) {
    completion = config_["label_completion"].as<std::string>();
  }

  labelDialog_ = new LabelDialog(fit_to_content, flags, labels, sort_labels,
                                 show_text_field, completion, this);

  labelList_ = new LabelListWidget();
  lastOpenDir_ = "";

  flag_dock_ = new QDockWidget(tr("Flags"), this);
  flag_dock_->setObjectName("Flags");

  flag_widget_ = new QListWidget();
  if (config["flags"]) {
    loadFlags();
  }

  flag_dock_->setWidget(flag_widget_);
  QObject::connect(this->flag_widget_, SIGNAL(itemChanged(QListWidgetItem*)),
                   this, SLOT(setDirty()));

  QObject::connect(this->labelList_, SIGNAL(itemSelectionChanged), this,
                   SLOT(labelSelectionChanged));
  QObject::connect(this->labelList_, SIGNAL(itemDoubleClicked), this,
                   SLOT(editLabel));
  QObject::connect(this->labelList_, SIGNAL(itemChanged), this,
                   SLOT(labelItemChanged));
  QObject::connect(this->labelList_, SIGNAL(itemDropped), this,
                   SLOT(labelOrderChanged));

  shape_dock_ = new QDockWidget(tr("Polygon Labels"), this);
  shape_dock_->setObjectName("Labels");
  shape_dock_->setWidget(labelList_);

  uniqLabelList_ = new UniqueLabelQListWidget();
  uniqLabelList_->setToolTip(
      tr("Select label to start annotating for it. Press 'Esc' to deselect."));

  if (config_["labels"].IsSequence()) {
    for (const auto& label : config_["labels"].as<std::vector<std::string>>()) {
      auto item =
          uniqLabelList_->createItemFromLabel(QString::fromStdString(label));
      uniqLabelList_->addItem(item);
      std::vector<int> rgb = _get_rgb_by_label(label);
      uniqLabelList_->setItemLabel(item, QString::fromStdString(label), rgb);
    }
  }

  label_dock_ = new QDockWidget(tr("Label List"), this);
  label_dock_->setObjectName("Label List");
  label_dock_->setWidget(uniqLabelList_);

  fileSearch_ = new QLineEdit();
  fileSearch_->setPlaceholderText(tr("Search Filename"));
  QObject::connect(this->fileSearch_, SIGNAL(textChanged(QString)), this,
                   SLOT(fileSearchChanged));

  fileListWidget_ = new QListWidget();
  QObject::connect(fileListWidget_, SIGNAL(itemSelectionChanged), this,
                   SLOT(fileSelectionChanged));

  fileListLayout_ = new QVBoxLayout();
  fileListLayout_->setContentsMargins(0, 0, 0, 0);
  fileListLayout_->setSpacing(0);
  fileListLayout_->addWidget(fileSearch_);
  fileListLayout_->addWidget(fileListWidget_);

  file_dock_ = new QDockWidget(tr("File List"), this);
  file_dock_->setObjectName("Files");

  auto* fileListWidget = new QWidget();
  fileListWidget->setLayout(fileListLayout_);
  file_dock_->setWidget(fileListWidget);

  zoomWidget_ = new ZoomWidget();
  this->acceptDrops();
}

QColor app::getYamlColor(const YAML::Node& node) {
  auto color = QColor();
  if (node.Type() == YAML::NodeType::Sequence) {
    auto r = node[0].as<int>();
    auto g = node[1].as<int>();
    auto b = node[2].as<int>();
    auto a = node[3].as<int>();
    color = QColor(r, g, b, a);
  }
  return color;
}

app::~app() {}
YAML::Node app::get_config() { return YAML::Node(); }
