#pragma once

#include "video_player/client/api/Video.h"

#include <QByteArray>
#include <QString>
#include <QVector>

namespace video_player {
namespace client {

bool ParseVideoJson(const QByteArray& json, Video* out, QString* error);
bool ParseVideoListJson(const QByteArray& json,
                        QVector<Video>* out,
                        QString* error);
QByteArray VideoToJsonBytes(const Video& video);

}  // namespace client
}  // namespace video_player
