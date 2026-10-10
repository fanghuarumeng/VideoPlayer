#include "square_dance/client/elder_ui/VideoListDelegate.h"

#include "square_dance/client/api/CatalogModel.h"
#include "square_dance/client/elder_ui/AppIcons.h"
#include "square_dance/client/elder_ui/AppTheme.h"

#include <QApplication>
#include <QPainter>

namespace square_dance {
namespace client {

VideoListDelegate::VideoListDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {}

QSize VideoListDelegate::sizeHint(const QStyleOptionViewItem& /*option*/,
                                const QModelIndex& /*index*/) const {
  return QSize(0, 52);
}

void VideoListDelegate::paint(QPainter* painter,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
  const QString title = index.data(CatalogModel::TitleRole).toString();
  const QString source = index.data(CatalogModel::SourceTypeRole).toString();

  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);
  if (option.state & QStyle::State_Selected) {
    painter->fillRect(option.rect, QColor(80, 134, 255, 51));
  } else if (option.state & QStyle::State_MouseOver) {
    painter->fillRect(option.rect, QColor(255, 255, 255, 20));
  } else {
    painter->fillRect(option.rect, Qt::transparent);
  }

  const QRect icon_rect(option.rect.left() + 10, option.rect.center().y() - 14,
                        28, 28);
  AppIcons::Film().paint(painter, icon_rect);

  const QColor title_color =
      (option.state & QStyle::State_Selected) ? QColor(Qt::white)
                                              : QColor(AppTheme::TextPrimary());
  const QColor sub_color =
      (option.state & QStyle::State_Selected)
          ? QColor(QString::fromUtf8("#e0f2fe"))
          : QColor(AppTheme::TextSecondary());

  QRect text_rect = option.rect.adjusted(46, 8, -8, -8);
  QFont title_font = AppTheme::UiFont();
  title_font.setBold(true);
  painter->setFont(title_font);
  painter->setPen(title_color);
  painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignTop | Qt::TextSingleLine,
                    title);

  QFont cap = AppTheme::CaptionFont();
  painter->setFont(cap);
  painter->setPen(sub_color);
  painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignBottom | Qt::TextSingleLine,
                    source);

  painter->restore();
}

}  // namespace client
}  // namespace square_dance
