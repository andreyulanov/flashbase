#pragma once

#include <QObject>
#include <QColor>
#include <QPixmap>
#include <QFile>
#include <QPainter>
#include <math.h>

namespace flashmath
{
constexpr double earth_r = 6378137;
double           deg2rad(double deg);
double           rad2deg(double rad);
double           getAngle(QPoint p1, QPoint p2);
double           sqr(double x);
template<typename T>
double getDistance(T p1, T p2)
{
  return sqrt(sqr(p1.x() - p2.x()) + sqr(p1.y() - p2.y()));
}
}

struct FlashGeoCoor
{
  FlashGeoCoor();
  explicit FlashGeoCoor(int lat, int lon);
  static FlashGeoCoor fromDegs(double lat, double lon);
  static FlashGeoCoor fromMeters(QPointF m);
  QPointF             toMeters() const;
  double              longitude() const;
  double              latitude() const;
  bool                isValid();

  int lat = 0;
  int lon = 0;
};

struct FlashGeoRect
{
  FlashGeoCoor top_left;
  FlashGeoCoor bottom_right;
  FlashGeoRect united(const FlashGeoRect&) const;
  bool         isNull() const;
  QRectF       toMeters() const;
  QSizeF       getSizeMeters() const;
  QRectF       toRectM() const;
};

struct FlashGeoPolygon: public QVector<FlashGeoCoor>
{
  FlashGeoRect getFrame() const;
  void         save(QByteArray& ba, int coor_precision_coef) const;
  void load(const QByteArray& ba, int& pos, int coor_precision_coef);
  QPolygonF toPolygonM();
};

Q_DECLARE_METATYPE(FlashGeoPolygon)
