#include "brightness_contrast_dialog.h"

#include <QFormLayout>

BrightnessContrastDialog::BrightnessContrastDialog(const QImage& image,
                                                   QWidget* parent)
    : QDialog(parent) {
  this->setModal(true);
  this->setWindowTitle(tr("Brightness/Contrast"));

  slider_brightness_ = _create_slider();
  slider_contrast_ = _create_slider();

  auto* formLayout = new QFormLayout();
  formLayout->addRow(tr("Brightness"), slider_brightness_);
  formLayout->addRow(tr("Contrast"), slider_contrast_);
  this->setLayout(formLayout);

  img_ = image;
}

void BrightnessContrastDialog::onNewValue(int value) {
  auto brightness = slider_brightness_->value() / 50.0;
  auto contrast = slider_contrast_->value() / 50.0;

  auto img = img_;
  // Image Enhance
}

QSlider* BrightnessContrastDialog::_create_slider() {
  auto* slider = new QSlider(Qt::Horizontal);
  slider->setRange(0, 150);
  slider->setValue(50);
  QObject::connect(slider, SIGNAL(valueChanged(int)), this,
                   SLOT(onNewValue(int)));
  return slider;
}
