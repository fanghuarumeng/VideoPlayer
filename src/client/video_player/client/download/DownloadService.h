#pragma once

#include "video_player/client/api/Video.h"

#include <QHash>
#include <QObject>
#include <QString>

class QFile;
class QNetworkAccessManager;
class QNetworkReply;

namespace video_player {
namespace client {

class DownloadService : public QObject {
  Q_OBJECT

 public:
  explicit DownloadService(const QString& download_dir, QObject* parent = 0);

  void startDownload(const Video& video);
  // If local_path is empty, saves under download_dir() with a sanitized name.
  void startDownload(const Video& video, const QString& local_path);
  void cancel(qint64 video_id);

  QString download_dir() const;
  QString defaultFileName(const Video& video) const;

 signals:
  void progress(qint64 video_id, qint64 received, qint64 total);
  void finished(qint64 video_id, const QString& localPath);
  void errorOccurred(qint64 video_id, const QString& message);

 private slots:
  void onReadyRead();
  void onDownloadProgress(qint64 received, qint64 total);
  void onFinished();

 private:
  struct ActiveDownload {
    qint64 video_id;
    QString local_path;
    QString temp_path;
    QNetworkReply* reply;
    QFile* file;
  };

  DownloadService(const DownloadService&);
  DownloadService& operator=(const DownloadService&);

  QString SanitizeFileName(const Video& video) const;
  ActiveDownload* FindByReply(QObject* sender) const;

  QString download_dir_;
  QNetworkAccessManager* nam_;
  QHash<qint64, ActiveDownload*> by_id_;
  QHash<QNetworkReply*, ActiveDownload*> by_reply_;
};

}  // namespace client
}  // namespace video_player
