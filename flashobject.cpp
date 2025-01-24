#include <math.h>

#include <QDebug>
#include "flashobject.h"
#include "flashserialize.h"

void FlashObject::load(QVector<FlashClass>& class_list, int& pos,
                       const QByteArray& ba)

{
  using namespace FlashSerialize;

  read(ba, pos, class_idx);
  auto cl = &class_list[class_idx];

  read(ba, pos, attributes);

  if (cl->type == FlashClass::Point)
  {
    FlashGeoCoor p;
    memcpy((char*)&p, (char*)&ba.data()[pos], sizeof(p));
    pos += sizeof(p);
    FlashGeoPolygon polygon;
    polygon.append(p);
    polygons.append(polygon);
    frame.top_left     = p;
    frame.bottom_right = p;
    return;
  }

  uchar is_multi_polygon;
  read(ba, pos, is_multi_polygon);

  if (is_multi_polygon)
  {
    int polygon_count;
    read(ba, pos, polygon_count);
    polygons.resize(polygon_count);
    for (std::size_t i = 0; auto& polygon: polygons)
    {
      polygon.load(ba, pos, cl->coor_precision_coef);
      if (i++ == 0)
        frame = polygon.getFrame();
      else
        frame = frame.united(polygon.getFrame());
    }
    read(ba, pos, inner_polygon_start_idx);
  }
  else
  {
    polygons.resize(1);
    polygons[0].load(ba, pos, cl->coor_precision_coef);
    frame = polygons[0].getFrame();
  }
}

void FlashObject::save(const QVector<FlashClass>& class_list,
                       QByteArray&                ba) const

{
  using namespace FlashSerialize;

  auto cl = &class_list[class_idx];
  write(ba, class_idx);
  write(ba, attributes);

  if (polygons.isEmpty() || polygons.first().isEmpty())
  {
    qDebug() << "save error: no geometry defined for point object"
             << attributes.value("name");
    return;
  }

  if (cl->type == FlashClass::Point)
  {
    write(ba, polygons.first().first());
    return;
  }

  write(ba, (uchar)(polygons.count() > 1));

  if (polygons.count() == 1)
  {
    polygons[0].save(ba, cl->coor_precision_coef);
    return;
  }
  else
  {
    write(ba, polygons.count());
    for (auto& polygon: polygons)
      polygon.save(ba, cl->coor_precision_coef);
    write(ba, inner_polygon_start_idx);
  }
}

bool FlashObject::isEmpty() const
{
  return class_idx < 0 && polygons.isEmpty();
}
