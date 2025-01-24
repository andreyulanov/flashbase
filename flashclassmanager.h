#pragma once

#include "flashclass.h"
#include <QMap>

class FlashClassManager: public QObject
{
  double                main_mip                    = 0;
  double                tile_mip                    = 0;
  int                   default_coor_precision_coef = 1;
  QStringList           save_attributes;
  QMap<QString, QColor> palette;
  QVector<FlashClass>   classes;

protected:
  QString    error_str;
  QString    root_dir;
  FlashClass processJsonObject(const QJsonObject&,
                               QSet<QString>& id_set);

public:
  FlashClassManager(QString root_dir);
  int                        getClassIdxById(QString id) const;
  FlashClass                 getClassById(QString id) const;
  void                       loadClasses(QString path);
  void                       addClass(FlashClass);
  FlashClassImageList        getClassImageList() const;
  const QVector<FlashClass>& getClasses() const;
  const QStringList&         getSaveAttributes() const;

  void   setMainMip(double);
  double getMainMip();

  void   setTileMip(double);
  double getTileMip();

  void   setDefaultCoorPrecisionCoef(double);
  double getDefaultCoorPrecisionCoef();

  QString getErrorStr();
};
