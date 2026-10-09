#include "square_dance/client/elder_ui/SearchBar.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

namespace square_dance {
namespace client {

SearchBar::SearchBar(QWidget* parent) : QWidget(parent) {
  QFont body;
  body.setPointSize(16);
  QFont title;
  title.setPointSize(18);

  edit_ = new QLineEdit(this);
  edit_->setFont(body);
  edit_->setPlaceholderText(QString::fromUtf8("输入舞曲名称"));
  edit_->setMinimumHeight(44);

  button_ = new QPushButton(QString::fromUtf8("搜索"), this);
  button_->setFont(title);
  button_->setMinimumHeight(44);
  button_->setMinimumWidth(120);

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(edit_, 1);
  layout->addWidget(button_);

  connect(button_, &QPushButton::clicked, this, &SearchBar::onSearchClicked);
  connect(edit_, &QLineEdit::returnPressed, this, &SearchBar::onSearchClicked);
}

QString SearchBar::query() const { return edit_->text().trimmed(); }

void SearchBar::setQuery(const QString& text) { edit_->setText(text); }

void SearchBar::onSearchClicked() { emit searchRequested(query()); }

}  // namespace client
}  // namespace square_dance
