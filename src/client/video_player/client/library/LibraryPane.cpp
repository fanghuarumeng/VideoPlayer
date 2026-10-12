#include "video_player/client/library/LibraryPane.h"

#include "video_player/client/elder_ui/AppIcons.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace video_player {
namespace client {

LibraryPane::LibraryPane(QWidget* parent)
    : QWidget(parent),
      list_(new QListWidget(this)),
      refresh_btn_(new QPushButton(QString::fromUtf8("刷新"), this)),
      upload_btn_(new QPushButton(QString::fromUtf8("上传"), this)),
      add_btn_(new QPushButton(QString::fromUtf8("加入播放列表"), this)),
      empty_label_(new QLabel(
          QString::fromUtf8("暂无视频。可上传本地文件，或等待目录同步。"),
          this)),
      nam_(new QNetworkAccessManager(this)) {
  list_->setViewMode(QListView::IconMode);
  list_->setResizeMode(QListView::Adjust);
  list_->setMovement(QListView::Static);
  list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
  list_->setIconSize(QSize(140, 90));
  list_->setGridSize(QSize(168, 148));
  list_->setWordWrap(true);
  list_->setUniformItemSizes(true);
  list_->setSpacing(8);
  list_->setObjectName(QString::fromUtf8("recommendList"));
  refresh_btn_->setObjectName(QString::fromUtf8("iconButton"));
  upload_btn_->setObjectName(QString::fromUtf8("primaryButton"));
  add_btn_->setObjectName(QString::fromUtf8("iconButton"));
  empty_label_->setObjectName(QString::fromUtf8("captionLabel"));
  empty_label_->setWordWrap(true);

  QHBoxLayout* tools = new QHBoxLayout();
  tools->setSpacing(8);
  tools->addWidget(upload_btn_);
  tools->addWidget(add_btn_);
  tools->addStretch(1);
  tools->addWidget(refresh_btn_);

  QVBoxLayout* root = new QVBoxLayout(this);
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(8);
  root->addLayout(tools);
  root->addWidget(empty_label_);
  root->addWidget(list_, 1);

  connect(refresh_btn_, &QPushButton::clicked, this,
          &LibraryPane::refreshRequested);
  connect(upload_btn_, &QPushButton::clicked, this,
          &LibraryPane::uploadRequested);
  connect(add_btn_, &QPushButton::clicked, this, &LibraryPane::onAddClicked);
  connect(list_, &QListWidget::itemDoubleClicked, this,
          &LibraryPane::onItemDoubleClicked);
}

QIcon LibraryPane::PlaceholderIcon() const {
  return AppIcons::Film();
}

QListWidgetItem* LibraryPane::FindItemById(qint64 id) const {
  for (int i = 0; i < list_->count(); ++i) {
    QListWidgetItem* item = list_->item(i);
    if (item != NULL &&
        static_cast<qint64>(item->data(Qt::UserRole).toULongLong()) == id) {
      return item;
    }
  }
  return NULL;
}

QListWidgetItem* LibraryPane::MakeItem(const Video& video) const {
  QListWidgetItem* item =
      new QListWidgetItem(PlaceholderIcon(), video.title);
  item->setData(Qt::UserRole, static_cast<qulonglong>(video.id));
  item->setData(Qt::UserRole + 1, video.play_url);
  item->setData(Qt::UserRole + 2, video.thumbnail_url);
  item->setToolTip(video.title);
  item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
  item->setSizeHint(QSize(160, 136));
  return item;
}

void LibraryPane::CancelAllThumbReplies() {
  // Copy keys first: abort() can emit finished synchronously and would
  // invalidate a live QHash iterator (crash on concurrent uploads).
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

void LibraryPane::CancelThumbRepliesForId(qint64 id) {
  QList<QNetworkReply*> doomed;
  for (QHash<QNetworkReply*, qint64>::iterator it = thumb_replies_.begin();
       it != thumb_replies_.end(); ++it) {
    if (it.value() == id) {
      doomed.push_back(it.key());
    }
  }
  for (int i = 0; i < doomed.size(); ++i) {
    QNetworkReply* reply = doomed.at(i);
    thumb_replies_.remove(reply);
    if (reply == NULL) {
      continue;
    }
    disconnect(reply, 0, this, 0);
    reply->abort();
    reply->deleteLater();
  }
}

void LibraryPane::setVideos(const QVector<Video>& videos) {
  videos_ = videos;
  RebuildList();
}

void LibraryPane::upsertVideo(const Video& video) {
  if (video.id == 0) {
    return;
  }
  for (int i = 0; i < videos_.size(); ++i) {
    if (videos_[i].id != video.id) {
      continue;
    }
    const QString old_thumb = videos_[i].thumbnail_url;
    videos_[i] = video;
    QListWidgetItem* item = FindItemById(video.id);
    if (item == NULL) {
      RebuildList();
      return;
    }
    item->setText(video.title);
    item->setData(Qt::UserRole + 1, video.play_url);
    item->setData(Qt::UserRole + 2, video.thumbnail_url);
    item->setToolTip(video.title);
    if (!video.thumbnail_url.isEmpty() &&
        video.thumbnail_url != old_thumb) {
      RequestOneThumbnail(video);
    }
    return;
  }

  // New video: insert without tearing down other tiles' in-flight thumbs.
  videos_.prepend(video);
  empty_label_->setVisible(false);
  list_->setVisible(true);
  list_->insertItem(0, MakeItem(video));
  RequestOneThumbnail(video);
}

void LibraryPane::removeVideo(qint64 id) {
  CancelThumbRepliesForId(id);
  for (int i = 0; i < videos_.size(); ++i) {
    if (videos_[i].id == id) {
      videos_.removeAt(i);
      break;
    }
  }
  QListWidgetItem* item = FindItemById(id);
  if (item != NULL) {
    delete list_->takeItem(list_->row(item));
  }
  empty_label_->setVisible(videos_.isEmpty());
  list_->setVisible(!videos_.isEmpty());
}

void LibraryPane::RebuildList() {
  CancelAllThumbReplies();
  list_->clear();
  empty_label_->setVisible(videos_.isEmpty());
  list_->setVisible(!videos_.isEmpty());
  for (int i = 0; i < videos_.size(); ++i) {
    list_->addItem(MakeItem(videos_[i]));
  }
  RequestThumbnails();
}

void LibraryPane::RequestThumbnails() {
  for (int i = 0; i < videos_.size(); ++i) {
    RequestOneThumbnail(videos_[i]);
  }
}

void LibraryPane::RequestOneThumbnail(const Video& video) {
  const QString url = video.thumbnail_url.trimmed();
  if (url.isEmpty() || video.id == 0) {
    return;
  }
  CancelThumbRepliesForId(video.id);
  QNetworkRequest req;
  req.setUrl(QUrl(url));
  QNetworkReply* reply = nam_->get(req);
  thumb_replies_.insert(reply, video.id);
  connect(reply, &QNetworkReply::finished, this,
          &LibraryPane::onThumbFinished);
}

void LibraryPane::onThumbFinished() {
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

QVector<Video> LibraryPane::selectedVideos() const {
  QVector<Video> out;
  for (int i = 0; i < list_->count(); ++i) {
    QListWidgetItem* item = list_->item(i);
    if (item == NULL || !item->isSelected()) {
      continue;
    }
    Video v;
    v.id = static_cast<qint64>(item->data(Qt::UserRole).toULongLong());
    v.title = item->text();
    v.play_url = item->data(Qt::UserRole + 1).toString();
    v.thumbnail_url = item->data(Qt::UserRole + 2).toString();
    out.push_back(v);
  }
  return out;
}

void LibraryPane::clearSelection() {
  list_->clearSelection();
}

void LibraryPane::onAddClicked() {
  const QVector<Video> selected = selectedVideos();
  if (!selected.isEmpty()) {
    emit addToPlaylistRequested(selected);
  }
}

void LibraryPane::onItemDoubleClicked() {
  QListWidgetItem* item = list_->currentItem();
  if (item == NULL) {
    return;
  }
  Video v;
  v.id = static_cast<qint64>(item->data(Qt::UserRole).toULongLong());
  v.title = item->text();
  v.play_url = item->data(Qt::UserRole + 1).toString();
  v.thumbnail_url = item->data(Qt::UserRole + 2).toString();
  emit playRequested(v);
}

}  // namespace client
}  // namespace video_player
