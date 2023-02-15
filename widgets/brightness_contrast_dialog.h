#ifndef BRIGHTNESSCONTRASTDIAG_H
#define BRIGHTNESSCONTRASTDIAG_H

#include <QDialog>
#include <QImage>
#include <QSlider>

class BrightnessContrastDialog : public QDialog {
  Q_OBJECT
 public:
  BrightnessContrastDialog(const QImage& image, QWidget* parent = nullptr);
  void onNewValue(int value);

 public:
  QSlider* slider_brightness_;
  QSlider* slider_contrast_;

 private:
  QImage img_;
  QSlider* _create_slider();
};

#endif  // BRIGHTNESSCONTRASTDIAG_H
