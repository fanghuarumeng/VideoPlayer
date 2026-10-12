#include "video_player/client/download/DownloadService.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegExp>
#include <QUrl>

namespace video_player {
namespace client {

DownloadService::DownloadService(const QString& download_dir, QObject* parent)
    : QObject(parent),
      download_dir_(download_dir),
      nam_(new QNetworkAccessManager(this)) {}

QString DownloadService::download_dir() const { return download_dir_; }

QString DownloadService::defaultFileName(const Video& video) const {
  return SanitizeFileName(video);
}

QString DownloadService::SanitizeFileName(const Video& video) const {
  QString base = video.title.trimmed();
  if (base.isEmpty()) {
    base = video.external_id;
  }
  if (base.isEmpty()) {
    base = QString::fromUtf8("video-") + QString::number(video.id);
  }
  base.replace(QRegExp(QString::fromUtf8("[\\\\/:*?\"<>|]")),
               QString::fromUtf8("_"));
  if (!base.endsWith(QString::fromUtf8(".mp4"), Qt::CaseInsensitive)) {
    base += QString::fromUtf8(".mp4");
  }
  return base;
}

DownloadService::ActiveDownload* DownloadService::FindByReply(
    QObject* sender) const {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender);
  if (reply == NULL) {
    return NULL;
  }
  return by_reply_.value(reply, NULL);
}

void DownloadService::startDownload(const Video& video) {
  startDownload(video, QString());
}

void DownloadService::startDownload(const Video& video,
                                    const QString& local_path_in) {
  if (by_id_.contains(video.id)) {
    emit errorOccurred(video.id, QString::fromUtf8("该视频正在下载"));
    return;
  }
  QString url = video.download_url.trimmed();
  if (url.isEmpty()) {
    url = video.play_url.trimmed();
  }
  if (url.isEmpty()) {
    emit errorOccurred(video.id, QString::fromUtf8("下载地址为空"));
    return;
  }
  const QUrl media_url(url);
  if (!media_url.isValid() || media_url.scheme().isEmpty()) {
    emit errorOccurred(video.id, QString::fromUtf8("下载地址无效"));
    return;
  }

  QString local_path = local_path_in.trimmed();
  if (local_path.isEmpty()) {
    QDir().mkpath(download_dir_);
    local_path = QDir(download_dir_).filePath(SanitizeFileName(video));
  } else {
    const QFileInfo info(local_path);
    QDir().mkpath(info.absolutePath());
  }
  const QString temp_path = local_path + QString::fromUtf8(".partial");

  QFile* file = new QFile(temp_path);
  if (!file->open(QIODevice::WriteOnly)) {
    delete file;
    emit errorOccurred(video.id, QString::fromUtf8("无法创建临时文件"));
    return;
  }

  QNetworkRequest request(media_url);
  QNetworkReply* reply = nam_->get(request);
  ActiveDownload* active = new ActiveDownload();
  active->video_id = video.id;
  active->local_path = local_path;
  active->temp_path = temp_path;
  active->reply = reply;
  active->file = file;
  by_id_.insert(video.id, active);
  by_reply_.insert(reply, active);

  connect(reply, &QNetworkReply::readyRead, this, &DownloadService::onReadyRead);
  connect(reply, &QNetworkReply::downloadProgress, this,
          &DownloadService::onDownloadProgress);
  connect(reply, &QNetworkReply::finished, this, &DownloadService::onFinished);
}

void DownloadService::cancel(qint64 video_id) {
  ActiveDownload* active = by_id_.value(video_id, NULL);
  if (active == NULL || active->reply == NULL) {
    return;
  }
  active->reply->abort();
}

void DownloadService::onReadyRead() {
  ActiveDownload* active = FindByReply(sender());
  if (active == NULL || active->file == NULL || active->reply == NULL) {
    return;
  }
  active->file->write(active->reply->readAll());
}

void DownloadService::onDownloadProgress(qint64 received, qint64 total) {
  ActiveDownload* active = FindByReply(sender());
  if (active == NULL) {
    return;
  }
  emit progress(active->video_id, received, total);
}

void DownloadService::onFinished() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  ActiveDownload* active = FindByReply(reply);
  if (active == NULL) {
    if (reply != NULL) {
      reply->deleteLater();
    }
    return;
  }

  if (active->file != NULL) {
    if (reply != NULL) {
      active->file->write(reply->readAll());
    }
    active->file->close();
    delete active->file;
    active->file = NULL;
  }

  const qint64 id = active->video_id;
  const QString local_path = active->local_path;
  const QString temp_path = active->temp_path;
  by_id_.remove(id);
  by_reply_.remove(reply);
  delete active;
  if (reply != NULL) {
    reply->deleteLater();
  }

  if (reply == NULL) {
    QFile::remove(temp_path);
    emit errorOccurred(id, QString::fromUtf8("下载失败"));
    return;
  }
  if (reply->error() == QNetworkReply::OperationCanceledError) {
    QFile::remove(temp_path);
    emit errorOccurred(id, QString::fromUtf8("下载已取消"));
    return;
  }
  if (reply->error() != QNetworkReply::NoError) {
    QFile::remove(temp_path);
    emit errorOccurred(id, reply->errorString());
    return;
  }
  const int status =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (status < 200 || status >= 300) {
    QFile::remove(temp_path);
    emit errorOccurred(id, QString::fromUtf8("HTTP ") + QString::number(status));
    return;
  }

  QFile::remove(local_path);
  if (!QFile::rename(temp_path, local_path)) {
    QFile::remove(temp_path);
    emit errorOccurred(id, QString::fromUtf8("无法保存下载文件"));
    return;
  }
  emit finished(id, local_path);
}

}  // namespace client
}  // namespace video_player
