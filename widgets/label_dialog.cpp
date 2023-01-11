#include "label_dialog.h"

#include <fmt/format.h>
#include <glog/logging.h>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QRegularExpression>

#include "utils/utils.h"

void LabelQLineEdit::keyPressEvent(QKeyEvent *event) {
  std::vector<int> keys{Qt::Key_Up, Qt::Key_Down};
  if (std::find(keys.begin(), keys.end(), event->key()) != keys.end())
  {
//    list_widget_->keyPressEvent(event);
// TODO: using eventFilter instead
  }
  else
  {
    QLineEdit::keyPressEvent(event);
  }
}
void LabelQLineEdit::setListWidget(QListWidget *list_widget) {
  list_widget_ = list_widget;
}

LabelDialog::LabelDialog(const std::map<QString, bool>& fit_to_content,
                         const std::map<QString, std::vector<QString>>& flags,
                         const QStringList& labels,
                         const bool sort_labels,
                         const bool show_text_field,
                         const QString &text,
                         const QString &completion,
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

  QObject::connect(edit_, SIGNAL(editingFinished), this, SLOT(postProcess));

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

  bb_->button(bb_->Ok)->setIcon(newIcon("done"));
  bb_->button(bb_->Cancel)->setIcon(newIcon("undo"));

  QObject::connect(bb_, SIGNAL(accepted()), this, SLOT(validate()));
  QObject::connect(bb_, SIGNAL(rejected()), this, SLOT(reject()));

  layout_->addWidget(bb_);

  // label_list
  labelList_ = new QListWidget();
  if (fit_to_content_["row"])
  {
    labelList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  }

  if (fit_to_content_["column"])
  {
    labelList_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  }

  sort_labels_ = sort_labels;
  if (!labels.isEmpty())
  {
    labelList_->addItems(labels);
  }

  if (sort_labels_)
  {
    labelList_->sortItems();
  }
  else
  {
    labelList_->setDragDropMode(QAbstractItemView::InternalMove);
  }

  QObject::connect(labelList_,
                   SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
                   this,
                   SLOT(labelSelected(QListWidgetItem*))
                   );
  QObject::connect(labelList_,
                   SIGNAL(itemDoubleClicked(QListWidgetItem*)),
                   this,
                   SLOT(labelDoubleClicked(QListWidgetItem*)));

  edit_->setListWidget(labelList_);

  layout_->addWidget(labelList_);

  // label flags
  flags_ = flags;

  flagsLayout_ = new QVBoxLayout();
  resetFlags();
  layout_->addItem(flagsLayout_);
  QObject::connect(edit_, SIGNAL(textChanged), this, SLOT(updateFlags));
  this->setLayout(layout_);

  // completion
  completer_ = new QCompleter();

  if (completion == "startswith")
  {
    completer_->setCompletionMode(QCompleter::InlineCompletion);
  }
  else if (completion == "contains")
  {
    completer_->setCompletionMode(QCompleter::PopupCompletion);
    completer_->setFilterMode(Qt::MatchContains);
  }
  else
  {
    LOG(ERROR) <<  fmt::format("Unsupported completion: {0}", completion.toStdString());
  }
  completer_->setModel(labelList_->model());
  edit_->setCompleter(completer_);
}

void LabelDialog::addLabelHistory(const QString &label) {
  auto result = labelList_->findItems(label, Qt::MatchExactly);
  if (!result.isEmpty())
  {
    return;
  }
  labelList_->addItem(label);
  if (sort_labels_)
  {
    labelList_->sortItems();
  }
}

void LabelDialog::labelSelected(QListWidgetItem *item) {
  edit_->setText(item->text());
}

void LabelDialog::validate() {
  auto text = edit_->text();
  text = text.trimmed();
  if (!text.isEmpty())
  {
    this->accept();
  }
}
void LabelDialog::labelDoubleClicked(QListWidgetItem *item) {
  validate();
}
void LabelDialog::postProcess() {
  auto text = edit_->text();
  text = text.trimmed();
  edit_->setText(text);
}

void LabelDialog::updateFlags(const QString &label_new) {
  // keep state of shared flags
  auto flags_old = getFlags();

  std::map<QString, bool> flags_new;
  for (auto [pattern, keys]: flags_)
  {
    QRegularExpression re(pattern);
    if (re.match(label_new).hasMatch())
    {
      for (auto& key: keys)
      {
        // flags_new[key] = flags_old.count(key) == 0 ? false : flags_old[key];
        flags_new[key] = flags_old.count(key) != 0 && flags_old[key];
      }
    }
  }
  setFlags(flags_new);
}

void LabelDialog::deleteFlags() {
  for(int i = flagsLayout_->count(); i > 0; --i)
  {
    auto item = flagsLayout_->itemAt(i - 1)->widget();
    flagsLayout_->removeWidget(item);
    item->setParent(nullptr);
  }
}

void LabelDialog::resetFlags(const QString& label) {
  std::map<QString, bool> flags;
  for (auto& [pattern, keys]: flags_)
  {
    QRegularExpression re(pattern);
    if (re.match(label).hasMatch())
    {
      for (auto& key: keys)
      {
        flags[key] = false;
      }
    }
  }
  setFlags(flags);
}

void LabelDialog::setFlags(const std::map<QString, bool>& flags) {
  deleteFlags();
  for (auto& [key, val]: flags)
  {
    auto* item = new QCheckBox(key, this);
    item->setChecked(val);
    flagsLayout_->addWidget(item);
    item->show();
  }
}

std::map<QString, bool> LabelDialog::getFlags() {

  std::map<QString, bool> flags;
  for (auto i = 0; i < flagsLayout_->count(); ++i)
  {
    auto* item =
        dynamic_cast<QCheckBox *>(flagsLayout_->itemAt(i)->widget());
    flags[item->text()] = item->isChecked();
  }
  return flags;
}
