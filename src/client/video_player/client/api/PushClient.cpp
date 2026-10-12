#include "video_player/client/api/PushClient.h"

#include "video_player/client/api/push_json.h"

#include <QAbstractSocket>
#include <QTimer>
#include <QtWebSockets/QWebSocket>

namespace video_player {
namespace client {
namespace {

const int kInitialBackoffMs = 1000;
const int kMaxBackoffMs = 30000;

}  // namespace

PushClient::PushClient(QObject* parent)
    : QObject(parent),
      socket_(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this)),
      reconnect_timer_(new QTimer(this)),
      want_connected_(false),
      backoff_ms_(kInitialBackoffMs) {
  reconnect_timer_->setSingleShot(true);
  connect(socket_, &QWebSocket::connected, this, &PushClient::onConnected);
  connect(socket_, &QWebSocket::disconnected, this, &PushClient::onDisconnected);
  connect(socket_, &QWebSocket::textMessageReceived, this,
          &PushClient::onTextMessageReceived);
  connect(reconnect_timer_, &QTimer::timeout, this,
          &PushClient::onReconnectTimer);

  qRegisterMetaType<video_player::client::Video>(
      "video_player::client::Video");
}

PushClient::~PushClient() {
  want_connected_ = false;
  reconnect_timer_->stop();
  socket_->close();
}

void PushClient::connectToServer(const QUrl& ws_url) {
  ws_url_ = ws_url;
  want_connected_ = true;
  ResetBackoff();
  reconnect_timer_->stop();
  if (socket_->state() == QAbstractSocket::ConnectedState) {
    socket_->close();
  }
  socket_->open(ws_url_);
}

void PushClient::disconnectFromServer() {
  want_connected_ = false;
  reconnect_timer_->stop();
  ResetBackoff();
  if (socket_->state() != QAbstractSocket::UnconnectedState) {
    socket_->close();
  }
}

bool PushClient::isConnected() const {
  return socket_->state() == QAbstractSocket::ConnectedState;
}

void PushClient::onConnected() {
  ResetBackoff();
  emit connectionChanged(true);
}

void PushClient::onDisconnected() {
  emit connectionChanged(false);
  if (want_connected_) {
    ScheduleReconnect();
  }
}

void PushClient::onTextMessageReceived(const QString& message) {
  PushMessage parsed;
  QString error;
  if (!ParsePushMessage(message.toUtf8(), &parsed, &error)) {
    return;
  }
  if (parsed.kind == PushMessage::kUpserted) {
    emit videoUpserted(parsed.video);
  } else if (parsed.kind == PushMessage::kRemoved) {
    emit videoRemoved(parsed.removed_id);
  }
}

void PushClient::onReconnectTimer() {
  if (!want_connected_) {
    return;
  }
  if (socket_->state() != QAbstractSocket::UnconnectedState) {
    return;
  }
  socket_->open(ws_url_);
}

void PushClient::ScheduleReconnect() {
  if (!want_connected_ || reconnect_timer_->isActive()) {
    return;
  }
  reconnect_timer_->start(backoff_ms_);
  backoff_ms_ = backoff_ms_ * 2;
  if (backoff_ms_ > kMaxBackoffMs) {
    backoff_ms_ = kMaxBackoffMs;
  }
}

void PushClient::ResetBackoff() { backoff_ms_ = kInitialBackoffMs; }

}  // namespace client
}  // namespace video_player
