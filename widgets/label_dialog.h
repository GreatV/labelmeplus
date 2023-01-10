#ifndef LABELMEPLUS_LABEL_DIALOG_H
#define LABELMEPLUS_LABEL_DIALOG_H


#include <QLineEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QVBoxLayout>


class LabelQLineEdit: public QLineEdit
{
  QLineEdit* list_widget_;
 public:
  void setListWidget(QLineEdit* list_widget);

 protected:
  void keyPressEvent(QKeyEvent *event) override;
};

class LabelDialog: public QDialog
{
 public:
  explicit LabelDialog(const std::map<QString, bool>& fit_to_content,
                       const std::map<QString, bool>& flags,
                       bool show_text_field = true,
                       const QString& text = "Enter object label",
                       QDialog* parent = nullptr);
 private:
  std::map<QString, bool> fit_to_content_{
      {"row", false},
      {"column", true},
  };
  LabelQLineEdit* edit_;
  QLineEdit* edit_group_id_;

  QVBoxLayout* layout_;
  QDialogButtonBox* buttonBox_, * bb_;
};
#endif  // LABELMEPLUS_LABEL_DIALOG_H
