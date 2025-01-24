#include "flashdatetime.h"

#include <QStringList>
#include <math.h>

FlashDateTime::FlashDateTime()
{
  year  = 0;
  month = 0;
  day   = 0;
  hour  = 0;
  min   = 0;
  sec   = 0;
  tz    = 0;
}

static QDateTime toQDateTime(FlashDateTime dt)
{
  QDateTime qdt(QDate(dt.getYear(), dt.getMonth(), dt.getDay()),
                QTime(dt.getHour(), dt.getMin(), dt.getSec()));
  qdt.setOffsetFromUtc(0);
  return qdt;
}

static FlashDateTime fromQDateTime(QDateTime dt, double time_zone)
{
  return FlashDateTime(dt.date().year(), dt.date().month(),
                       dt.date().day(), dt.time().hour(),
                       dt.time().minute(), dt.time().second(),
                       time_zone);
}

FlashDateTime::FlashDateTime(int _year, int _month, int _day,
                             int _hour, int _min, int _sec,
                             double _tz)
{
  year  = _year;
  month = _month;
  day   = _day;
  hour  = _hour;
  min   = _min;
  sec   = _sec;
  setTimeZone(_tz);
}

FlashDateTime::FlashDateTime(QDateTime qdt)
{
  year  = qdt.date().year();
  month = qdt.date().month();
  day   = qdt.date().day();
  hour  = qdt.time().hour();
  min   = qdt.time().minute();
  sec   = qdt.time().second();
  setTimeZone(1.0 * qdt.offsetFromUtc() / 3600);
}

int FlashDateTime::getYear() const
{
  return year;
}

int FlashDateTime::getMonth() const
{
  return month;
}

int FlashDateTime::getDay() const
{
  return day;
}

int FlashDateTime::getHour() const
{
  return hour;
}

int FlashDateTime::getMin() const
{
  return min;
}

int FlashDateTime::getSec() const
{
  return sec;
}

double FlashDateTime::getTimeZone() const
{
  return (1.0 * int(tz)) / time_zone_coef;
}

void FlashDateTime::setTimeZone(double v)
{
  tz = int(v * time_zone_coef);
}

bool FlashDateTime::isEqual(const FlashDateTime& v) const
{
  return year == v.year && month == v.month && day == v.day &&
         hour == v.hour && min == v.min && sec == v.sec && tz == v.tz;
}

QString FlashDateTime::toString(QString format) const
{
  if (format.isEmpty())
  {
    QString str = toQDateTime(*this).toString("yyyy-MM-ddThh:mm:ss");
    return str += QString().asprintf("Z%02.1f", getTimeZone());
  }
  else
    return toQDateTime(*this).toString(format);
}

FlashDateTime FlashDateTime::fromString(QString str)
{
  FlashDateTime dt;
  dt.year  = str.mid(0, 4).toDouble();
  dt.month = str.mid(5, 2).toDouble();
  dt.day   = str.mid(8, 2).toDouble();
  dt.hour  = str.mid(11, 2).toDouble();
  dt.min   = str.mid(14, 2).toDouble();
  dt.sec   = str.mid(17, 2).toDouble();
  dt.setTimeZone(str.mid(20, 4).toDouble());
  return dt;
}

bool FlashDateTime::isValid() const
{
  return (bool)year;
}

int FlashDateTime::secsTo(FlashDateTime v) const
{
  QDateTime dt1 = toQDateTime(*this);
  dt1           = dt1.addSecs(-getTimeZone() * 3600);
  QDateTime dt2 = toQDateTime(v);
  dt2           = dt2.addSecs(-v.getTimeZone() * 3600);
  return dt1.secsTo(dt2);
}

int FlashDateTime::secsToWithoutTZ(FlashDateTime v) const
{
  QDateTime dt1 = toQDateTime(*this);
  QDateTime dt2 = toQDateTime(v);
  return dt1.secsTo(dt2);
}

FlashDateTime FlashDateTime::addDays(int days)
{
  QDateTime dt1 = toQDateTime(*this);
  QDateTime dt2 = dt1.addDays(days);
  return fromQDateTime(dt2, getTimeZone());
}

FlashDateTime FlashDateTime::addSecs(int secs)
{
  QDateTime dt1 = toQDateTime(*this);
  QDateTime dt2 = dt1.addSecs(secs);
  return fromQDateTime(dt2, getTimeZone());
}

QTime FlashDateTime::str2time(QString str)
{
  QStringList str_list = str.split(':');
  if (str_list.count() < 3)
    return QTime();
  int hms[3];
  for (int i = 0; i < 3; i++)
    hms[i] = str_list[i].toInt();
  QTime t(hms[0], hms[1], hms[2]);
  return t;
}

QDate FlashDateTime::str2date(QString str)
{
  QStringList str_list = str.split('.');
  if (str_list.count() < 3)
    return QDate();
  int ymd[3];
  for (int i = 0; i < 3; i++)
    ymd[i] = str_list[i].toInt();
  QDate d(ymd[2], ymd[1], ymd[0]);
  return d;
}

int FlashDateTime::rus2sec(QString str)
{
  QStringList str_list = str.split(' ');
  if (str_list.count() == 2)
  {
    int     value = str_list.at(0).toInt();
    QString units = str_list.at(1).toLower();
    if (units == "мин")
      value *= 60;
    if (units == "час" || units == "часа" || units == "часов")
      value *= 3600;
    return value;
  }
  else
    return 0;
}

QString FlashDateTime::sec2str(int t)
{
  int h = t / 3600;
  int m = (t - h * 3600) / 60;
  int s = t - h * 3600 - m * 60;
  if (h > 99)
    return QString().asprintf("--:--:--");
  else
    return QString().asprintf("%02d:%02d:%02d", h, m, s);
}

QString FlashDateTime::timezone2str(double t)
{
  int     h = (int)t;
  QString str;

  if (t > 0)
    str = "+";
  else if (t < 0)
    str = "-";
  str += QString().asprintf("%02d:%02d", abs(h),
                            (int)(fabs(t - h) * 60));
  return str;
}

double FlashDateTime::str2timezone(QString str)
{
  double sign = 1;
  if (str.at(0) == '-')
  {
    sign = -1;
    str.remove(0, 1);
  }
  QStringList str_list = str.split(':');

  if (str_list.count() == 2)
    return sign * (str_list.at(0).toInt() +
                   1.0 * str_list.at(1).toInt() / 60);
  else
    return 0;
}
