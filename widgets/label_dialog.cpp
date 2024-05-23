#include "label_dialog.h"

#include <QCoreApplication>
#include <QKeyEvent>

LabelQLineEdit::LabelQLineEdit(QWidget* parent) : QLineEdit(parent) {}

void LabelQLineEdit::setListWidget(QListWidget* list_widget) {
  list_widget_ = list_widget;
}

void LabelQLineEdit::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down) {
    QCoreApplication::sendEvent(list_widget_, event);
  } else {
    QLineEdit::keyPressEvent(event);
  }
}

LabelDialog::LabelDialog(const QString& text, QWidget* parent,
                         const QStringList& labels, bool sort_labels,
                         bool show_text_field, const QString& completion,
                         const std::map<QString, bool>& fit_to_content,
                         const std::map<QString, bool>& flags)
    : QDialog(parent) {
  if (!fit_to_content.empty()) {
    fit_to_content_ = fit_to_content;
  }

  edit_ = new LabelQLineEdit();
  edit_->setPlaceholderText(text);
  edit_->setValidator(new QRegularExpressionValidator(
      QRegularExpression(QString(R"(^[^ \t].+)"))));

  QObject::connect(edit_, SIGNAL(editingFinished()), this, SLOT(postProcess()));
}

void LabelDialog::postProcess() {
  auto text = edit_->text();
  text = text.trimmed();
  edit_->setText(text);
}
