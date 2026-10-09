#include "square_dance/client/api/CatalogModel.h"
#include "square_dance/client/api/catalog_json.h"

#include <gtest/gtest.h>

namespace {

square_dance::client::Video MakeVideo(qint64 id, const char* title) {
  square_dance::client::Video v;
  v.id = id;
  v.external_id = QString::fromUtf8("ext-") + QString::number(id);
  v.title = QString::fromUtf8(title);
  v.play_url = QString::fromUtf8("https://example.com/") + QString::number(id) +
               QString::fromUtf8(".mp4");
  v.download_url = v.play_url;
  v.source_type = QString::fromUtf8("authorized");
  v.updated_at = QString::fromUtf8("2026-10-09T08:00:00Z");
  return v;
}

}  // namespace

TEST(CatalogModel, ResetFromSearchReplacesRows) {
  square_dance::client::CatalogModel model;
  QVector<square_dance::client::Video> first;
  first.push_back(MakeVideo(1, "a"));
  first.push_back(MakeVideo(2, "b"));
  model.resetFromSearch(first);
  ASSERT_EQ(model.rowCount(), 2);
  EXPECT_EQ(model.videoAt(0).title, QString::fromUtf8("a"));

  QVector<square_dance::client::Video> second;
  second.push_back(MakeVideo(9, "only"));
  model.resetFromSearch(second);
  ASSERT_EQ(model.rowCount(), 1);
  EXPECT_EQ(model.videoAt(0).id, 9);
  EXPECT_EQ(model.videoAt(0).title, QString::fromUtf8("only"));
}

TEST(CatalogModel, ResetFromEmptyClears) {
  square_dance::client::CatalogModel model;
  QVector<square_dance::client::Video> first;
  first.push_back(MakeVideo(1, "a"));
  model.resetFromSearch(first);
  model.resetFromSearch(QVector<square_dance::client::Video>());
  EXPECT_EQ(model.rowCount(), 0);
}
