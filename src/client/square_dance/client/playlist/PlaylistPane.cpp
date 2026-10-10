#include "square_dance/client/playlist/PlaylistPane.h"

#include "square_dance/client/elder_ui/AppIcons.h"
#include "square_dance/client/elder_ui/AppTheme.h"

#include <QHBoxLayout>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QSize>
#include <QUrl>
#include <QVBoxLayout>

namespace square_dance {
namespace client {

PlaylistPane::PlaylistPane(PlaylistStore* store, QWidget* parent)
    : QWidget(parent),
      store_(store),
      list_(new QListWidget(this)),
      clear_btn_(new QPushButton(QString::fromUtf8("清空播放列表"), this)),
      nam_(new QNetworkAccessManager(this)) {
  list_->setUniformItemSizes(true);
  list_->setIconSize(QSize(96, 54));
  list_->setSelectionMode(QAbstractItemView::SingleSelection);
  list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  list_->setSpacing(4);
  list_->setStyleSheet(AppTheme::ListPanelStyle());

  clear_btn_->setStyleSheet(AppTheme::IconButtonStyle());

  QVBoxLayout* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(8);
  root->addWidget(list_, 1);
  root->addWidget(clear_btn_, 0, Qt::AlignLeft);

  connect(clear_btn_, &QPushButton::clicked, this,
          &PlaylistPane::onClearClicked);
  connect(list_, &QListWidget::itemDoubleClicked, this,
          &PlaylistPane::onItemDoubleClicked);

  // Always load thumbs for the current store snapshot (do not wait for a
  // mergeFromVideo change — restart with unchanged URLs used to skip refresh).
  RebuildList();
}

void PlaylistPane::CancelAllThumbReplies() {
  const QList<QNetworkReply*> replies = thumb_replies_.keys();
  thumb_replies_.clear();
  for (int i = 0; i < replies.size(); ++i) {
    QNetworkReply* reply = replies.at(i);
    if (reply == NULL) {
      continue;
    }
    disconnect(reply, 0, this, 0);
    reply->abort();
    reply->deleteLater();
  }
}

QListWidgetItem* PlaylistPane::FindItemById(qint64 id) const {
  for (int i = 0; i < list_->count(); ++i) {
    QListWidgetItem* item = list_->item(i);
    if (item != NULL &&
        static_cast<qint64>(item->data(Qt::UserRole).toULongLong()) == id) {
      return item;
    }
  }
  return NULL;
}

PlaylistItem PlaylistPane::ItemFromRow(int row) const {
  PlaylistItem out;
  if (row < 0 || row >= list_->count()) {
    return out;
  }
  QListWidgetItem* item = list_->item(row);
  if (item == NULL) {
    return out;
  }
  out.id = static_cast<qint64>(item->data(Qt::UserRole).toULongLong());
  out.title = item->text();
  out.play_url = item->data(Qt::UserRole + 1).toString();
  out.thumbnail_url = item->data(Qt::UserRole + 2).toString();
  return out;
}

void PlaylistPane::RebuildList() {
  CancelAllThumbReplies();
  list_->clear();
  if (store_ == NULL) {
    return;
  }
  const QVector<PlaylistItem> items = store_->items();
  for (int i = 0; i < items.size(); ++i) {
    const PlaylistItem& pl = items[i];
    QListWidgetItem* item = new QListWidgetItem(AppIcons::Film(), pl.title);
    item->setData(Qt::UserRole, static_cast<qulonglong>(pl.id));
    item->setData(Qt::UserRole + 1, pl.play_url);
    item->setData(Qt::UserRole + 2, pl.thumbnail_url);
    item->setToolTip(pl.title);
    item->setSizeHint(QSize(0, 64));
    list_->addItem(item);
  }
  if (store_->currentIndex() >= 0 &&
      store_->currentIndex() < list_->count()) {
    list_->setCurrentRow(store_->currentIndex());
  }
  RequestThumbnails();
}

void PlaylistPane::RequestThumbnails() {
  if (store_ == NULL) {
    return;
  }
  const QVector<PlaylistItem> items = store_->items();
  for (int i = 0; i < items.size(); ++i) {
    RequestOneThumbnail(items[i]);
  }
}

void PlaylistPane::RequestOneThumbnail(const PlaylistItem& item) {
  const QString url = item.thumbnail_url.trimmed();
  if (url.isEmpty() || item.id == 0) {
    return;
  }
  QNetworkRequest req;
  req.setUrl(QUrl(url));
  QNetworkReply* reply = nam_->get(req);
  thumb_replies_.insert(reply, item.id);
  connect(reply, &QNetworkReply::finished, this,
          &PlaylistPane::onThumbFinished);
}

void PlaylistPane::onThumbFinished() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  if (reply == NULL) {
    return;
  }
  const qint64 id = thumb_replies_.take(reply);
  const QNetworkReply::NetworkError err = reply->error();
  const QByteArray bytes = reply->readAll();
  reply->deleteLater();
  if (id == 0 || err != QNetworkReply::NoError) {
    return;
  }
  QListWidgetItem* item = FindItemById(id);
  if (item == NULL) {
    return;
  }
  QPixmap pix;
  if (!pix.loadFromData(bytes)) {
    return;
  }
  item->setIcon(QIcon(pix.scaled(list_->iconSize(), Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation)));
}

void PlaylistPane::refresh() {
  RebuildList();
}

bool PlaylistPane::selectedItem(PlaylistItem* out) const {
  if (out == NULL) {
    return false;
  }
  *out = ItemFromRow(list_->currentRow());
  return out->id != 0;
}

void PlaylistPane::selectRow(int row) {
  if (row < 0 || row >= list_->count()) {
    return;
  }
  list_->setCurrentRow(row);
}

void PlaylistPane::onClearClicked() {
  const int ret = QMessageBox::question(
      this, QString::fromUtf8("清空播放列表"),
      QString::fromUtf8("确定清空播放列表吗？"),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (ret != QMessageBox::Yes || store_ == NULL) {
    return;
  }
  store_->clear();
  RebuildList();
  emit cleared();
}

void PlaylistPane::onItemDoubleClicked() {
  PlaylistItem item;
  if (!selectedItem(&item)) {
    return;
  }
  if (store_ != NULL) {
    store_->setCurrentIndex(list_->currentRow());
  }
  emit playRequested(item);
}

}  // namespace client
}  // namespace square_dance
