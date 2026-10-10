#include "square_dance/client/playback/PlaybackController.h"

#include "square_dance/client/playback/FfmpegPlaybackEngine.h"
#include "square_dance/client/playback/VideoPlayerWidget.h"

#include <QAudioFormat>
#include <QAudioOutput>
#include <QFileInfo>
#include <QUrl>

namespace square_dance {
namespace client {

PlaybackController::PlaybackController(QObject* parent)
    : QObject(parent),
      player_widget_(NULL),
      engine_(new FfmpegPlaybackEngine(this)),
      audio_out_(NULL),
      audio_device_(NULL),
      paused_(false),
      playing_(false),
      has_presented_frame_(false),
      scrubbing_(false),
      duration_sec_(0.0),
      position_sec_(0.0) {
  connect(engine_, &FfmpegPlaybackEngine::videoFrameReady, this,
          &PlaybackController::onVideoFrame, Qt::QueuedConnection);
  connect(engine_, &FfmpegPlaybackEngine::audioSamplesReady, this,
          &PlaybackController::onAudioSamples, Qt::QueuedConnection);
  connect(engine_, &FfmpegPlaybackEngine::playbackFailed, this,
          &PlaybackController::onEngineFailed, Qt::QueuedConnection);
  connect(engine_, &FfmpegPlaybackEngine::playbackEnded, this,
          &PlaybackController::onEngineEnded, Qt::QueuedConnection);
  connect(engine_, &FfmpegPlaybackEngine::durationReady, this,
          &PlaybackController::onDurationReady, Qt::QueuedConnection);
  connect(engine_, &FfmpegPlaybackEngine::positionChanged, this,
          &PlaybackController::onPositionChanged, Qt::QueuedConnection);
  connect(engine_, &FfmpegPlaybackEngine::seekCompleted, this,
          &PlaybackController::onSeekCompleted, Qt::QueuedConnection);
}

PlaybackController::~PlaybackController() {
  player_widget_ = NULL;
  stop();
}

void PlaybackController::setPlayerWidget(VideoPlayerWidget* widget) {
  player_widget_ = widget;
}

QUrl PlaybackController::ResolvePlayUrl(const QString& url_text) const {
  const QUrl url(url_text);
  if (url.isLocalFile()) {
    const QString local = url.toLocalFile();
    if (QFileInfo::exists(local)) {
      return QUrl::fromLocalFile(local);
    }
  }
  if (url.scheme().isEmpty() && QFileInfo::exists(url_text)) {
    return QUrl::fromLocalFile(url_text);
  }
  return url;
}

void PlaybackController::UpdateHint(const QString& text) {
  if (player_widget_ != NULL) {
    player_widget_->setHintText(text);
  }
}

void PlaybackController::TearDownAudio() {
  if (audio_out_ != NULL) {
    audio_out_->stop();
    audio_out_->deleteLater();
    audio_out_ = NULL;
    audio_device_ = NULL;
  }
}

bool PlaybackController::EnsureAudioOutput(QString* error) {
  if (audio_out_ != NULL && audio_device_ != NULL) {
    return true;
  }
  TearDownAudio();

  QAudioFormat format;
  format.setSampleRate(FfmpegPlaybackEngine::kAudioSampleRate);
  format.setChannelCount(FfmpegPlaybackEngine::kAudioChannels);
  format.setSampleSize(16);
  format.setCodec(QString::fromUtf8("audio/pcm"));
  format.setByteOrder(QAudioFormat::LittleEndian);
  format.setSampleType(QAudioFormat::SignedInt);

  audio_out_ = new QAudioOutput(format, this);
  audio_out_->setBufferSize(FfmpegPlaybackEngine::kAudioSampleRate *
                            FfmpegPlaybackEngine::kAudioChannels *
                            FfmpegPlaybackEngine::kAudioBytesPerSample / 2);
  audio_device_ = audio_out_->start();
  if (audio_device_ == NULL) {
    TearDownAudio();
    if (error != NULL) {
      *error = QString::fromUtf8("无法打开音频输出");
    }
    return false;
  }
  return true;
}

void PlaybackController::EmitProgress() {
  emit playbackProgress(position_sec_, duration_sec_);
}

void PlaybackController::setScrubbing(bool scrubbing) { scrubbing_ = scrubbing; }

void PlaybackController::seekTo(double position_sec) {
  if (!playing_) {
    return;
  }
  engine_->seekTo(position_sec);
}

void PlaybackController::play(const Video& video) {
  const QString url_text = video.play_url.trimmed();
  if (url_text.isEmpty()) {
    emit errorOccurred(QString::fromUtf8("播放地址为空"));
    return;
  }
  const QUrl media_url = ResolvePlayUrl(url_text);
  if (!media_url.isValid()) {
    emit errorOccurred(QString::fromUtf8("播放地址无效"));
    return;
  }
  if (media_url.isLocalFile() && !QFileInfo::exists(media_url.toLocalFile())) {
    emit errorOccurred(QString::fromUtf8("本地视频文件不存在：") +
                       media_url.toLocalFile());
    return;
  }

  stop();

  QString error;
  if (!EnsureAudioOutput(&error)) {
    emit errorOccurred(error);
    return;
  }

  pending_title_ = video.title;
  paused_ = false;
  playing_ = true;
  has_presented_frame_ = false;
  duration_sec_ = 0.0;
  position_sec_ = 0.0;
  EmitProgress();

  if (player_widget_ != NULL) {
    player_widget_->setLoadingCoverVisible(true);
    player_widget_->clearFrame();
  }

  const QString open_url = media_url.isLocalFile() ? media_url.toLocalFile()
                                                   : media_url.toString();
  engine_->startPlayback(open_url);
  emit statusMessage(QString::fromUtf8("正在播放：") + video.title);
}

void PlaybackController::pause() {
  if (!playing_) {
    emit statusMessage(QString::fromUtf8("当前没有播放中的视频"));
    return;
  }
  if (!paused_) {
    paused_ = true;
    engine_->setPaused(true);
    if (audio_out_ != NULL) {
      audio_out_->suspend();
    }
    emit statusMessage(QString::fromUtf8("已暂停"));
    return;
  }
  paused_ = false;
  engine_->setPaused(false);
  if (audio_out_ != NULL) {
    audio_out_->resume();
  }
  emit statusMessage(QString::fromUtf8("继续播放：") + pending_title_);
}

void PlaybackController::stop() {
  engine_->requestStop();
  engine_->wait(5000);
  TearDownAudio();
  playing_ = false;
  paused_ = false;
  has_presented_frame_ = false;
  duration_sec_ = 0.0;
  position_sec_ = 0.0;
  EmitProgress();
  if (player_widget_ != NULL) {
    player_widget_->setLoadingCoverVisible(false);
    player_widget_->showHintPage();
  }
  UpdateHint(QString::fromUtf8("双击列表条目，或点击播放"));
  emit statusMessage(QString::fromUtf8("已停止"));
}

void PlaybackController::onVideoFrame(const QImage& frame, double /*pts_sec*/) {
  if (!playing_ || paused_ || frame.isNull()) {
    return;
  }
  if (player_widget_ != NULL) {
    player_widget_->setFrame(frame);
    if (!has_presented_frame_) {
      has_presented_frame_ = true;
      player_widget_->setLoadingCoverVisible(false);
    }
  }
}

void PlaybackController::onAudioSamples(const QByteArray& pcm,
                                        double /*pts_sec*/) {
  if (!playing_ || paused_ || audio_device_ == NULL || pcm.isEmpty()) {
    return;
  }
  audio_device_->write(pcm);
}

void PlaybackController::onDurationReady(double duration_sec) {
  duration_sec_ = duration_sec;
  EmitProgress();
}

void PlaybackController::onPositionChanged(double position_sec) {
  if (scrubbing_) {
    return;
  }
  position_sec_ = position_sec;
  EmitProgress();
}

void PlaybackController::onSeekCompleted(double position_sec) {
  position_sec_ = position_sec;
  EmitProgress();
  TearDownAudio();
  if (playing_) {
    QString error;
    EnsureAudioOutput(&error);
    if (paused_ && audio_out_ != NULL) {
      audio_out_->suspend();
    }
  }
}

void PlaybackController::onEngineFailed(const QString& message) {
  playing_ = false;
  TearDownAudio();
  if (player_widget_ != NULL) {
    player_widget_->setLoadingCoverVisible(false);
    player_widget_->showHintPage();
  }
  UpdateHint(message);
  emit errorOccurred(message);
}

void PlaybackController::onEngineEnded() {
  playing_ = false;
  if (duration_sec_ > 0.0) {
    position_sec_ = duration_sec_;
    EmitProgress();
  }
  TearDownAudio();
  if (player_widget_ != NULL) {
    player_widget_->setLoadingCoverVisible(false);
    player_widget_->showHintPage();
  }
  UpdateHint(QString::fromUtf8("播放结束。可重新选择视频播放。"));
  emit statusMessage(QString::fromUtf8("播放结束"));
}

}  // namespace client
}  // namespace square_dance
