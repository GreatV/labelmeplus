#ifndef LABELMEPLUS_LABEL_DIALOG_H
#define LABELMEPLUS_LABEL_DIALOG_H


#include <QLineEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QCompleter>


class LabelQLineEdit: public QLineEdit
{
  QListWidget* list_widget_;
 public:
  void setListWidget(QListWidget* list_widget);

 protected:
  void keyPressEvent(QKeyEvent *event) override;
};

class LabelDialog: public QDialog
{
 public:
  explicit LabelDialog(const std::map<QString, bool>& fit_to_content,
                       const std::map<QString, std::vector<QString>>& flags,
                       const QStringList& labels,
                       bool sort_labels = true,
                       bool show_text_field = true,
                       const QString& text = "Enter object label",
                       const QString& completion = "startswith",
                       QDialog* parent = nullptr);
  void addLabelHistory(const QString& label);

 private:
  std::map<QString, bool> fit_to_content_{
      {"row", false},
      {"column", true},
  };
  LabelQLineEdit* edit_;
  QLineEdit* edit_group_id_;

  QVBoxLayout* layout_;
  QDialogButtonBox* buttonBox_, * bb_;

  QListWidget* labelList_;

  bool sort_labels_;

  std::map<QString, std::vector<QString>> flags_;
  QVBoxLayout* flagsLayout_;

  QCompleter *completer_;

 private slots:
  void labelSelected(QListWidgetItem* item);
  void validate();
  void labelDoubleClicked(QListWidgetItem* item);
  void postProcess();
  void updateFlags(const QString& label_new);
  void deleteFlags();
  void resetFlags(const QString& label = "");
  void setFlags(const std::map<QString, bool>& flags);
  std::map<QString, bool> getFlags();
  int getGroupId();
  void popUp(QString& out_label,
             std::map<QString, bool>& out_flags,
             int& out_group_id,
             const QString& text = "",
             bool move = true,
             const std::map<QString, bool>& flags = {},
             int group_id = -1);
};
#endif  // LABELMEPLUS_LABEL_DIALOG_H
