#include "zoom_widget.h"

#include <QFontMetrics>

ZoomWidget::ZoomWidget(int value, QWidget* parent) : QSpinBox(parent) {
  this->setButtonSymbols(QAbstractSpinBox::NoButtons);
  this->setRange(1, 1000);
  this->setSuffix(" %");
  this->setValue(value);
  this->setToolTip("Zoom level");
  this->setStatusTip(this->toolTip());
  this->setAlignment(Qt::AlignCenter);
}

QSize ZoomWidget::minimumSizeHint() {
  auto height = QSpinBox::minimumSizeHint().height();
  auto fm = QFontMetrics(this->font());
  // https://kdepepo.wordpress.com/2019/08/05/about-deprecation-of-qfontmetricswidth/
  auto width = fm.horizontalAdvance(QString::number(this->maximum()));
  return {width, height};
}
