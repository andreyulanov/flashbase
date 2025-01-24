#pragma once

#include <QDateTime>

class FlashDateTime
{
  static constexpr int time_zone_coef = 4;

  short       year;
  signed char month;
  signed char day;
  signed char hour;
  signed char min;
  signed char sec;
  signed char tz;

public:
  FlashDateTime();
  FlashDateTime(int year, int month, int day, int hour, int min, int sec,
            double tz);
  explicit FlashDateTime(QDateTime);
  int              getYear() const;
  int              getMonth() const;
  int              getDay() const;
  int              getHour() const;
  int              getMin() const;
  int              getSec() const;
  double           getTimeZone() const;
  void             setTimeZone(double);
  bool             isEqual(const FlashDateTime&) const;
  QString          toString(QString format = QString()) const;
  static FlashDateTime fromString(QString);
  bool             isValid() const;
  int              secsTo(FlashDateTime) const;
  int              secsToWithoutTZ(FlashDateTime) const;
  FlashDateTime        addDays(int);
  FlashDateTime        addSecs(int secs);
  static QTime     str2time(QString);
  static QDate     str2date(QString);
  static int       rus2sec(QString);
  static QString   sec2str(int t);
  static QString   timezone2str(double t);
  static double    str2timezone(QString str);
};
