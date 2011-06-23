#ifndef SCRIPT_H
#define SCRIPT_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>

#include "style.h"
#include "event.h"

class Script : public QObject
{
    Q_OBJECT
public:
    explicit Script(const QString &p_fileName, QObject *parent = 0);
    const QString &fileName() const;
    const QString &title() const;
    const Style *style(const QString &p_name) const;
    int eventsCount() const;
    const QListIterator<Event *> events() const;
signals:
public slots:
private:
    QString m_fileName;
    QString m_title;
    QMap<QString, Style *> m_styles;
    QList<Event *> m_events;
};

#endif // SCRIPT_H
