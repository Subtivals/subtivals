#ifndef SCRIPT_H
#define SCRIPT_H

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>

#include "style.h"
#include "event.h"

/*
 * Scipts from an ASS file : union of events and styles.
 */
class Script : public QObject
{
    Q_OBJECT
public:
    /*
     * Constructs a script from an ASS file.
     */
    explicit Script(const QString &p_fileName, QObject *parent = 0);
    /*
     * Returns the name of the ASS file used to create the script.
     */
    const QString &fileName() const;
    /*
     * Returns the script title.
     */
    const QString &title() const;
    /*
     * Returns a style from its name.
     */
    const Style *style(const QString &p_name) const;
    /*
     * Return the number of events in the script.
     */
    int eventsCount() const;
    /*
     * Returns an iterator on the events of the script.
     */
    const QListIterator<Event *> events() const;
private:
    /*
     * Script ASS file name.
     */
    QString m_fileName;
    /*
     * Script title.
     */
    QString m_title;
    /*
     * Script styles map.
     */
    QMap<QString, Style *> m_styles;
    /*
     * Script events list.
     */
    QList<Event *> m_events;
};

#endif // SCRIPT_H
