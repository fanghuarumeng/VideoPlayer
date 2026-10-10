#include "square_dance/client/playback/FfmpegPlaybackEngine.h"

#include <QFile>
#include <QFileInfo>
#include <QUrl>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace square_dance {
namespace client {
namespace {

QString AvErr(int err) {
  char buf[256];
  av_strerror(err, buf, sizeof(buf));
  return QString::fromUtf8(buf);
}

QByteArray UrlForFfmpeg(const QString& url_text) {
  const QUrl url(url_text);
  if (url.isLocalFile()) {
    return QFile::encodeName(url.toLocalFile());
  }
  if (url.scheme().isEmpty() && QFileInfo::exists(url_text)) {
    return QFile::encodeName(url_text);
  }
  return url_text.toUtf8();
}

int InterruptCallback(void* opaque) {
  FfmpegPlaybackEngine* self = static_cast<FfmpegPlaybackEngine*>(opaque);
  return (self != NULL && self->ShouldAbort()) ? 1 : 0;
}

}  // namespace

FfmpegPlaybackEngine::FfmpegPlaybackEngine(QObject* parent)
    : QThread(parent),
      stop_requested_(false),
      paused_(false),
      has_job_(false),
      clock_started_(false),
      pts_origin_sec_(0.0),
      pause_accumulated_ms_(0),
      pause_wall_mark_valid_(false),
      pause_wall_mark_ms_(0),
      duration_sec_(0.0),
      last_reported_position_sec_(-1.0),
      pending_seek_(false),
      pending_seek_sec_(0.0),
      format_ctx_(NULL),
      video_codec_ctx_(NULL),
      audio_codec_ctx_(NULL),
      sws_ctx_(NULL),
      swr_ctx_(NULL),
      video_stream_(-1),
      audio_stream_(-1),
      frame_w_(0),
      frame_h_(0) {}

FfmpegPlaybackEngine::~FfmpegPlaybackEngine() {
  requestStop();
  wait(8000);
  CloseInput();
}

void FfmpegPlaybackEngine::startPlayback(const QString& url) {
  requestStop();
  wait(8000);
  {
    QMutexLocker lock(&mutex_);
    url_ = url;
    stop_requested_ = false;
    paused_ = false;
    has_job_ = true;
    clock_started_ = false;
    pts_origin_sec_ = 0.0;
    pause_accumulated_ms_ = 0;
    pause_wall_mark_valid_ = false;
    pause_wall_mark_ms_ = 0;
    pending_seek_ = false;
    duration_sec_ = 0.0;
    last_reported_position_sec_ = -1.0;
  }
  start();
}

void FfmpegPlaybackEngine::requestStop() {
  {
    QMutexLocker lock(&mutex_);
    stop_requested_ = true;
    paused_ = false;
    pause_wall_mark_valid_ = false;
  }
  requestInterruption();
  pause_wait_.wakeAll();
}

void FfmpegPlaybackEngine::setPaused(bool paused) {
  QMutexLocker lock(&mutex_);
  if (paused == paused_) {
    return;
  }
  paused_ = paused;
  if (!paused_) {
    pause_wait_.wakeAll();
  }
}

bool FfmpegPlaybackEngine::ShouldAbort() const {
  return stop_requested_ || isInterruptionRequested();
}

void FfmpegPlaybackEngine::seekTo(double position_sec) {
  QMutexLocker lock(&mutex_);
  if (duration_sec_ > 0.0) {
    pending_seek_sec_ = qBound(0.0, position_sec, duration_sec_);
  } else {
    pending_seek_sec_ = qMax(0.0, position_sec);
  }
  pending_seek_ = true;
  pause_wait_.wakeAll();
}

double FfmpegPlaybackEngine::ComputeDurationSec() const {
  AVFormatContext* fmt = static_cast<AVFormatContext*>(format_ctx_);
  if (fmt == NULL) {
    return 0.0;
  }
  if (fmt->duration != AV_NOPTS_VALUE && fmt->duration > 0) {
    return static_cast<double>(fmt->duration) / AV_TIME_BASE;
  }
  double max_stream = 0.0;
  for (unsigned i = 0; i < fmt->nb_streams; ++i) {
    AVStream* st = fmt->streams[i];
    if (st->duration > 0 && st->time_base.den > 0) {
      const double d = st->duration * av_q2d(st->time_base);
      if (d > max_stream) {
        max_stream = d;
      }
    }
  }
  return max_stream;
}

bool FfmpegPlaybackEngine::PerformSeek(double target_sec) {
  AVFormatContext* fmt = static_cast<AVFormatContext*>(format_ctx_);
  if (fmt == NULL) {
    return false;
  }
  const int64_t ts = static_cast<int64_t>(target_sec * AV_TIME_BASE);
  if (av_seek_frame(fmt, -1, ts, AVSEEK_FLAG_BACKWARD) < 0) {
    return false;
  }
  if (video_codec_ctx_ != NULL) {
    avcodec_flush_buffers(static_cast<AVCodecContext*>(video_codec_ctx_));
  }
  if (audio_codec_ctx_ != NULL) {
    avcodec_flush_buffers(static_cast<AVCodecContext*>(audio_codec_ctx_));
  }
  clock_started_ = false;
  pts_origin_sec_ = target_sec;
  pause_accumulated_ms_ = 0;
  pause_wall_mark_valid_ = false;
  wall_timer_.restart();
  last_reported_position_sec_ = target_sec - 1.0;
  return true;
}

void FfmpegPlaybackEngine::MaybeEmitPosition(double pts_sec) {
  double pos = pts_sec;
  if (duration_sec_ > 0.0) {
    pos = qBound(0.0, pos, duration_sec_);
  } else if (pos < 0.0) {
    pos = 0.0;
  }
  if (last_reported_position_sec_ < 0.0 ||
      pos - last_reported_position_sec_ >= 0.2 ||
      (duration_sec_ > 0.0 && duration_sec_ - pos < 0.05)) {
    last_reported_position_sec_ = pos;
    emit positionChanged(pos);
  }
}

bool FfmpegPlaybackEngine::OpenInput(const QString& url, QString* error) {
  CloseInput();
  avformat_network_init();

  AVFormatContext* fmt = avformat_alloc_context();
  if (fmt == NULL) {
    if (error != NULL) {
      *error = QString::fromUtf8("avformat_alloc_context failed");
    }
    return false;
  }
  fmt->interrupt_callback.callback = InterruptCallback;
  fmt->interrupt_callback.opaque = this;

  const QByteArray path = UrlForFfmpeg(url);
  AVFormatContext* opened = fmt;
  int rc = avformat_open_input(&opened, path.constData(), NULL, NULL);
  if (rc < 0) {
    avformat_free_context(fmt);
    if (error != NULL) {
      *error = QString::fromUtf8("打开媒体失败：") + AvErr(rc);
    }
    return false;
  }
  format_ctx_ = opened;

  rc = avformat_find_stream_info(static_cast<AVFormatContext*>(format_ctx_), NULL);
  if (rc < 0) {
    if (error != NULL) {
      *error = QString::fromUtf8("读取媒体信息失败：") + AvErr(rc);
    }
    CloseInput();
    return false;
  }
  return true;
}

void FfmpegPlaybackEngine::CloseInput() {
  if (sws_ctx_ != NULL) {
    sws_freeContext(static_cast<SwsContext*>(sws_ctx_));
    sws_ctx_ = NULL;
  }
  if (swr_ctx_ != NULL) {
    SwrContext* swr = static_cast<SwrContext*>(swr_ctx_);
    swr_free(&swr);
    swr_ctx_ = NULL;
  }
  if (video_codec_ctx_ != NULL) {
    AVCodecContext* ctx = static_cast<AVCodecContext*>(video_codec_ctx_);
    avcodec_free_context(&ctx);
    video_codec_ctx_ = NULL;
  }
  if (audio_codec_ctx_ != NULL) {
    AVCodecContext* ctx = static_cast<AVCodecContext*>(audio_codec_ctx_);
    avcodec_free_context(&ctx);
    audio_codec_ctx_ = NULL;
  }
  if (format_ctx_ != NULL) {
    AVFormatContext* fmt = static_cast<AVFormatContext*>(format_ctx_);
    avformat_close_input(&fmt);
    format_ctx_ = NULL;
  }
  video_stream_ = -1;
  audio_stream_ = -1;
}

bool FfmpegPlaybackEngine::OpenCodecs(QString* error) {
  AVFormatContext* fmt = static_cast<AVFormatContext*>(format_ctx_);
  video_stream_ = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  audio_stream_ = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

  if (video_stream_ < 0 && audio_stream_ < 0) {
    if (error != NULL) {
      *error = QString::fromUtf8("未找到可播放的音视频流");
    }
    return false;
  }

  if (video_stream_ >= 0) {
    AVStream* st = fmt->streams[video_stream_];
    const AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
    if (codec == NULL) {
      if (error != NULL) {
        *error = QString::fromUtf8("不支持的视频编码");
      }
      return false;
    }
    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (ctx == NULL) {
      if (error != NULL) {
        *error = QString::fromUtf8("视频解码器分配失败");
      }
      return false;
    }
    int rc = avcodec_parameters_to_context(ctx, st->codecpar);
    if (rc < 0 || (rc = avcodec_open2(ctx, codec, NULL)) < 0) {
      avcodec_free_context(&ctx);
      if (error != NULL) {
        *error = QString::fromUtf8("打开视频解码器失败：") + AvErr(rc);
      }
      return false;
    }
    video_codec_ctx_ = ctx;
    frame_w_ = ctx->width;
    frame_h_ = ctx->height;
    if (frame_w_ <= 0 || frame_h_ <= 0) {
      if (error != NULL) {
        *error = QString::fromUtf8("无效的视频尺寸");
      }
      return false;
    }
    sws_ctx_ = sws_getContext(frame_w_, frame_h_, ctx->pix_fmt, frame_w_,
                              frame_h_, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL,
                              NULL, NULL);
    if (sws_ctx_ == NULL) {
      if (error != NULL) {
        *error = QString::fromUtf8("颜色转换初始化失败");
      }
      return false;
    }
  }

  if (audio_stream_ >= 0) {
    AVStream* st = fmt->streams[audio_stream_];
    const AVCodec* codec = avcodec_find_decoder(st->codecpar->codec_id);
    if (codec == NULL) {
      if (error != NULL) {
        *error = QString::fromUtf8("不支持的音频编码");
      }
      return false;
    }
    AVCodecContext* ctx = avcodec_alloc_context3(codec);
    if (ctx == NULL) {
      if (error != NULL) {
        *error = QString::fromUtf8("音频解码器分配失败");
      }
      return false;
    }
    int rc = avcodec_parameters_to_context(ctx, st->codecpar);
    if (rc < 0 || (rc = avcodec_open2(ctx, codec, NULL)) < 0) {
      avcodec_free_context(&ctx);
      if (error != NULL) {
        *error = QString::fromUtf8("打开音频解码器失败：") + AvErr(rc);
      }
      return false;
    }
    audio_codec_ctx_ = ctx;

    SwrContext* swr = swr_alloc();
    if (swr == NULL) {
      if (error != NULL) {
        *error = QString::fromUtf8("音频重采样分配失败");
      }
      return false;
    }
#if LIBAVUTIL_VERSION_MAJOR >= 57
    AVChannelLayout in_ch;
    if (ctx->ch_layout.nb_channels > 0) {
      rc = av_channel_layout_copy(&in_ch, &ctx->ch_layout);
    } else {
      av_channel_layout_default(&in_ch, kAudioChannels);
      rc = 0;
    }
    if (rc < 0) {
      swr_free(&swr);
      if (error != NULL) {
        *error = QString::fromUtf8("音频声道布局无效");
      }
      return false;
    }
    AVChannelLayout out_ch;
    av_channel_layout_default(&out_ch, kAudioChannels);
    rc = swr_alloc_set_opts2(&swr, &out_ch, AV_SAMPLE_FMT_S16, kAudioSampleRate,
                             &in_ch, ctx->sample_fmt, ctx->sample_rate, 0, NULL);
    av_channel_layout_uninit(&in_ch);
    av_channel_layout_uninit(&out_ch);
#else
    const int64_t in_layout =
        ctx->channel_layout != 0
            ? static_cast<int64_t>(ctx->channel_layout)
            : av_get_default_channel_layout(2);
    swr = swr_alloc_set_opts(
        swr, av_get_default_channel_layout(kAudioChannels), AV_SAMPLE_FMT_S16,
        kAudioSampleRate, in_layout, ctx->sample_fmt, ctx->sample_rate, 0,
        NULL);
    rc = (swr == NULL) ? AVERROR(ENOMEM) : 0;
#endif
    if (rc < 0 || swr_init(swr) < 0) {
      swr_free(&swr);
      if (error != NULL) {
        *error = QString::fromUtf8("音频重采样初始化失败");
      }
      return false;
    }
    swr_ctx_ = swr;
  }

  return true;
}

void FfmpegPlaybackEngine::run() {
  QString url;
  {
    QMutexLocker lock(&mutex_);
    if (!has_job_) {
      return;
    }
    url = url_;
    has_job_ = false;
  }

  QString error;
  if (!OpenInput(url, &error) || !OpenCodecs(&error)) {
    CloseInput();
    emit playbackFailed(error);
    return;
  }

  duration_sec_ = ComputeDurationSec();
  last_reported_position_sec_ = -1.0;
  pending_seek_ = false;
  emit durationReady(duration_sec_);
  emit positionChanged(0.0);

  DecodeLoop();
  CloseInput();
  if (!isInterruptionRequested()) {
    emit playbackEnded();
  }
}

void FfmpegPlaybackEngine::PaceToPts(double pts_sec) {
  // Shared wall clock across audio/video so neither stream races ahead.
  if (!clock_started_) {
    clock_started_ = true;
    pts_origin_sec_ = pts_sec;
    pause_accumulated_ms_ = 0;
    pause_wall_mark_valid_ = false;
    wall_timer_.restart();
    return;
  }

  while (!ShouldAbort()) {
    {
      QMutexLocker lock(&mutex_);
      if (paused_) {
        if (!pause_wall_mark_valid_) {
          pause_wall_mark_ms_ = wall_timer_.elapsed();
          pause_wall_mark_valid_ = true;
        }
        pause_wait_.wait(&mutex_, 50);
        continue;
      }
      if (pause_wall_mark_valid_) {
        pause_accumulated_ms_ += wall_timer_.elapsed() - pause_wall_mark_ms_;
        pause_wall_mark_valid_ = false;
      }
    }

    const double media_t = pts_sec - pts_origin_sec_;
    const double wall_t =
        static_cast<double>(wall_timer_.elapsed() - pause_accumulated_ms_) /
        1000.0;
    if (media_t <= wall_t + 0.005) {
      break;
    }
    const int sleep_ms = static_cast<int>((media_t - wall_t) * 1000.0);
    msleep(sleep_ms > 30 ? 30 : (sleep_ms > 0 ? sleep_ms : 1));
  }
}

void FfmpegPlaybackEngine::DecodeLoop() {
  AVFormatContext* fmt = static_cast<AVFormatContext*>(format_ctx_);
  AVCodecContext* vctx = static_cast<AVCodecContext*>(video_codec_ctx_);
  AVCodecContext* actx = static_cast<AVCodecContext*>(audio_codec_ctx_);
  SwsContext* sws = static_cast<SwsContext*>(sws_ctx_);
  SwrContext* swr = static_cast<SwrContext*>(swr_ctx_);

  AVPacket* packet = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  if (packet == NULL || frame == NULL) {
    av_packet_free(&packet);
    av_frame_free(&frame);
    emit playbackFailed(QString::fromUtf8("解码缓冲分配失败"));
    return;
  }

  QImage rgb(frame_w_, frame_h_, QImage::Format_RGB888);
  uint8_t* dst_slices[4] = {rgb.bits(), NULL, NULL, NULL};
  int dst_linesize[4] = {static_cast<int>(rgb.bytesPerLine()), 0, 0, 0};

  while (!ShouldAbort()) {
    {
      QMutexLocker lock(&mutex_);
      while (paused_ && !stop_requested_ && !isInterruptionRequested()) {
        pause_wait_.wait(&mutex_, 200);
      }
      if (stop_requested_) {
        break;
      }
      if (pending_seek_) {
        const double target = pending_seek_sec_;
        pending_seek_ = false;
        lock.unlock();
        if (PerformSeek(target)) {
          emit seekCompleted(target);
          MaybeEmitPosition(target);
        }
        continue;
      }
    }

    int rc = av_read_frame(fmt, packet);
    if (rc == AVERROR_EOF) {
      break;
    }
    if (rc < 0) {
      if (rc != AVERROR_EXIT) {
        emit playbackFailed(QString::fromUtf8("读取失败：") + AvErr(rc));
      }
      break;
    }

    if (packet->stream_index == video_stream_ && vctx != NULL && sws != NULL) {
      rc = avcodec_send_packet(vctx, packet);
      av_packet_unref(packet);
      if (rc < 0) {
        continue;
      }
      while (rc >= 0 && !ShouldAbort()) {
        rc = avcodec_receive_frame(vctx, frame);
        if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF) {
          break;
        }
        if (rc < 0) {
          break;
        }
        sws_scale(sws, frame->data, frame->linesize, 0, frame_h_, dst_slices,
                  dst_linesize);
        double pts = 0.0;
        if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
          pts = frame->best_effort_timestamp *
                av_q2d(fmt->streams[video_stream_]->time_base);
        }
        PaceToPts(pts);
        if (ShouldAbort()) {
          av_frame_unref(frame);
          break;
        }
        emit videoFrameReady(rgb.copy(), pts);
        av_frame_unref(frame);
      }
      continue;
    }

    if (packet->stream_index == audio_stream_ && actx != NULL && swr != NULL) {
      rc = avcodec_send_packet(actx, packet);
      av_packet_unref(packet);
      if (rc < 0) {
        continue;
      }
      while (rc >= 0 && !ShouldAbort()) {
        rc = avcodec_receive_frame(actx, frame);
        if (rc == AVERROR(EAGAIN) || rc == AVERROR_EOF) {
          break;
        }
        if (rc < 0) {
          break;
        }
        uint8_t* out_buf = NULL;
        const int max_out = av_rescale_rnd(
            swr_get_delay(swr, frame->sample_rate) + frame->nb_samples,
            kAudioSampleRate, frame->sample_rate, AV_ROUND_UP);
        av_samples_alloc(&out_buf, NULL, kAudioChannels, max_out,
                         AV_SAMPLE_FMT_S16, 0);
        const int converted =
            swr_convert(swr, &out_buf, max_out,
                        const_cast<const uint8_t**>(frame->extended_data),
                        frame->nb_samples);
        if (converted > 0 && out_buf != NULL) {
          const int bytes =
              converted * kAudioChannels * kAudioBytesPerSample;
          QByteArray pcm(reinterpret_cast<const char*>(out_buf), bytes);
          double pts = 0.0;
          if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
            pts = frame->best_effort_timestamp *
                  av_q2d(fmt->streams[audio_stream_]->time_base);
          }
          PaceToPts(pts);
          if (!ShouldAbort()) {
            MaybeEmitPosition(pts);
            emit audioSamplesReady(pcm, pts);
          }
        }
        if (out_buf != NULL) {
          av_freep(&out_buf);
        }
        av_frame_unref(frame);
      }
      continue;
    }

    av_packet_unref(packet);
  }

  av_packet_free(&packet);
  av_frame_free(&frame);
}

}  // namespace client
}  // namespace square_dance
