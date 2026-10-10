#include "square_dance/client/elder_ui/SearchBar.h"

#include "square_dance/client/elder_ui/ElderStyle.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

namespace square_dance {
namespace client {

SearchBar::SearchBar(QWidget* parent) : QWidget(parent) {
  edit_ = new QLineEdit(this);
  edit_->setFont(ElderStyle::BodyFont());
  edit_->setPlaceholderText(QString::fromUtf8("输入舞曲名称，例如：广场舞"));
  edit_->setStyleSheet(ElderStyle::SearchEditStyle());

  button_ = new QPushButton(QString::fromUtf8("搜索"), this);
  button_->setFont(ElderStyle::ButtonFont());
  button_->setStyleSheet(ElderStyle::PrimaryButtonStyle());
  button_->setMinimumWidth(128);
  button_->setCursor(Qt::PointingHandCursor);

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);
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
