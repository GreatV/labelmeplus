#include "escapable_qlist_widget.h"

void EscapableQListWidget::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    this->clearSelection();
    return;
  }

  QListWidget::keyPressEvent(event);
}
