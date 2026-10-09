#include "square_dance/client/api/Video.h"
#include "square_dance/client/download/DownloadService.h"

#include <QByteArray>
#include <QFile>
#include <QHostAddress>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <gtest/gtest.h>

namespace {

class FakeFileServer {
 public:
  bool Start(const QByteArray& body, int status = 200) {
    body_ = body;
    status_ = status;
    buffer_.clear();
    server_.reset(new QTcpServer());
    if (!server_->listen(QHostAddress::LocalHost, 0)) {
      return false;
    }
    QObject::connect(server_.data(), &QTcpServer::newConnection, [this]() {
      QTcpSocket* sock = server_->nextPendingConnection();
      if (sock == NULL) {
        return;
      }
      QObject::connect(sock, &QTcpSocket::readyRead, [this, sock]() {
        buffer_.append(sock->readAll());
        if (!buffer_.contains("\r\n\r\n")) {
          return;
        }
        const QByteArray reason =
            (status_ >= 200 && status_ < 300) ? QByteArray("OK")
                                              : QByteArray("Error");
        const QByteArray response =
            QByteArray("HTTP/1.1 ") + QByteArray::number(status_) + " " +
            reason +
            "\r\nContent-Type: application/octet-stream\r\nContent-Length: " +
            QByteArray::number(body_.size()) + "\r\nConnection: close\r\n\r\n" +
            body_;
        sock->write(response);
        sock->disconnectFromHost();
        buffer_.clear();
      });
    });
    return true;
  }

  quint16 port() const { return server_->serverPort(); }

 private:
  QScopedPointer<QTcpServer> server_;
  QByteArray body_;
  QByteArray buffer_;
  int status_;
};

bool WaitForSpy(QSignalSpy* spy, int timeout_ms = 3000) {
  if (spy->count() > 0) {
    return true;
  }
  return spy->wait(timeout_ms);
}

square_dance::client::Video MakeVideo(qint64 id, const QString& url) {
  square_dance::client::Video v;
  v.id = id;
  v.external_id = QString::fromUtf8("d") + QString::number(id);
  v.title = QString::fromUtf8("测试下载");
  v.play_url = url;
  v.download_url = url;
  v.source_type = QString::fromUtf8("authorized");
  v.updated_at = QString::fromUtf8("2026-10-09T08:00:00Z");
  return v;
}

}  // namespace

TEST(DownloadService, DownloadsFileAndEmitsProgress) {
  const QByteArray payload("tiny-mp4-bytes");
  FakeFileServer fake;
  ASSERT_TRUE(fake.Start(payload));

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  square_dance::client::DownloadService service(dir.path());

  QSignalSpy progress_spy(
      &service, &square_dance::client::DownloadService::progress);
  QSignalSpy finished_spy(
      &service, &square_dance::client::DownloadService::finished);
  QSignalSpy error_spy(
      &service, &square_dance::client::DownloadService::errorOccurred);

  const QString url = QString::fromUtf8("http://127.0.0.1:") +
                      QString::number(fake.port()) +
                      QString::fromUtf8("/clip.mp4");
  service.startDownload(MakeVideo(11, url));
  ASSERT_TRUE(WaitForSpy(&finished_spy)) << "finished not emitted";
  EXPECT_EQ(error_spy.count(), 0);
  EXPECT_GE(progress_spy.count(), 1);

  const QString local_path = finished_spy.at(0).at(1).toString();
  EXPECT_TRUE(QFile::exists(local_path));
  QFile file(local_path);
  ASSERT_TRUE(file.open(QIODevice::ReadOnly));
  EXPECT_EQ(file.readAll(), payload);
}

TEST(DownloadService, EmptyUrlEmitsError) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  square_dance::client::DownloadService service(dir.path());
  QSignalSpy error_spy(
      &service, &square_dance::client::DownloadService::errorOccurred);
  service.startDownload(MakeVideo(2, QString()));
  ASSERT_EQ(error_spy.count(), 1);
}

TEST(DownloadService, CancelRemovesPartial) {
  // Server accepts connection but never completes headers → cancel cleans partial.
  QTcpServer stall;
  ASSERT_TRUE(stall.listen(QHostAddress::LocalHost, 0));
  QObject::connect(&stall, &QTcpServer::newConnection, [&]() {
    stall.nextPendingConnection();  // leave hanging
  });

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  square_dance::client::DownloadService service(dir.path());
  QSignalSpy error_spy(
      &service, &square_dance::client::DownloadService::errorOccurred);

  const QString url = QString::fromUtf8("http://127.0.0.1:") +
                      QString::number(stall.serverPort()) +
                      QString::fromUtf8("/slow.mp4");
  service.startDownload(MakeVideo(3, url));
  service.cancel(3);
  ASSERT_TRUE(WaitForSpy(&error_spy));
  const QStringList leftovers =
      QDir(dir.path()).entryList(QDir::Files | QDir::NoDotAndDotDot);
  for (int i = 0; i < leftovers.size(); ++i) {
    EXPECT_FALSE(leftovers.at(i).endsWith(QString::fromUtf8(".partial")));
  }
}
