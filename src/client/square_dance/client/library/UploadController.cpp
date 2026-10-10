#include "square_dance/client/library/UploadController.h"

#include "square_dance/client/api/catalog_json.h"

#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace square_dance {
namespace client {

UploadController::UploadController(const QString& http_base_url,
                                   int max_in_flight,
                                   QObject* parent)
    : QObject(parent),
      http_base_url_(NormalizeBaseUrl(http_base_url)),
      max_in_flight_(max_in_flight > 0 ? max_in_flight : 2),
      nam_(new QNetworkAccessManager(this)),
      in_flight_(0),
      retry_timer_(new QTimer(this)) {
  if (max_in_flight_ > 4) {
    max_in_flight_ = 4;
  }
  retry_timer_->setSingleShot(true);
  connect(retry_timer_, &QTimer::timeout, this, &UploadController::pump);
}

QString UploadController::NormalizeBaseUrl(const QString& url) const {
  QString out = url.trimmed();
  while (out.endsWith(QLatin1Char('/'))) {
    out.chop(1);
  }
  return out;
}

int UploadController::pendingCount() const {
  return pending_.size();
}

int UploadController::inFlightCount() const {
  return in_flight_;
}

bool UploadController::IsAllowedPath(const QString& path) const {
  const QString ext = QFileInfo(path).suffix().toLower();
  return ext == QString::fromUtf8("mp4") || ext == QString::fromUtf8("mkv") ||
         ext == QString::fromUtf8("webm") || ext == QString::fromUtf8("avi") ||
         ext == QString::fromUtf8("mov") || ext == QString::fromUtf8("mxf");
}

void UploadController::enqueueFiles(const QStringList& paths) {
  for (int i = 0; i < paths.size(); ++i) {
    const QString path = paths[i];
    if (!IsAllowedPath(path)) {
      emit uploadFailed(path, QString::fromUtf8("unsupported extension"));
      continue;
    }
    Job job;
    job.path = path;
    job.attempts = 0;
    pending_.push_back(job);
  }
  pump();
}

void UploadController::pump() {
  while (in_flight_ < max_in_flight_ && !pending_.isEmpty()) {
    const Job job = pending_.takeFirst();
    StartJob(job);
  }
  if (in_flight_ == 0 && pending_.isEmpty()) {
    emit queueIdle();
  }
}

void UploadController::StartJob(const Job& job) {
  QFile* file = new QFile(job.path);
  if (!file->open(QIODevice::ReadOnly)) {
    const QString err = QString::fromUtf8("cannot open file");
    delete file;
    emit uploadFailed(job.path, err);
    pump();
    return;
  }

  QHttpMultiPart* multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);

  QHttpPart file_part;
  file_part.setHeader(
      QNetworkRequest::ContentDispositionHeader,
      QVariant(QString::fromUtf8("form-data; name=\"file\"; filename=\"%1\"")
                   .arg(QFileInfo(job.path).fileName())));
  file_part.setBodyDevice(file);
  file->setParent(multi);
  multi->append(file_part);

  QHttpPart title_part;
  title_part.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QString::fromUtf8("form-data; name=\"title\"")));
  title_part.setBody(QFileInfo(job.path).completeBaseName().toUtf8());
  multi->append(title_part);

  const QUrl url(http_base_url_ + QString::fromUtf8("/api/v1/videos/upload"));
  QNetworkRequest request(url);
  QNetworkReply* reply = nam_->post(request, multi);
  multi->setParent(reply);
  reply->setProperty("upload_path", job.path);
  reply->setProperty("upload_attempts", job.attempts);
  ++in_flight_;
  emit statusMessage(QString::fromUtf8("上传中：") +
                     QFileInfo(job.path).fileName());
  connect(reply, &QNetworkReply::finished, this,
          &UploadController::onReplyFinished);
}

void UploadController::onReplyFinished() {
  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  if (reply == NULL) {
    return;
  }
  reply->deleteLater();
  if (in_flight_ > 0) {
    --in_flight_;
  }

  const QString path = reply->property("upload_path").toString();
  const int attempts = reply->property("upload_attempts").toInt();
  const int status =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  const QByteArray body = reply->readAll();

  if (status == 503) {
    Job job;
    job.path = path;
    job.attempts = attempts + 1;
    pending_.push_front(job);
    emit statusMessage(QString::fromUtf8("服务器忙，稍后重试上传…"));
    if (!retry_timer_->isActive()) {
      retry_timer_->start(500);
    }
    return;
  }

  if (reply->error() != QNetworkReply::NoError || status < 200 ||
      status >= 300) {
    emit uploadFailed(path, QString::fromUtf8("HTTP ") +
                                QString::number(status) +
                                QString::fromUtf8(" ") +
                                QString::fromUtf8(body));
    pump();
    return;
  }

  Video video;
  QString error;
  if (!ParseVideoJson(body, &video, &error)) {
    emit uploadFailed(path, error);
    pump();
    return;
  }
  emit uploadFinished(video);
  emit statusMessage(QString::fromUtf8("上传完成：") + video.title);
  pump();
}

}  // namespace client
}  // namespace square_dance
