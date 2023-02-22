#include "unique_label_qlist_widget.h"

#include <fmt/format.h>
#include <glog/logging.h>

#include <QLabel>

void UniqueLabelQListWidget::mousePressEvent(QMouseEvent* event) {
  if (this->indexAt(event->pos()).isValid()) {
    this->clearSelection();
    return;
  }
  EscapableQListWidget::mousePressEvent(event);
}
QListWidgetItem* UniqueLabelQListWidget::findItemByLabel(const QString& label) {
  QListWidgetItem* item = nullptr;
  for (auto row = 0; row < this->count(); ++row) {
    item = this->item(row);
    if (item->data(Qt::UserRole) == label) {
      return item;
    }
  }
  return item;
}

QListWidgetItem* UniqueLabelQListWidget::createItemFromLabel(
    const QString& label) {
  if (findItemByLabel(label) != nullptr) {
    LOG(ERROR) << fmt::format("Item for label {0} already exists.",
                              label.toStdString());
  }
  auto item = new QListWidgetItem();
  item->setData(Qt::UserRole, label);

  return item;
}
void UniqueLabelQListWidget::setItemLabel(QListWidgetItem* item,
                                          const QString& label,
                                          const std::vector<int>& color) {
  auto* q_label = new QLabel();
  if (color.empty() || color.size() < 3) {
    q_label->setText(QString("%1").arg(label));
  } else {
    q_label->setText(QString("%1 <font color=\"#%2%3%4\">●</font>")
                         .arg(label.toHtmlEscaped())
                         .arg(color[0], 2, 16, QLatin1Char('0'))
                         .arg(color[1], 2, 16, QLatin1Char('0'))
                         .arg(color[2], 2, 16, QLatin1Char('0')));
  }
  q_label->setAlignment(Qt::AlignBottom);
  item->setSizeHint(q_label->sizeHint());
  setItemWidget(item, q_label);
}
