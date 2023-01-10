#include "label_dialog.h"
#include "utils/utils.h"

#include <QKeyEvent>
#include <QHBoxLayout>


void LabelQLineEdit::keyPressEvent(QKeyEvent *event) {
  std::vector<int> keys{Qt::Key_Up, Qt::Key_Down};
  if (std::find(keys.begin(), keys.end(), event->key()) != keys.end())
  {
//    list_widget_->keyPressEvent(event);
  }
  else
  {
    QLineEdit::keyPressEvent(event);
  }
}
void LabelQLineEdit::setListWidget(QLineEdit *list_widget) {
  list_widget_ = list_widget;
}

LabelDialog::LabelDialog(const std::map<QString, bool>& fit_to_content,
                         const std::map<QString, bool>& flags,
                         const bool show_text_field,
                         const QString &text,
                         QDialog *parent)
: QDialog(parent)
{
  if (!fit_to_content.empty())
  {
    fit_to_content_ = fit_to_content;
  }

  edit_ = new LabelQLineEdit();
  edit_->setPlaceholderText(text);
  edit_->setValidator(labelValidator());

  QObject::connect(edit_, SIGNAL(editingFinished()), this, SLOT(postProcess));

  if (!flags.empty())
  {
    QObject::connect(edit_, SIGNAL(textChanged()), this, SLOT(updateFlags));
  }

  edit_group_id_ = new QLineEdit();
  edit_group_id_->setPlaceholderText(tr("Group ID"));
  edit_group_id_->setValidator(new QRegularExpressionValidator(QRegularExpression(R"(\d*)"), nullptr));

  layout_ = new QVBoxLayout();

  if (show_text_field)
  {
    auto* layout_edit = new QHBoxLayout();
    layout_edit->addWidget(edit_, 6);
    layout_edit->addWidget(edit_group_id_, 2);
    layout_->addLayout(layout_edit);
  }

  // buttons
  buttonBox_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  bb_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);

}
