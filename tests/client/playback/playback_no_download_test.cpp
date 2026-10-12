#include "video_player/client/api/Video.h"
#include "video_player/client/playback/PlaybackController.h"
#include "video_player/client/playback/VideoPlayerWidget.h"

#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <gtest/gtest.h>

namespace {

video_player::client::Video MakeVideo(const QString& play_url) {
  video_player::client::Video v;
  v.id = 1;
  v.external_id = QString::fromUtf8("p1");
  v.title = QString::fromUtf8("t");
  v.play_url = play_url;
  v.download_url = QString::fromUtf8("https://example.com/should-not-fetch.mp4");
  v.source_type = QString::fromUtf8("authorized");
  v.updated_at = QString::fromUtf8("2026-10-09T08:00:00Z");
  return v;
}

int CountFilesRecursive(const QString& dir_path) {
  int count = 0;
  QDir dir(dir_path);
  const QFileInfoList entries =
      dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
  for (int i = 0; i < entries.size(); ++i) {
    if (entries.at(i).isDir()) {
      count += CountFilesRecursive(entries.at(i).absoluteFilePath());
    } else {
      ++count;
    }
  }
  return count;
}

}  // namespace

TEST(Playback, EmptyUrlEmitsErrorAndLeavesDownloadDirUntouched) {
  QTemporaryDir download_dir;
  ASSERT_TRUE(download_dir.isValid());
  const int before = CountFilesRecursive(download_dir.path());

  video_player::client::PlaybackController controller;
  video_player::client::VideoPlayerWidget widget;
  controller.setPlayerWidget(&widget);

  QSignalSpy error_spy(&controller,
                       &video_player::client::PlaybackController::errorOccurred);
  controller.play(MakeVideo(QString()));
  ASSERT_EQ(error_spy.count(), 1);
  EXPECT_EQ(CountFilesRecursive(download_dir.path()), before);

  controller.pause();
  controller.stop();
  EXPECT_EQ(CountFilesRecursive(download_dir.path()), before);
}

TEST(Playback, MissingLocalFileEmitsErrorWithoutWritingDownloadDir) {
  QTemporaryDir download_dir;
  ASSERT_TRUE(download_dir.isValid());
  const int before = CountFilesRecursive(download_dir.path());

  video_player::client::PlaybackController controller;
  video_player::client::VideoPlayerWidget widget;
  controller.setPlayerWidget(&widget);
  QSignalSpy error_spy(&controller,
                       &video_player::client::PlaybackController::errorOccurred);
  controller.play(MakeVideo(QString::fromUtf8(
      "file:///C:/video_player_missing_media_do_not_create.mp4")));
  ASSERT_GE(error_spy.count(), 1);
  EXPECT_EQ(CountFilesRecursive(download_dir.path()), before);
}
