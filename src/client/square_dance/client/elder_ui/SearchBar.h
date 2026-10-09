#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;

namespace square_dance {
namespace client {

class SearchBar : public QWidget {
  Q_OBJECT

 public:
  explicit SearchBar(QWidget* parent = 0);

  QString query() const;
  void setQuery(const QString& text);

 signals:
  void searchRequested(const QString& query);

 private slots:
  void onSearchClicked();

 private:
  QLineEdit* edit_;
  QPushButton* button_;
};

}  // namespace client
}  // namespace square_dance
