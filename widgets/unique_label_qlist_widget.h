#ifndef LABELMEPLUS_UNIQUE_LABEL_QLIST_WIDGET_H
#define LABELMEPLUS_UNIQUE_LABEL_QLIST_WIDGET_H

#include <QMouseEvent>

#include "escapable_qlist_widget.h"

class UniqueLabelQListWidget : public EscapableQListWidget {
 protected:
  void mousePressEvent(QMouseEvent* event);

 public:
  QListWidgetItem* createItemFromLabel(const QString& label);
  void setItemLabel(QListWidgetItem* item, const QString& label,
                    const std::vector<int>& color);
  QListWidgetItem* findItemByLabel(const QString& label);
};

#endif  // LABELMEPLUS_UNIQUE_LABEL_QLIST_WIDGET_H
