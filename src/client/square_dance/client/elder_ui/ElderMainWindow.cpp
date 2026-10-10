#include "square_dance/client/elder_ui/ElderMainWindow.h"

#include "square_dance/client/api/CatalogClient.h"
#include "square_dance/client/api/CatalogModel.h"
#include "square_dance/client/api/PushClient.h"
#include "square_dance/client/api/Video.h"
#include "square_dance/client/download/DownloadService.h"
#include "square_dance/client/elder_ui/AppIcons.h"
#include "square_dance/client/elder_ui/AppTheme.h"
#include "square_dance/client/elder_ui/ConnectionStatusWidget.h"
#include "square_dance/client/elder_ui/PlayerPane.h"
#include "square_dance/client/elder_ui/SearchBar.h"
#include "square_dance/client/elder_ui/VideoResultList.h"
#include "square_dance/client/library/LibraryPane.h"
#include "square_dance/client/history/HistoryPane.h"
#include "square_dance/client/history/PlayHistoryStore.h"
#include "square_dance/client/library/UploadController.h"
#include "square_dance/client/playback/PlaybackController.h"
#include "square_dance/client/playback/VideoPlayerWidget.h"
#include "square_dance/client/playlist/PlaylistPane.h"
#include "square_dance/client/playlist/PlaylistStore.h"

#include <QColor>
#include <QDateTime>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPalette>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
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
      content_stack_(NULL),
      nav_library_(NULL),
      nav_history_(NULL),
      nav_search_(NULL),
      playlist_store_(NULL),
      playlist_pane_(NULL),
      library_pane_(NULL),
      upload_(NULL),
      history_store_(NULL),
      history_pane_(NULL),
      model_(new CatalogModel(this)),
      catalog_(new CatalogClient(config.http_base_url, this)),
      push_(new PushClient(this)),
      playback_(new PlaybackController(this)),
      downloads_(new DownloadService(config.download_dir, this)),
      playing_from_playlist_(false),
      last_history_save_ms_(0) {
  upload_ = new UploadController(config.http_base_url, 2, this);
  history_store_ = new PlayHistoryStore(ResolveHistoryPath(), 100);
  QString hist_err;
  history_store_->load(&hist_err);
  setWindowTitle(QString::fromUtf8("Video Player"));
  setWindowIcon(AppIcons::Logo());
  resize(1280, 800);

  setStyleSheet(AppTheme::GlobalStylesheet());
  QPalette pal = palette();
  pal.setColor(QPalette::Window, QColor(0x35, 0x3A, 0x40));
  pal.setColor(QPalette::Base, QColor(0x2E, 0x33, 0x38));
  pal.setColor(QPalette::Text, QColor(0xFF, 0xFF, 0xFF));
  pal.setColor(QPalette::WindowText, QColor(0xFF, 0xFF, 0xFF));
  setPalette(pal);
  setAutoFillBackground(true);

  playlist_store_ = new PlaylistStore(ResolvePlaylistPath());
  QString pl_err;
  playlist_store_->load(&pl_err);

  QLabel* logo = new QLabel(this);
  logo->setPixmap(AppIcons::Logo().pixmap(28, 28));
  QLabel* brand = new QLabel(QString::fromUtf8("Video Player"), this);
  brand->setObjectName(QString::fromUtf8("brandLabel"));
  connection_status_ = new ConnectionStatusWidget(this);

  nav_library_ = new QPushButton(QString::fromUtf8("视频库"), this);
  nav_history_ = new QPushButton(QString::fromUtf8("历史"), this);
  nav_search_ = new QPushButton(QString::fromUtf8("搜索"), this);
  nav_library_->setObjectName(QString::fromUtf8("navButton"));
  nav_history_->setObjectName(QString::fromUtf8("navButton"));
  nav_search_->setObjectName(QString::fromUtf8("navButton"));
  nav_library_->setCheckable(true);
  nav_history_->setCheckable(true);
  nav_search_->setCheckable(true);
  nav_library_->setCursor(Qt::PointingHandCursor);
  nav_history_->setCursor(Qt::PointingHandCursor);
  nav_search_->setCursor(Qt::PointingHandCursor);

  library_pane_ = new LibraryPane(this);
  playlist_pane_ = new PlaylistPane(playlist_store_, this);
  history_pane_ = new HistoryPane(history_store_, this);

  search_bar_ = new SearchBar(this);
  result_list_ = new VideoResultList(this);
  result_list_->setCatalogModel(model_);
  QPushButton* add_pl_btn =
      new QPushButton(QString::fromUtf8("加入播放列表"), this);
  add_pl_btn->setObjectName(QString::fromUtf8("iconButton"));
  QWidget* search_body = new QWidget(this);
  QVBoxLayout* search_layout = new QVBoxLayout(search_body);
  search_layout->setContentsMargins(8, 8, 8, 8);
  search_layout->setSpacing(8);
  search_layout->addWidget(search_bar_);
  search_layout->addWidget(result_list_, 1);
  search_layout->addWidget(add_pl_btn, 0, Qt::AlignLeft);

  player_pane_ = new PlayerPane(this);
  playback_->setPlayerWidget(player_pane_->playerWidget());

  content_stack_ = new QStackedWidget(this);
  content_stack_->addWidget(player_pane_);  // 0
  content_stack_->addWidget(
      WrapSecondaryPage(library_pane_, QString::fromUtf8("视频库")));  // 1
  content_stack_->addWidget(
      WrapSecondaryPage(history_pane_, QString::fromUtf8("历史")));  // 2
  content_stack_->addWidget(
      WrapSecondaryPage(search_body, QString::fromUtf8("搜索")));  // 3
  content_stack_->setCurrentIndex(0);

  QFrame* up_next = new QFrame(this);
  up_next->setObjectName(QString::fromUtf8("sidePanel"));
  up_next->setAttribute(Qt::WA_StyledBackground, true);
  up_next->setMinimumWidth(300);
  up_next->setMaximumWidth(380);
  QVBoxLayout* up_next_layout = new QVBoxLayout(up_next);
  up_next_layout->setContentsMargins(0, 0, 0, 0);
  up_next_layout->addWidget(playlist_pane_, 1);

  status_label_ =
      new QLabel(QString::fromUtf8("正在加载视频库…"), this);
  status_label_->setObjectName(QString::fromUtf8("captionLabel"));
  statusBar()->addWidget(status_label_, 1);

  QFrame* top_bar = new QFrame(this);
  top_bar->setObjectName(QString::fromUtf8("topBar"));
  top_bar->setAttribute(Qt::WA_StyledBackground, true);
  QHBoxLayout* brand_row = new QHBoxLayout(top_bar);
  brand_row->setContentsMargins(8, 4, 8, 4);
  brand_row->setSpacing(12);
  brand_row->addWidget(logo);
  brand_row->addWidget(brand);
  brand_row->addSpacing(16);
  brand_row->addWidget(nav_library_);
  brand_row->addWidget(nav_history_);
  brand_row->addWidget(nav_search_);
  brand_row->addStretch(1);
  brand_row->addWidget(connection_status_, 0, Qt::AlignVCenter);

  QSplitter* split = new QSplitter(Qt::Horizontal, this);
  split->addWidget(content_stack_);
  split->addWidget(up_next);
  split->setStretchFactor(0, 1);
  split->setStretchFactor(1, 0);
  split->setChildrenCollapsible(false);
  QList<int> sizes;
  sizes << 900 << 340;
  split->setSizes(sizes);

  QWidget* central = new QWidget(this);
  QVBoxLayout* root = new QVBoxLayout(central);
  root->setContentsMargins(20, 16, 20, 12);
  root->setSpacing(12);
  root->addWidget(top_bar);
  root->addWidget(split, 1);
  setCentralWidget(central);

  connect(nav_library_, &QPushButton::clicked, this,
          &ElderMainWindow::showLibraryPage);
  connect(nav_history_, &QPushButton::clicked, this,
          &ElderMainWindow::showHistoryPage);
  connect(nav_search_, &QPushButton::clicked, this,
          &ElderMainWindow::showSearchPage);

  connect(result_list_, &QListView::doubleClicked, this,
          &ElderMainWindow::onPlayClicked);
  connect(search_bar_, &SearchBar::searchRequested, this,
          &ElderMainWindow::onSearchRequested);
  connect(add_pl_btn, &QPushButton::clicked, this,
          &ElderMainWindow::onAddSearchToPlaylist);
  connect(catalog_, &CatalogClient::searchFinished, this,
          &ElderMainWindow::onSearchFinished);
  connect(catalog_, &CatalogClient::searchFailed, this,
          &ElderMainWindow::onSearchFailed);
  connect(catalog_, &CatalogClient::listFinished, this,
          &ElderMainWindow::onListFinished);
  connect(catalog_, &CatalogClient::listFailed, this,
          &ElderMainWindow::onListFailed);
  connect(push_, &PushClient::videoUpserted, this,
          &ElderMainWindow::onVideoUpserted);
  connect(push_, &PushClient::videoRemoved, this,
          &ElderMainWindow::onVideoRemoved);
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
  connect(playback_, &PlaybackController::statusMessage, this,
          &ElderMainWindow::SetStatusMessage);
  connect(playback_, &PlaybackController::playbackProgress, player_pane_,
          &PlayerPane::setPlaybackProgress);
  connect(playback_, &PlaybackController::playbackProgress, this,
          &ElderMainWindow::onPlaybackProgressForHistory);
  connect(playback_, &PlaybackController::bufferProgress, player_pane_,
          &PlayerPane::setBufferProgress);
  connect(playback_, &PlaybackController::pausedChanged, player_pane_,
          &PlayerPane::setPausedState);
  connect(playback_, &PlaybackController::playbackFinished, this,
          &ElderMainWindow::onPlaybackFinished);
  connect(player_pane_, &PlayerPane::seekRequested, playback_,
          &PlaybackController::seekTo);
  connect(player_pane_, &PlayerPane::scrubbingChanged, playback_,
          &PlaybackController::setScrubbing);
  connect(downloads_, &DownloadService::progress, this,
          &ElderMainWindow::onDownloadProgress);
  connect(downloads_, &DownloadService::finished, this,
          &ElderMainWindow::onDownloadFinished);
  connect(downloads_, &DownloadService::errorOccurred, this,
          &ElderMainWindow::onDownloadError);
  connect(playlist_pane_, &PlaylistPane::playRequested, this,
          &ElderMainWindow::onPlaylistPlayRequested);
  connect(library_pane_, &LibraryPane::refreshRequested, this,
          &ElderMainWindow::onLibraryRefresh);
  connect(library_pane_, &LibraryPane::uploadRequested, this,
          &ElderMainWindow::onLibraryUpload);
  connect(library_pane_, &LibraryPane::addToPlaylistRequested, this,
          &ElderMainWindow::onLibraryAddToPlaylist);
  connect(library_pane_, &LibraryPane::playRequested, this,
          &ElderMainWindow::onLibraryPlayRequested);
  connect(history_pane_, &HistoryPane::playRequested, this,
          &ElderMainWindow::onHistoryPlayRequested);
  connect(upload_, &UploadController::statusMessage, this,
          &ElderMainWindow::SetStatusMessage);
  connect(upload_, &UploadController::uploadFinished, this,
          &ElderMainWindow::onUploadFinished);
  connect(upload_, &UploadController::uploadFailed, this,
          [this](const QString& path, const QString& error) {
            SetStatusMessage(QString::fromUtf8("上传失败：") + path +
                             QString::fromUtf8(" — ") + error);
          });

  push_->connectToServer(QUrl(config_.ws_url));
  catalog_->listVideos(200, 0);
}

QString ElderMainWindow::ResolvePlaylistPath() const {
  if (!config_.playlist_path.isEmpty()) {
    return config_.playlist_path;
  }
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         QString::fromUtf8("/playlist.json");
}

QString ElderMainWindow::ResolveHistoryPath() const {
  return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         QString::fromUtf8("/play_history.json");
}

Video ElderMainWindow::VideoFromPlaylistItem(const PlaylistItem& item) const {
  Video video;
  video.id = item.id;
  video.title = item.title;
  video.play_url = item.play_url;
  video.download_url = item.download_url.isEmpty() ? item.play_url
                                                   : item.download_url;
  video.thumbnail_url = item.thumbnail_url;
  video.source_type = QString::fromUtf8("playlist");
  return video;
}

QWidget* ElderMainWindow::WrapSecondaryPage(QWidget* body,
                                            const QString& title) {
  QWidget* page = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(page);
  layout->setContentsMargins(8, 4, 16, 8);
  layout->setSpacing(8);

  QPushButton* back = new QPushButton(QString::fromUtf8("← 返回播放"), page);
  back->setObjectName(QString::fromUtf8("navButton"));
  back->setCursor(Qt::PointingHandCursor);
  QLabel* heading = new QLabel(title, page);
  heading->setObjectName(QString::fromUtf8("videoTitleLabel"));

  QHBoxLayout* head = new QHBoxLayout();
  head->setSpacing(12);
  head->addWidget(back);
  head->addWidget(heading);
  head->addStretch(1);
  layout->addLayout(head);
  layout->addWidget(body, 1);

  connect(back, &QPushButton::clicked, this, &ElderMainWindow::showPlayerPage);
  return page;
}

void ElderMainWindow::SetNavChecked(QPushButton* active) {
  nav_library_->setChecked(active == nav_library_);
  nav_history_->setChecked(active == nav_history_);
  nav_search_->setChecked(active == nav_search_);
}

void ElderMainWindow::showPlayerPage() {
  content_stack_->setCurrentIndex(0);
  SetNavChecked(NULL);
}

void ElderMainWindow::showLibraryPage() {
  content_stack_->setCurrentIndex(1);
  SetNavChecked(nav_library_);
}

void ElderMainWindow::showHistoryPage() {
  content_stack_->setCurrentIndex(2);
  SetNavChecked(nav_history_);
}

void ElderMainWindow::showSearchPage() {
  content_stack_->setCurrentIndex(3);
  SetNavChecked(nav_search_);
}

void ElderMainWindow::PlayVideo(const Video& video, qint64 seek_ms) {
  current_video_ = video;
  last_history_save_ms_ = 0;
  player_pane_->setTitle(video.title);
  showPlayerPage();
  SetStatusMessage(QString::fromUtf8("正在播放：") + video.title);
  playback_->play(video);
  RecordHistory(seek_ms > 0 ? (seek_ms / 1000.0) : 0.0, 0.0);
  if (seek_ms > 0) {
    const double seek_sec = static_cast<double>(seek_ms) / 1000.0;
    QTimer::singleShot(400, playback_, [this, seek_sec]() {
      playback_->seekTo(seek_sec);
    });
  }
  if (history_pane_ != NULL) {
    history_pane_->refresh();
  }
}

void ElderMainWindow::onHistoryPlayRequested(const Video& video,
                                            qint64 position_ms) {
  playing_from_playlist_ = false;
  PlayVideo(video, position_ms);
}

void ElderMainWindow::RecordHistory(double position_sec, double duration_sec) {
  if (history_store_ == NULL || current_video_.id == 0) {
    return;
  }
  const qint64 pos_ms = static_cast<qint64>(position_sec * 1000.0);
  const qint64 dur_ms = static_cast<qint64>(duration_sec * 1000.0);
  history_store_->recordProgress(current_video_, pos_ms, dur_ms);
  last_history_save_ms_ = QDateTime::currentMSecsSinceEpoch();
  if (history_pane_ != NULL) {
    history_pane_->refresh();
  }
}

void ElderMainWindow::onPlaybackProgressForHistory(double position_sec,
                                                   double duration_sec) {
  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (last_history_save_ms_ != 0 && now - last_history_save_ms_ < 5000) {
    return;
  }
  RecordHistory(position_sec, duration_sec);
}

bool ElderMainWindow::SelectedSearchVideo(Video* out) const {
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

void ElderMainWindow::onLibraryRefresh() {
  SetStatusMessage(QString::fromUtf8("正在刷新视频库…"));
  catalog_->listVideos(200, 0);
}

void ElderMainWindow::onLibraryUpload() {
  const QStringList paths = QFileDialog::getOpenFileNames(
      this, QString::fromUtf8("选择要上传的视频"), QString(),
      QString::fromUtf8(
          "Videos (*.mp4 *.mkv *.webm *.avi *.mov *.mxf);;All (*.*)"));
  if (paths.isEmpty()) {
    return;
  }
  upload_->enqueueFiles(paths);
}

void ElderMainWindow::onUploadFinished(const Video& video) {
  library_pane_->upsertVideo(video);
}

void ElderMainWindow::onListFinished(const QVector<Video>& videos) {
  library_pane_->setVideos(videos);
  bool playlist_changed = false;
  for (int i = 0; i < videos.size(); ++i) {
    if (playlist_store_ != NULL && playlist_store_->mergeFromVideo(videos[i])) {
      playlist_changed = true;
    }
  }
  if (playlist_changed && playlist_pane_ != NULL) {
    playlist_pane_->refresh();
  }
  SetStatusMessage(QString::fromUtf8("视频库共 %1 个").arg(videos.size()));
}

void ElderMainWindow::onListFailed(const QString& error) {
  SetStatusMessage(QString::fromUtf8("加载视频库失败：") + error);
}

void ElderMainWindow::onLibraryAddToPlaylist(const QVector<Video>& videos) {
  int added = 0;
  for (int i = 0; i < videos.size(); ++i) {
    if (playlist_store_->append(videos[i])) {
      ++added;
    } else {
      // Already in list: still refresh snapshot (e.g. thumb arrived later).
      playlist_store_->mergeFromVideo(videos[i]);
    }
  }
  playlist_pane_->refresh();
  library_pane_->clearSelection();
  SetStatusMessage(QString::fromUtf8("已加入播放列表 %1 个").arg(added));
}

void ElderMainWindow::onLibraryPlayRequested(const Video& video) {
  playing_from_playlist_ = false;
  PlayVideo(video);
}

void ElderMainWindow::onVideoUpserted(const Video& video) {
  model_->applyUpserted(video);
  library_pane_->upsertVideo(video);
  if (playlist_store_ != NULL && playlist_store_->mergeFromVideo(video) &&
      playlist_pane_ != NULL) {
    playlist_pane_->refresh();
  }
}

void ElderMainWindow::onVideoRemoved(qint64 id) {
  model_->applyRemoved(id);
  library_pane_->removeVideo(id);
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
  SetStatusMessage(QString::fromUtf8("找到 %1 个结果").arg(videos.size()));
  if (!videos.isEmpty()) {
    result_list_->setCurrentIndex(model_->index(0, 0));
  }
}

void ElderMainWindow::onSearchFailed(const QString& error) {
  SetStatusMessage(QString::fromUtf8("搜索失败：") + error);
}

void ElderMainWindow::onPlayClicked() {
  const int page = content_stack_->currentIndex();
  if (page == 1) {
    const QVector<Video> selected = library_pane_->selectedVideos();
    if (!selected.isEmpty()) {
      onLibraryPlayRequested(selected.first());
      return;
    }
  }
  if (page == 3) {
    Video video;
    if (SelectedSearchVideo(&video)) {
      playing_from_playlist_ = false;
      PlayVideo(video);
      return;
    }
  }
  PlaylistItem item;
  if (playlist_pane_->selectedItem(&item)) {
    onPlaylistPlayRequested(item);
    return;
  }
  if (current_video_.id != 0 || !current_video_.play_url.isEmpty()) {
    playing_from_playlist_ = false;
    PlayVideo(current_video_);
    return;
  }
  SetStatusMessage(QString::fromUtf8("请先选择一个视频"));
}

void ElderMainWindow::onPlaylistPlayRequested(const PlaylistItem& item) {
  playing_from_playlist_ = true;
  const QVector<PlaylistItem> items = playlist_store_->items();
  for (int i = 0; i < items.size(); ++i) {
    if (items[i].id == item.id) {
      playlist_store_->setCurrentIndex(i);
      playlist_pane_->selectRow(i);
      break;
    }
  }
  PlayVideo(VideoFromPlaylistItem(item));
}

void ElderMainWindow::onPlaybackFinished() {
  if (!playing_from_playlist_ || playlist_store_ == NULL) {
    return;
  }
  const int next = playlist_store_->currentIndex() + 1;
  if (next < 0 || next >= playlist_store_->items().size()) {
    SetStatusMessage(QString::fromUtf8("播放列表已播完"));
    return;
  }
  playlist_store_->setCurrentIndex(next);
  playlist_pane_->selectRow(next);
  PlayVideo(VideoFromPlaylistItem(playlist_store_->items()[next]));
}

void ElderMainWindow::onAddSearchToPlaylist() {
  Video video;
  if (!SelectedSearchVideo(&video)) {
    SetStatusMessage(QString::fromUtf8("请先选择一个视频"));
    return;
  }
  if (playlist_store_->append(video)) {
    playlist_pane_->refresh();
    SetStatusMessage(QString::fromUtf8("已加入播放列表：") + video.title);
  } else {
    SetStatusMessage(QString::fromUtf8("已在播放列表中：") + video.title);
  }
}

void ElderMainWindow::onPauseClicked() { playback_->pause(); }

void ElderMainWindow::onStopClicked() {
  playing_from_playlist_ = false;
  playback_->stop();
  player_pane_->resetPlaybackProgress();
}

void ElderMainWindow::onDownloadClicked() {
  Video video = current_video_;
  if (video.id == 0 && video.play_url.isEmpty()) {
    if (!SelectedSearchVideo(&video)) {
      PlaylistItem item;
      if (playlist_pane_->selectedItem(&item)) {
        video = VideoFromPlaylistItem(item);
      } else {
        const QVector<Video> selected = library_pane_->selectedVideos();
        if (!selected.isEmpty()) {
          video = selected.first();
        } else {
          SetStatusMessage(QString::fromUtf8("请先选择一个视频"));
          return;
        }
      }
    }
  }
  SetStatusMessage(QString::fromUtf8("开始下载：") + video.title);
  downloads_->startDownload(video);
}

void ElderMainWindow::onPlaybackError(const QString& message) {
  SetStatusMessage(QString::fromUtf8("播放错误：") + message);
}

void ElderMainWindow::onDownloadProgress(qint64, qint64 received, qint64 total) {
  if (total > 0) {
    SetStatusMessage(QString::fromUtf8("下载进度：%1%").arg(
        static_cast<int>((received * 100) / total)));
  } else {
    SetStatusMessage(QString::fromUtf8("已下载 %1 字节").arg(received));
  }
}

void ElderMainWindow::onDownloadFinished(qint64, const QString& local_path) {
  SetStatusMessage(QString::fromUtf8("下载完成：") + local_path);
}

void ElderMainWindow::onDownloadError(qint64, const QString& message) {
  SetStatusMessage(QString::fromUtf8("下载失败：") + message);
}

void ElderMainWindow::onConnectionChanged(bool connected) {
  connection_status_->setConnected(connected);
  if (connected) {
    SetStatusMessage(QString::fromUtf8("已连接实时推送"));
  } else {
    SetStatusMessage(QString::fromUtf8("推送未连接，正在重试…"));
  }
}

}  // namespace client
}  // namespace square_dance
