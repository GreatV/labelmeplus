#ifndef LABEL_LIST_WIDGET_H
#define LABEL_LIST_WIDGET_H

#include <QListView>
#include <QModelIndex>
#include <QPainter>
#include <QStandardItem>
#include <QStyledItemDelegate>
#include <QTextDocument>

#include "shape.h"

class HTMLDelegate : public QStyledItemDelegate {
  Q_OBJECT
 public:
  explicit HTMLDelegate(QWidget* parent = nullptr);

 private:
  QTextDocument* doc_;
  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
  [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option,
                               const QModelIndex& index) const override;
};

class LabelListWidgetItem : public QStandardItem {
  //    Q_OBJECT
 public:
  explicit LabelListWidgetItem(const QString& text = {},
                               lmp::Shape* shape = nullptr);
  LabelListWidgetItem* clone();
  void setShape(lmp::Shape* shape);
  lmp::Shape* shape();
};

class StandardItemModel : public QStandardItemModel {
  Q_OBJECT
 public:
  bool removeRows(int row, int count, const QModelIndex& parent) override;

 signals:
  void itemDropped();
};

class LabelListWidget : public QListView {
  Q_OBJECT
 public:
  LabelListWidget();
  QList<LabelListWidgetItem*> selectedItems();
  void scrollToItem(QStandardItem* item);
  void addItem(QStandardItem* item);
  void removeItem(QStandardItem* item);
  void selectItem(QStandardItem* item);
  LabelListWidgetItem* findItemByShape(lmp::Shape* shape);
  void clear();
  StandardItemModel* model();

 private:
  StandardItemModel* model_{};

 signals:
  void itemDoubleClicked(QStandardItem*);
  void itemSelectionChanged(const QList<QStandardItem*>&,
                            const QList<QStandardItem*>&);
 private slots:
  void itemDoubleClickedEvent(const QModelIndex& index);
  void itemSelectionChangedEvent(const QItemSelection& selected,
                                 const QItemSelection& deselected);
};

#endif
