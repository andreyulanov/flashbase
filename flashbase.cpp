#include <math.h>
#include "flashbase.h"
#include "flashserialize.h"

using namespace flashmath;

namespace flashmath
{
double deg2rad(double deg)
{
  return deg * M_PI / 180;
}
double rad2deg(double rad)
{
  return rad / M_PI * 180;
}
double sqr(double x)
{
  return x * x;
}
double getAngle(QPoint p1, QPoint p2)
{
  return atan2(p2.y() - p1.y(), p2.x() - p1.x());
}
}

double FlashGeoCoor::longitude() const
{
  return lon * 1E-7;
}

double FlashGeoCoor::latitude() const
{
  return -lat * 1E-7;
}

bool FlashGeoCoor::isValid()
{
  return !(lat == 0 && lon == 0);
}

FlashGeoCoor::FlashGeoCoor()
{
  lat = 0;
  lon = 0;
}

FlashGeoCoor::FlashGeoCoor(int _lat, int _lon)
{
  lat = _lat;
  lon = _lon;
}

FlashGeoCoor FlashGeoCoor::fromDegs(double lat, double lon)
{
  FlashGeoCoor ret;
  ret.lon = round(lon * 1E+7);
  ret.lat = -round(lat * 1E+7);
  return ret;
}
FlashGeoCoor FlashGeoCoor::fromMeters(QPointF m)
{
  double lon_deg = rad2deg(m.x() / earth_r);
  double lat_deg =
      -rad2deg(2 * atan(exp(m.y() / earth_r)) - M_PI / 2);
  return fromDegs(lat_deg, lon_deg);
}

QPointF FlashGeoCoor::toMeters() const
{
  double x = deg2rad(longitude()) * earth_r;
  double y = log(tan(deg2rad(-latitude()) / 2 + M_PI / 4)) * earth_r;
  return {x, y};
}

FlashGeoRect FlashGeoRect::united(const FlashGeoRect& v) const
{
  FlashGeoRect res;
  res.top_left.lon = std::min(top_left.lon, v.top_left.lon);
  res.top_left.lat = std::min(top_left.lat, v.top_left.lat);
  res.bottom_right.lon =
      std::max(bottom_right.lon, v.bottom_right.lon);
  res.bottom_right.lat =
      std::max(bottom_right.lat, v.bottom_right.lat);
  return res;
}

bool FlashGeoRect::isNull() const
{
  return top_left.lon == bottom_right.lon &&
         top_left.lat == bottom_right.lat;
}

QRectF FlashGeoRect::toMeters() const
{
  return {top_left.toMeters(), bottom_right.toMeters()};
}

QSizeF FlashGeoRect::getSizeMeters() const
{
  auto   top_left_m     = top_left.toMeters();
  auto   bottom_right_m = bottom_right.toMeters();
  double w              = bottom_right_m.x() - top_left_m.x();
  double h              = bottom_right_m.y() - top_left_m.y();
  return {w, h};
}

QRectF FlashGeoRect::toRectM() const
{
  return {top_left.toMeters(), bottom_right.toMeters()};
}

FlashGeoRect FlashGeoPolygon::getFrame() const
{
  using namespace std;
  auto minx = numeric_limits<int>().max();
  auto miny = numeric_limits<int>().max();
  auto maxx = numeric_limits<int>().min();
  auto maxy = numeric_limits<int>().min();
  for (auto& p: *this)
  {
    minx = min(p.lon, minx);
    miny = min(p.lat, miny);
    maxx = max(p.lon, maxx);
    maxy = max(p.lat, maxy);
  }
  FlashGeoRect rect;
  rect.top_left.lon     = minx;
  rect.top_left.lat     = miny;
  rect.bottom_right.lon = maxx;
  rect.bottom_right.lat = maxy;
  return rect;
}

void FlashGeoPolygon::save(QByteArray& ba,
                           int         coor_precision_coef) const
{
  using namespace FlashSerialize;
  write(ba, count());

  if (count() == 1)
  {
    write(ba, first().lat);
    write(ba, first().lon);
    return;
  }
  if (count() == 2)
  {
    write(ba, first().lat);
    write(ba, first().lon);
    write(ba, last().lat);
    write(ba, last().lon);
    return;
  }

  auto frame = getFrame();
  write(ba, frame.top_left);
  int span_lat =
      std::max(1, frame.bottom_right.lat - frame.top_left.lat);
  int span_lon =
      std::max(1, frame.bottom_right.lon - frame.top_left.lon);

  span_lat /= coor_precision_coef;
  span_lon /= coor_precision_coef;

  int span_type = 0;

  if (span_lat <= 0xff && span_lon <= 0xff)
    span_type = 0;
  else if (span_lat <= 0xffff && span_lon <= 0xffff)
    span_type = 1;
  else
    span_type = 2;

  if (span_lat == 0 || span_lon == 0)
    span_type = 2;

  write(ba, (uchar)span_type);

  if (span_type == 2)
  {
    for (auto& point: (*this))
      write(ba, point);
    return;
  }

  for (auto& point: (*this))
  {
    int dlat =
        1.0 * (point.lat - frame.top_left.lat) / coor_precision_coef;
    int dlon =
        1.0 * (point.lon - frame.top_left.lon) / coor_precision_coef;
    if (span_type == 0)
    {
      write(ba, (uchar)dlat);
      write(ba, (uchar)dlon);
    }
    else
    {
      write(ba, (ushort)dlat);
      write(ba, (ushort)dlon);
    }
  }
}

QPolygonF FlashGeoPolygon::toPolygonM()
{
  QPolygonF ret;
  for (auto p: *this)
    ret.append(p.toMeters());
  return ret;
}

void FlashGeoPolygon::load(const QByteArray& ba, int& pos,
                           int coor_precision_coef)
{
  using namespace FlashSerialize;
  int point_count;
  read(ba, pos, point_count);
  resize(point_count);

  if (count() <= 2)
  {
    for (auto& p: *this)
      read(ba, pos, p);
    return;
  }

  FlashGeoCoor top_left;
  read(ba, pos, top_left);

  uchar span_type;
  read(ba, pos, span_type);

  if (span_type == 2)
  {
    for (auto& point: (*this))
      read(ba, pos, point);
    return;
  }

  for (auto& point: (*this))
  {
    int dlat = 0;
    int dlon = 0;

    if (span_type == 0)
    {
      uchar _dlat = 0;
      uchar _dlon = 0;
      read(ba, pos, _dlat);
      read(ba, pos, _dlon);
      dlat = _dlat;
      dlon = _dlon;
    }
    else
    {
      ushort _dlat = 0;
      ushort _dlon = 0;
      read(ba, pos, _dlat);
      read(ba, pos, _dlon);
      dlat = _dlat;
      dlon = _dlon;
    }
    dlat *= coor_precision_coef;
    dlon *= coor_precision_coef;
    point.lat = top_left.lat + dlat;
    point.lon = top_left.lon + dlon;
  }
}
