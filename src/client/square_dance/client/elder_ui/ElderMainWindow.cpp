#include "square_dance/client/elder_ui/ElderMainWindow.h"

#include "square_dance/client/api/CatalogClient.h"
#include "square_dance/client/api/CatalogModel.h"
#include "square_dance/client/api/PushClient.h"
#include "square_dance/client/api/Video.h"
#include "square_dance/client/download/DownloadService.h"
#include "square_dance/client/elder_ui/ConnectionStatusWidget.h"
#include "square_dance/client/elder_ui/PlayerPane.h"
#include "square_dance/client/elder_ui/SearchBar.h"
#include "square_dance/client/elder_ui/VideoResultList.h"
#include "square_dance/client/playback/PlaybackController.h"
#include "square_dance/client/playback/VideoPlayerWidget.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

namespace square_dance {
namespace client {

ElderMainWindow::ElderMainWindow(const ClientConfig& config, QWidget* parent)
    : QMainWindow(parent),
      config_(config),
      search_bar_(NULL),
      result_list_(NULL),
      player_pane_(NULL),
      connection_status_(NULL),
      status_label_(NULL),
      model_(new CatalogModel(this)),
      catalog_(new CatalogClient(config.http_base_url, this)),
      push_(new PushClient(this)),
      playback_(new PlaybackController(this)),
      downloads_(new DownloadService(config.download_dir, this)) {
  setWindowTitle(QString::fromUtf8("广场舞"));
  resize(1100, 720);

  QFont body;
  body.setPointSize(16);
  QFont title;
  title.setPointSize(18);

  QLabel* brand = new QLabel(QString::fromUtf8("广场舞"), this);
  brand->setFont(title);

  connection_status_ = new ConnectionStatusWidget(this);
  search_bar_ = new SearchBar(this);
  result_list_ = new VideoResultList(this);
  result_list_->setCatalogModel(model_);
  // Large touch-friendly rows for arm's-length reading.
  result_list_->setStyleSheet(
      QString::fromUtf8("QListView::item { min-height: 40px; padding: 8px; }"));

  player_pane_ = new PlayerPane(this);
  playback_->setVideoOutput(player_pane_->playerWidget()->videoWidget());

  status_label_ = new QLabel(QString::fromUtf8("请输入名称后点击搜索"), this);
  status_label_->setFont(body);
  status_label_->setWordWrap(true);

  QHBoxLayout* top = new QHBoxLayout();
  top->addWidget(brand);
  top->addStretch(1);
  top->addWidget(connection_status_);

  QSplitter* split = new QSplitter(Qt::Horizontal, this);
  split->addWidget(result_list_);
  split->addWidget(player_pane_);
  split->setStretchFactor(0, 2);
  split->setStretchFactor(1, 3);

  QWidget* central = new QWidget(this);
  QVBoxLayout* root = new QVBoxLayout(central);
  root->addLayout(top);
  root->addWidget(search_bar_);
  root->addWidget(split, 1);
  root->addWidget(status_label_);
  setCentralWidget(central);

  connect(search_bar_, &SearchBar::searchRequested, this,
          &ElderMainWindow::onSearchRequested);
  connect(catalog_, &CatalogClient::searchFinished, this,
          &ElderMainWindow::onSearchFinished);
  connect(catalog_, &CatalogClient::searchFailed, this,
          &ElderMainWindow::onSearchFailed);
  connect(push_, &PushClient::videoUpserted, model_,
          &CatalogModel::applyUpserted);
  connect(push_, &PushClient::videoRemoved, model_,
          &CatalogModel::applyRemoved);
  connect(push_, &PushClient::connectionChanged, this,
          &ElderMainWindow::onConnectionChanged);
  connect(player_pane_, &PlayerPane::playClicked, this,
          &ElderMainWindow::onPlayClicked);
  connect(player_pane_, &PlayerPane::pauseClicked, this,
          &ElderMainWindow::onPauseClicked);
  connect(player_pane_, &PlayerPane::stopClicked, this,
          &ElderMainWindow::onStopClicked);
  connect(player_pane_, &PlayerPane::downloadClicked, this,
          &ElderMainWindow::onDownloadClicked);
  connect(playback_, &PlaybackController::errorOccurred, this,
          &ElderMainWindow::onPlaybackError);
  connect(downloads_, &DownloadService::progress, this,
          &ElderMainWindow::onDownloadProgress);
  connect(downloads_, &DownloadService::finished, this,
          &ElderMainWindow::onDownloadFinished);
  connect(downloads_, &DownloadService::errorOccurred, this,
          &ElderMainWindow::onDownloadError);

  push_->connectToServer(QUrl(config_.ws_url));
}

bool ElderMainWindow::SelectedVideo(Video* out) const {
  if (out == NULL) {
    return false;
  }
  const QModelIndex idx = result_list_->currentIndex();
  if (!idx.isValid()) {
    return false;
  }
  *out = model_->videoAt(idx.row());
  return out->id != 0 || !out->play_url.isEmpty();
}

void ElderMainWindow::SetStatusMessage(const QString& message) {
  status_label_->setText(message);
}

void ElderMainWindow::onSearchRequested(const QString& query) {
  current_query_ = query;
  model_->setFilterQuery(query);
  SetStatusMessage(QString::fromUtf8("正在搜索…"));
  catalog_->search(query, 50);
}

void ElderMainWindow::onSearchFinished(
    const QVector<square_dance::client::Video>& videos) {
  model_->resetFromSearch(videos);
  model_->setFilterQuery(current_query_);
  SetStatusMessage(QString::fromUtf8("找到 %1 个结果")
                       .arg(videos.size()));
}

void ElderMainWindow::onSearchFailed(const QString& error) {
  SetStatusMessage(QString::fromUtf8("搜索失败：") + error);
}

void ElderMainWindow::onPlayClicked() {
  Video video;
  if (!SelectedVideo(&video)) {
    SetStatusMessage(QString::fromUtf8("请先选择一个视频"));
    return;
  }
  SetStatusMessage(QString::fromUtf8("正在播放：") + video.title);
  playback_->play(video);
}

void ElderMainWindow::onPauseClicked() { playback_->pause(); }

void ElderMainWindow::onStopClicked() { playback_->stop(); }

void ElderMainWindow::onDownloadClicked() {
  Video video;
  if (!SelectedVideo(&video)) {
    SetStatusMessage(QString::fromUtf8("请先选择一个视频"));
    return;
  }
  SetStatusMessage(QString::fromUtf8("开始下载：") + video.title);
  downloads_->startDownload(video);
}

void ElderMainWindow::onPlaybackError(const QString& message) {
  SetStatusMessage(QString::fromUtf8("播放错误：") + message);
}

void ElderMainWindow::onDownloadProgress(qint64 /*video_id*/, qint64 received,
                                         qint64 total) {
  if (total > 0) {
    const int pct = static_cast<int>((received * 100) / total);
    SetStatusMessage(QString::fromUtf8("下载进度：%1%").arg(pct));
  } else {
    SetStatusMessage(QString::fromUtf8("已下载 %1 字节").arg(received));
  }
}

void ElderMainWindow::onDownloadFinished(qint64 /*video_id*/,
                                         const QString& local_path) {
  SetStatusMessage(QString::fromUtf8("下载完成：") + local_path);
}

void ElderMainWindow::onDownloadError(qint64 /*video_id*/,
                                      const QString& message) {
  SetStatusMessage(QString::fromUtf8("下载失败：") + message);
}

void ElderMainWindow::onConnectionChanged(bool connected) {
  connection_status_->setConnected(connected);
  if (connected) {
    SetStatusMessage(QString::fromUtf8("已连接服务器推送"));
  } else {
    SetStatusMessage(QString::fromUtf8("推送未连接，正在重试…"));
  }
}

}  // namespace client
}  // namespace square_dance
