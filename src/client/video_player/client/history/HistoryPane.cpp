#include "video_player/client/history/HistoryPane.h"

#include "video_player/client/elder_ui/AppIcons.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace video_player {
namespace client {

HistoryPane::HistoryPane(PlayHistoryStore* store, QWidget* parent)
    : QWidget(parent),
      store_(store),
      list_(new QListWidget(this)),
      clear_btn_(new QPushButton(QString::fromUtf8("清空历史"), this)) {
  list_->setObjectName(QString::fromUtf8("recommendList"));
  list_->setIconSize(QSize(32, 32));
  clear_btn_->setObjectName(QString::fromUtf8("moreButton"));
  clear_btn_->setText(QString::fromUtf8("清空历史"));

  QVBoxLayout* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(8);
  root->addWidget(list_, 1);
  root->addWidget(clear_btn_, 0, Qt::AlignLeft);

  connect(clear_btn_, &QPushButton::clicked, this,
          &HistoryPane::onClearClicked);
  connect(list_, &QListWidget::itemDoubleClicked, this,
          &HistoryPane::onItemActivated);
  connect(list_, &QListWidget::itemActivated, this,
          &HistoryPane::onItemActivated);

  refresh();
}

QString HistoryPane::FormatTime(qint64 ms) {
  if (ms < 0) {
    ms = 0;
  }
  const qint64 total_sec = ms / 1000;
  const qint64 m = total_sec / 60;
  const qint64 s = total_sec % 60;
  return QString::fromUtf8("%1:%2")
      .arg(m, 2, 10, QLatin1Char('0'))
      .arg(s, 2, 10, QLatin1Char('0'));
}

void HistoryPane::refresh() {
  Rebuild();
}

void HistoryPane::Rebuild() {
  list_->clear();
  if (store_ == NULL) {
    return;
  }
  const QVector<PlayHistoryItem> items = store_->items();
  for (int i = 0; i < items.size(); ++i) {
    const PlayHistoryItem& h = items[i];
    const qint64 resume_ms = PlayHistoryStore::ResumePositionMs(h);
    const QString text =
        h.title + QString::fromUtf8("  ") + FormatTime(resume_ms) +
        QString::fromUtf8(" / ") + FormatTime(h.duration_ms);
    QListWidgetItem* item = new QListWidgetItem(AppIcons::Film(), text);
    item->setData(Qt::UserRole, static_cast<qulonglong>(h.id));
    item->setData(Qt::UserRole + 1, h.play_url);
    item->setData(Qt::UserRole + 2, h.thumbnail_url);
    item->setData(Qt::UserRole + 3, h.title);
    item->setData(Qt::UserRole + 4, static_cast<qulonglong>(resume_ms));
    list_->addItem(item);
  }
}

void HistoryPane::onClearClicked() {
  const int ret = QMessageBox::question(
      this, QString::fromUtf8("清空历史"),
      QString::fromUtf8("确定清空播放历史吗？"),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (ret != QMessageBox::Yes || store_ == NULL) {
    return;
  }
  store_->clear();
  Rebuild();
  emit cleared();
}

void HistoryPane::onItemActivated() {
  QListWidgetItem* item = list_->currentItem();
  if (item == NULL) {
    return;
  }
  Video video;
  video.id = static_cast<qint64>(item->data(Qt::UserRole).toULongLong());
  video.play_url = item->data(Qt::UserRole + 1).toString();
  video.thumbnail_url = item->data(Qt::UserRole + 2).toString();
  video.title = item->data(Qt::UserRole + 3).toString();
  const qint64 pos_ms =
      static_cast<qint64>(item->data(Qt::UserRole + 4).toULongLong());
  emit playRequested(video, pos_ms);
}

}  // namespace client
}  // namespace video_player
