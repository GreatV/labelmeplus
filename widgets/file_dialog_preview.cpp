#include "file_dialog_preview.h"

#include <QLayout>
#include <QSize>
#include <QVBoxLayout>
#include <fstream>
#include <nlohmann/json.hpp>

ScrollAreaPreview::ScrollAreaPreview(QWidget *parent) : QScrollArea(parent) {
  this->setWidgetResizable(true);
  auto *content = new QWidget(this);
  this->setWidget(content);

  auto *lay = new QVBoxLayout(content);

  label = new QLabel(content);
  label->setWordWrap(true);

  lay->addWidget(label);
}

void ScrollAreaPreview::setText(const QString &text) { label->setText(text); }

void ScrollAreaPreview::setPixmap(const QPixmap &pixmap) {
  label->setPixmap(pixmap);
}

void ScrollAreaPreview::clear() { label->clear(); }

FileDialogPreview::FileDialogPreview(QWidget *parent, const QString &caption,
                                     const QString &directory,
                                     const QString &filter)
    : QFileDialog(parent, caption, directory, filter) {
  this->setOption(this->DontUseNativeDialog, true);

  labelPreview = new ScrollAreaPreview(this);
  labelPreview->setFixedSize(300, 300);
  labelPreview->setHidden(true);

  auto *box = new QVBoxLayout();
  box->addWidget(labelPreview);
  box->addStretch();

  this->setFixedSize(this->width() + 300, this->height());
  //    this->setLayout(box);
  QObject::connect(this, SIGNAL(currentChanged(QString)), this, SLOT(onChange));
}

void FileDialogPreview::onChange(const QString &path) {
  if (path.endsWith(".json", Qt::CaseInsensitive)) {
    std::fstream f(path.toStdString(), std::ios::in);
    auto data = nlohmann::json::parse(f);
    std::string json_dumps = data.dump(4);
    labelPreview->setText(QString::fromStdString(json_dumps));
    labelPreview->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    labelPreview->setHidden(false);
  } else {
    auto pixmap = QPixmap(path);
    if (pixmap.isNull()) {
      labelPreview->clear();
      labelPreview->setHidden(true);
    } else {
      labelPreview->setPixmap(
          pixmap.scaled(labelPreview->width() - 30, labelPreview->height() - 30,
                        Qt::KeepAspectRatio, Qt::SmoothTransformation));
      labelPreview->label->setAlignment(Qt::AlignCenter);
      labelPreview->setHidden(false);
    }
  }
}
