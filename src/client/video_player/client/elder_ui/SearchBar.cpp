#include "video_player/client/elder_ui/SearchBar.h"

#include "video_player/client/elder_ui/AppIcons.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

namespace video_player {
namespace client {

SearchBar::SearchBar(QWidget* parent) : QWidget(parent) {
  edit_ = new QLineEdit(this);
  edit_->setObjectName(QString::fromUtf8("searchField"));
  edit_->setPlaceholderText(QString::fromUtf8("搜索视频标题…"));
  edit_->setClearButtonEnabled(true);

  QAction* search_action = edit_->addAction(AppIcons::Search(), QLineEdit::LeadingPosition);

  button_ = new QPushButton(QString::fromUtf8("搜索"), this);
  button_->setObjectName(QString::fromUtf8("primaryButton"));
  button_->setIcon(AppIcons::Search());
  button_->setIconSize(QSize(18, 18));
  button_->setMinimumWidth(88);
  button_->setCursor(Qt::PointingHandCursor);
  Q_UNUSED(search_action);

  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(8);
  layout->addWidget(edit_, 1);
  layout->addWidget(button_);

  connect(button_, &QPushButton::clicked, this, &SearchBar::onSearchClicked);
  connect(edit_, &QLineEdit::returnPressed, this, &SearchBar::onSearchClicked);
}

QString SearchBar::query() const { return edit_->text().trimmed(); }

void SearchBar::setQuery(const QString& text) { edit_->setText(text); }

void SearchBar::onSearchClicked() { emit searchRequested(query()); }

}  // namespace client
}  // namespace video_player
