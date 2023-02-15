#include "label_list_widget.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>

HTMLDelegate::HTMLDelegate(QWidget *parent) : QStyledItemDelegate(parent) {
  doc_ = new QTextDocument(this);
}

void HTMLDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const {
  painter->save();

  auto *options = new QStyleOptionViewItem(option);

  this->initStyleOption(options, index);
  doc_->setHtml(options->text);
  options->text = "";

  auto style = options->widget == nullptr ? QApplication::style()
                                          : options->widget->style();
  style->drawControl(QStyle::CE_ItemViewItem, options, painter);

  auto ctx = QAbstractTextDocumentLayout::PaintContext();

  if (option.state & QStyle::State_Selected) {
    ctx.palette.setColor(
        QPalette::Text,
        option.palette.color(QPalette::Active, QPalette::HighlightedText));
  } else {
    ctx.palette.setColor(
        QPalette::Text, option.palette.color(QPalette::Active, QPalette::Text));
  }

  auto textRect = style->subElementRect(QStyle::SE_ItemViewItemText, options);

  if (index.column() != 0) {
    textRect.adjust(5, 0, 0, 0);
  }

  const int constant = 4;
  auto margin = (option.rect.height() - options->fontMetrics.height()) / 2;
  margin -= constant;
  textRect.setTop(textRect.top() + margin);

  painter->translate(textRect.topLeft());
  painter->setClipRect(textRect.translated(-textRect.topLeft()));
  doc_->documentLayout()->draw(painter, ctx);

  painter->restore();
}

QSize HTMLDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const {
  const int constant = 4;
  return {int(doc_->idealWidth()), int(doc_->size().height() - constant)};
}

LabelListWidgetItem::LabelListWidgetItem(const QString &text,
                                         lmp::Shape *shape) {
  this->setText(text);
  setShape(shape);

  this->setCheckable(true);
  this->setCheckState(Qt::Checked);
  this->setEditable(false);
  this->setTextAlignment(Qt::AlignBottom);
}

LabelListWidgetItem *LabelListWidgetItem::clone() {
  return new LabelListWidgetItem(this->text(), shape());
}

void LabelListWidgetItem::setShape(lmp::Shape *shape) {
  this->setData(QVariant::fromValue(shape), Qt::UserRole);
}

lmp::Shape *LabelListWidgetItem::shape() {
  return qvariant_cast<lmp::Shape *>(this->data(Qt::UserRole));
}

bool StandardItemModel::removeRows(int row, int count,
                                   const QModelIndex &parent) {
  auto ret = QStandardItemModel::removeRows(row, count, parent);
  emit itemDropped();
  return ret;
}

LabelListWidgetItem *StandardItemModel::itemFromIndex(
    const QModelIndex &index) const {
  return new LabelListWidgetItem();
}

LabelListWidget::LabelListWidget() {
  this->setWindowFlags(Qt::Window);
  auto *model = new StandardItemModel();
  model->setItemPrototype(new LabelListWidgetItem());
  this->setModel(model);
  //    this->setModel(new StandardItemModel());
  //    this->model()->setItemPrototype(new LabelListWidgetItem());
  this->setItemDelegate(new HTMLDelegate());
  this->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->setDefaultDropAction(Qt::MoveAction);

  QObject::connect(this, SIGNAL(doubleClicked(QModelIndex)), this,
                   SLOT(itemDoubleClickedEvent(QModelIndex)));

  QObject::connect(
      this->selectionModel(),
      SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
      SLOT(itemSelectionChangedEvent(QItemSelection, QItemSelection)));
}

void LabelListWidget::itemSelectionChangedEvent(
    const QItemSelection &selected, const QItemSelection &deselected) {
  auto selected_items = QList<QStandardItem *>();
  auto deselected_items = QList<QStandardItem *>();
  for (auto &i : selected.indexes()) {
    auto item = this->model()->itemFromIndex(i);
    selected_items.emplaceBack(item);
  }

  for (auto &i : deselected.indexes()) {
    auto item = this->model()->itemFromIndex(i);
    deselected_items.emplaceBack(item);
  }

  emit itemSelectionChanged(selected_items, deselected_items);
}

void LabelListWidget::itemDoubleClickedEvent(const QModelIndex &index) {
  auto item = this->model()->itemFromIndex(index);
  emit itemDoubleClicked(item);
}

QList<LabelListWidgetItem *> LabelListWidget::selectedItems() {
  auto selected_items = QList<LabelListWidgetItem *>();
  for (auto &i : this->selectedIndexes()) {
    auto item = (LabelListWidgetItem *)this->model()->itemFromIndex(i);
    selected_items.emplaceBack(item);
  }
  return selected_items;
}

void LabelListWidget::scrollToItem(QStandardItem *item) {
  this->scrollTo(this->model()->indexFromItem(item));
}

void LabelListWidget::addItem(QStandardItem *item) {
  this->model()->setItem(this->model()->rowCount(), 0, item);
  item->setSizeHint(
      this->itemDelegate()->sizeHint(QStyleOptionViewItem(), QModelIndex()));
}

void LabelListWidget::removeItem(QStandardItem *item) {
  auto index = this->model()->indexFromItem(item);
  this->model()->removeRows(index.row(), 1);
}

void LabelListWidget::selectItem(QStandardItem *item) {
  auto index = this->model()->indexFromItem(item);
  this->selectionModel()->select(index, QItemSelectionModel::Select);
}

LabelListWidgetItem *LabelListWidget::findItemByShape(lmp::Shape *shape) {
  for (auto row = 0; row < this->model()->rowCount(); ++row) {
    LabelListWidgetItem *item =
        (LabelListWidgetItem *)this->model()->item(row, 0);
    if (item->shape() == shape) {
      return item;
    }
  }
  return nullptr;
}

void LabelListWidget::clear() { this->model()->clear(); }

StandardItemModel *LabelListWidget::model() { return this->model_; }
