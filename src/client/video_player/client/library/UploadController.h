#pragma once

#include "video_player/client/api/Video.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace video_player {
namespace client {

class UploadController : public QObject {
  Q_OBJECT

 public:
  explicit UploadController(const QString& http_base_url,
                            int max_in_flight = 2,
                            QObject* parent = 0);

  void enqueueFiles(const QStringList& paths);
  int pendingCount() const;
  int inFlightCount() const;

 signals:
  void statusMessage(const QString& message);
  void uploadFinished(const Video& video);
  void uploadFailed(const QString& path, const QString& error);
  void queueIdle();

 private slots:
  void onReplyFinished();
  void pump();

 private:
  struct Job {
    QString path;
    int attempts;
  };

  UploadController(const UploadController&);
  UploadController& operator=(const UploadController&);

  QString NormalizeBaseUrl(const QString& url) const;
  bool IsAllowedPath(const QString& path) const;
  void StartJob(const Job& job);

  QString http_base_url_;
  int max_in_flight_;
  QNetworkAccessManager* nam_;
  QVector<Job> pending_;
  int in_flight_;
  QTimer* retry_timer_;
};

}  // namespace client
}  // namespace video_player
