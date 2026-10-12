#include "video_player/client/playlist/PlaylistStore.h"

#include <QFile>
#include <gtest/gtest.h>

namespace {

video_player::client::PlaylistItem MakeItem(qint64 id, const char* title) {
  video_player::client::PlaylistItem item;
  item.id = id;
  item.title = QString::fromUtf8(title);
  item.play_url =
      QString::fromUtf8("http://127.0.0.1:8080/media/videos/%1.mp4").arg(id);
  item.thumbnail_url =
      QString::fromUtf8("http://127.0.0.1:8080/media/thumbs/%1.jpg").arg(id);
  return item;
}

}  // namespace

class PlaylistStoreTest : public ::testing::Test {
 protected:
  void SetUp() override {
    path_ = QString::fromUtf8("playlist_store_test.json");
    QFile::remove(path_);
  }

  void TearDown() override {
    QFile::remove(path_);
  }

  QString path_;
};

TEST_F(PlaylistStoreTest, AppendDedupeClearAndRoundTrip) {
  video_player::client::PlaylistStore store(path_);
  EXPECT_TRUE(store.append(MakeItem(1, "One")));
  EXPECT_TRUE(store.append(MakeItem(2, "Two")));
  EXPECT_FALSE(store.append(MakeItem(1, "One Again")));
  ASSERT_EQ(store.items().size(), 2);
  EXPECT_EQ(store.items()[0].title, QString::fromUtf8("One"));
  EXPECT_EQ(store.items()[1].id, 2);
  store.setCurrentIndex(1);
  EXPECT_EQ(store.currentIndex(), 1);

  video_player::client::PlaylistStore loaded(path_);
  QString error;
  ASSERT_TRUE(loaded.load(&error)) << error.toStdString();
  ASSERT_EQ(loaded.items().size(), 2);
  EXPECT_EQ(loaded.items()[0].play_url, store.items()[0].play_url);
  EXPECT_EQ(loaded.items()[1].thumbnail_url, store.items()[1].thumbnail_url);
  EXPECT_EQ(loaded.currentIndex(), 1);

  loaded.clear();
  EXPECT_TRUE(loaded.items().isEmpty());
  EXPECT_EQ(loaded.currentIndex(), -1);

  video_player::client::PlaylistStore again(path_);
  ASSERT_TRUE(again.load(&error)) << error.toStdString();
  EXPECT_TRUE(again.items().isEmpty());
}

TEST_F(PlaylistStoreTest, LoadMissingFileStartsEmpty) {
  video_player::client::PlaylistStore store(
      QString::fromUtf8("playlist_store_missing.json"));
  QFile::remove(store.path());
  QString error;
  ASSERT_TRUE(store.load(&error)) << error.toStdString();
  EXPECT_TRUE(store.items().isEmpty());
  EXPECT_EQ(store.currentIndex(), -1);
}

TEST_F(PlaylistStoreTest, AppendFromVideo) {
  video_player::client::PlaylistStore store(path_);
  video_player::client::Video video;
  video.id = 9;
  video.title = QString::fromUtf8("V");
  video.play_url = QString::fromUtf8("http://127.0.0.1/x.mp4");
  video.thumbnail_url = QString::fromUtf8("http://127.0.0.1/x.jpg");
  EXPECT_TRUE(store.append(video));
  ASSERT_EQ(store.items().size(), 1);
  EXPECT_EQ(store.items()[0].id, 9);
}

TEST_F(PlaylistStoreTest, MergeFromVideoFillsThumbnail) {
  video_player::client::PlaylistStore store(path_);
  video_player::client::PlaylistItem item = MakeItem(3, "Clip");
  item.thumbnail_url.clear();
  EXPECT_TRUE(store.append(item));
  EXPECT_TRUE(store.items()[0].thumbnail_url.isEmpty());

  video_player::client::Video video;
  video.id = 3;
  video.title = QString::fromUtf8("Clip");
  video.play_url = item.play_url;
  video.thumbnail_url =
      QString::fromUtf8("http://127.0.0.1:8080/media/thumbs/3.jpg");
  EXPECT_TRUE(store.mergeFromVideo(video));
  EXPECT_EQ(store.items()[0].thumbnail_url, video.thumbnail_url);
  EXPECT_FALSE(store.mergeFromVideo(video));
}
