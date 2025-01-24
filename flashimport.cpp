#include "flashimport.h"
#include <QDebug>

namespace flashimport
{
QVector<FlashGeoPolygon> loadPolyFile(QString poly_path)
{
  QVector<FlashGeoPolygon> ret;
  QFile                    f(poly_path);
  if (!f.open(QIODevice::ReadOnly))
  {
    qDebug() << "Error opening" << poly_path;
    return ret;
  }
  QTextStream     in(&f);
  FlashGeoPolygon curr_polygon;
  while (!in.atEnd())
  {
    auto line  = in.readLine();
    auto items = line.split(' ', Qt::SkipEmptyParts);
    if (items.count() == 2)
    {
      double lon = items.at(0).toDouble();
      double lat = items.at(1).toDouble();
      curr_polygon.append(FlashGeoCoor().fromDegs(lat, lon));
    }
    else if (!curr_polygon.isEmpty())
    {
      ret.append(curr_polygon);
      curr_polygon.clear();
    }
  }
  return ret;
}
}
