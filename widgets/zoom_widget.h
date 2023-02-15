#ifndef LABELMEPLUS_ZOOM_WIDGET_H
#define LABELMEPLUS_ZOOM_WIDGET_H

#include <QSpinBox>

class ZoomWidget : public QSpinBox {
  Q_OBJECT
 public:
  explicit ZoomWidget(int value = 100, QWidget* parent = nullptr);

 private:
  QSize minimumSizeHint();
};

#endif  // LABELMEPLUS_ZOOM_WIDGET_H
