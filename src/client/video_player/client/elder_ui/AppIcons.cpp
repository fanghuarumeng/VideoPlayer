#include "video_player/client/elder_ui/AppIcons.h"

namespace video_player {
namespace client {

namespace {
QIcon Load(const char* path) { return QIcon(QString::fromUtf8(path)); }
}  // namespace

QIcon AppIcons::Logo() { return Load(":/app/icons/logo.svg"); }
QIcon AppIcons::Search() { return Load(":/app/icons/search.svg"); }
QIcon AppIcons::Play() { return Load(":/app/icons/play.svg"); }
QIcon AppIcons::Pause() { return Load(":/app/icons/pause.svg"); }
QIcon AppIcons::Stop() { return Load(":/app/icons/stop.svg"); }
QIcon AppIcons::Download() { return Load(":/app/icons/download.svg"); }
QIcon AppIcons::Film() { return Load(":/app/icons/film.svg"); }

}  // namespace client
}  // namespace video_player
