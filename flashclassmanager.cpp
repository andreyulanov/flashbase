#include "flashclassmanager.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaEnum>
#include <QMapIterator>

FlashClass
FlashClassManager::processJsonObject(const QJsonObject& obj,
                                     QSet<QString>&     id_set)
{
  FlashClass cl;
  if (!obj.contains("id"))
  {
    auto _main_mip = obj.value("main_mip").toDouble();
    if (_main_mip > 0)
      main_mip = _main_mip;

    auto _tile_mip = obj.value("tile_mip").toDouble();
    if (_tile_mip > 0)
      tile_mip = _tile_mip;

    auto _coor_precision_coef =
        obj.value("default_coor_precision_coef").toInt();
    if (_coor_precision_coef > 0)
      default_coor_precision_coef = _coor_precision_coef;

    auto _save_attributes = obj.value("save_attributes").toArray();
    if (!_save_attributes.isEmpty())
    {
      auto list = _save_attributes.toVariantList();
      for (auto v: list)
        save_attributes.append(v.toString());
    }

    auto keys = obj.keys();
    for (auto key: keys)
    {
      if (key.startsWith("color"))
      {
        auto   l = obj.value(key).toArray().toVariantList();
        QColor c;
        if (l.count() >= 3)
          c = QColor{l.at(0).toInt(), l.at(1).toInt(),
                     l.at(2).toInt()};
        if (l.count() == 4)
          c.setAlpha(l.at(3).toInt());
        else
          c.setAlpha(255);
        auto color_name = key.remove("color").simplified();
        palette.insert(color_name, c);
      }
    }

    auto palette_val = obj.value("palette").toString();
    if (!palette_val.isEmpty())
    {
      auto color_str_list = palette_val.split(";");
      for (auto color_str: color_str_list)
      {
        auto c = color_str.split(" ");
        if (c.count() == 4)
          palette.insert(c.first(),
                         QColor(c.at(1).toInt(), c.at(2).toInt(),
                                c.at(3).toInt()));
      }
    }
    return cl;
  }

  if (obj.contains("base_id"))
  {
    auto base_id = obj.value("base_id").toString();
    if (!base_id.isEmpty())
    {
      auto base_class = getClassById(base_id);
      if (base_class.id.isEmpty())
        qDebug() << "error: base class" << base_id << "not defined";
      else
        cl = base_class;
    }
  }

  cl.id = obj.value("id").toString();
  if (id_set.contains(cl.id))
  {
    error_str = cl.id + " has been already defined!";
    return cl;
  }
  id_set.insert(cl.id);

  if (obj.contains("type"))
  {
    auto type_str = obj.value("type").toString();
    if (!type_str.isEmpty())
    {
      bool ok = false;
      cl.type = static_cast<FlashClass::Type>(
          QMetaEnum::fromType<FlashClass::Type>().keyToValue(
              type_str.toUtf8(), &ok));
      if (!ok)
      {
        error_str = type_str + " is an unrecognized type!";
        return cl;
      }
    }
  }

  if (obj.contains("style"))
  {
    cl.style       = FlashClass::Solid;
    auto style_str = obj.value("style").toString();
    if (!style_str.isEmpty())
    {
      bool ok  = false;
      cl.style = static_cast<FlashClass::Style>(
          QMetaEnum::fromType<FlashClass::Style>().keyToValue(
              style_str.toUtf8(), &ok));
      if (!ok)
      {
        error_str = style_str + " is an unrecognized style!";
        return cl;
      }
    }
  }

  if (obj.contains("layer"))
    cl.layer = obj.value("layer").toInt();

  if (obj.contains("width_mm"))
    cl.width_mm = obj.value("width_mm").toDouble();

  for (int color_idx = 0; color_idx < 3; color_idx++)
  {
    QString color_id = "pen";
    QColor* c        = &cl.pen;
    if (color_idx == 1)
    {
      color_id = "brush";
      c        = &cl.brush;
    }
    if (color_idx == 2)
    {
      color_id = "text";
      c        = &cl.text;
    }

    if (obj.contains(color_id))
    {
      if (obj.value(color_id).isArray())
      {
        auto l = obj.value(color_id).toArray().toVariantList();
        if (l.count() >= 3)
          *c = QColor{l.at(0).toInt(), l.at(1).toInt(),
                      l.at(2).toInt()};
        if (l.count() == 4)
          c->setAlpha(l.at(3).toInt());
        else
          c->setAlpha(255);
      }
      else
      {
        auto color_name = obj.value(color_id).toString();
        if (palette.contains(color_name))
          *c = palette.value(color_name);
        else
          qDebug() << "error: undefined color name:" << color_name;
      }
    }
  }

  if (obj.contains("min_mip"))
    cl.min_mip = obj.value("min_mip").toDouble();

  if (obj.contains("max_mip"))
    cl.max_mip = obj.value("max_mip").toDouble();

  if (obj.contains("image"))
  {
    auto image_name = obj.value("image").toString();
    if (!image_name.isEmpty())
    {
      auto image_path = root_dir + "/images/" + image_name;
      cl.image        = QImage(image_path);
      if (cl.image.isNull())
        qDebug() << "error opening" << image_path;
    }
  }

  if (obj.contains("coor_precision_coef"))
    cl.coor_precision_coef = obj.value("coor_precision_coef").toInt();

  if (cl.coor_precision_coef == 0 && !obj.contains("base_id"))
    cl.coor_precision_coef = default_coor_precision_coef;

  if (obj.contains("detect_tags"))
  {
    auto detect_tags_value = obj.value("detect_tags");
    if (detect_tags_value.isObject())
    {
      auto obj = detect_tags_value.toObject();
      auto vm  = obj.toVariantMap();
      for (auto i = vm.begin(); i != vm.end(); i++)
        cl.detect_tags.insert(i.key(), i.value().toString());
    }
  }

  if (obj.contains("detect_tags"))
  {
    auto attributes = obj.value("attributes");
    if (attributes.isObject())
    {
      auto obj = attributes.toObject();
      auto vm  = obj.toVariantMap();
      for (auto i = vm.begin(); i != vm.end(); i++)
        cl.attributes.insert(i.key(), i.value().toString());
    }
  }

  return cl;
}

void FlashClassManager::addClass(FlashClass cl)
{
  if (getClassById(cl.id).id.isEmpty())
    classes.append(cl);
  else
    qDebug() << "error: class" << cl.id << "already defined!";
}

void FlashClassManager::loadClasses(QString name)
{
  QFile         f(root_dir + "/" + name);
  QSet<QString> id_set;
  if (f.open(QIODevice::ReadOnly))
  {
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(f.readAll(), &error);
    f.close();
    if (doc.isNull())
    {
      qDebug() << "error: could not parse" << name;
      return;
    }

    if (doc.isArray())
    {
      auto ar = doc.array();
      for (auto json_val: ar)
      {
        auto obj = json_val.toObject();
        if (obj.isEmpty())
          continue;
        auto cl = processJsonObject(obj, id_set);
        if (!cl.id.isEmpty())
          classes.append(cl);
      }
    }
  }
}

FlashClassManager::FlashClassManager(QString _root_dir)
{
  root_dir = _root_dir;
}

int FlashClassManager::getClassIdxById(QString id) const
{
  for (int i = -1; auto cl: classes)
  {
    i++;
    if (cl.id == id)
      return i;
  }
  return -1;
}

FlashClass FlashClassManager::getClassById(QString id) const
{
  auto idx = getClassIdxById(id);
  if (idx >= 0)
    return classes.at(idx);
  else
    return FlashClass();
}

const QVector<FlashClass>& FlashClassManager::getClasses() const
{
  return classes;
}

const QStringList& FlashClassManager::getSaveAttributes() const
{
  return save_attributes;
}

FlashClassImageList FlashClassManager::getClassImageList() const
{
  QVector<FlashClassImage> ret;
  for (auto cl: classes)
    ret.append({cl.id, cl.image});
  return ret;
}

void FlashClassManager::setMainMip(double v)
{
  main_mip = v;
}

double FlashClassManager::getMainMip()
{
  return main_mip;
}

void FlashClassManager::setTileMip(double v)
{
  tile_mip = v;
}

double FlashClassManager::getTileMip()
{
  return tile_mip;
}

void FlashClassManager::setDefaultCoorPrecisionCoef(double v)
{
  default_coor_precision_coef = v;
}

double FlashClassManager::getDefaultCoorPrecisionCoef()
{
  return default_coor_precision_coef;
}

QString FlashClassManager::getErrorStr()
{
  return error_str;
}
