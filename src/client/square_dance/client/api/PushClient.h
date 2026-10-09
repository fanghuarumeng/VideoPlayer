#pragma once

#include "square_dance/client/api/Video.h"

#include <QObject>
#include <QString>
#include <QUrl>

class QTimer;
class QWebSocket;

namespace square_dance {
namespace client {

class PushClient : public QObject {
  Q_OBJECT

 public:
  explicit PushClient(QObject* parent = 0);
  ~PushClient();

  void connectToServer(const QUrl& ws_url);
  void disconnectFromServer();

  bool isConnected() const;

 signals:
  void videoUpserted(const square_dance::client::Video& video);
  void videoRemoved(qint64 id);
  void connectionChanged(bool connected);

 private slots:
  void onConnected();
  void onDisconnected();
  void onTextMessageReceived(const QString& message);
  void onReconnectTimer();

 private:
  PushClient(const PushClient&);
  PushClient& operator=(const PushClient&);

  void ScheduleReconnect();
  void ResetBackoff();

  QWebSocket* socket_;
  QTimer* reconnect_timer_;
  QUrl ws_url_;
  bool want_connected_;
  int backoff_ms_;
};

}  // namespace client
}  // namespace square_dance
