#ifndef LABELMEPLUS_ESCAPABLE_QLIST_WIDGET_H
#define LABELMEPLUS_ESCAPABLE_QLIST_WIDGET_H

#include <QListWidget>
#include <QMouseEvent>

class EscapableQListWidget : public QListWidget {
  Q_OBJECT
 protected:
  void keyPressEvent(QKeyEvent *event);
};

#endif  // LABELMEPLUS_ESCAPABLE_QLIST_WIDGET_H
