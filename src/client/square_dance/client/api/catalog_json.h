#pragma once

#include "square_dance/client/api/Video.h"

#include <QByteArray>
#include <QString>
#include <QVector>

namespace square_dance {
namespace client {

bool ParseVideoJson(const QByteArray& json, Video* out, QString* error);
bool ParseVideoListJson(const QByteArray& json,
                        QVector<Video>* out,
                        QString* error);
QByteArray VideoToJsonBytes(const Video& video);

}  // namespace client
}  // namespace square_dance
