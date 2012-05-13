/**
  *  This file is part of Subtivals.
  *
  *  Subtivals is free software: you can redistribute it and/or modify
  *  it under the terms of the GNU General Public License as published by
  *  the Free Software Foundation, either version 3 of the License, or
  *  (at your option) any later version.
  *
  *  Subtivals is distributed in the hope that it will be useful,
  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  *  GNU General Public License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with Subtivals.  If not, see <http://www.gnu.org/licenses/>
  **/
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
    Style *style(const QString &p_name) const;
    /*
     * Returns the list of styles
     */
    QList<Style*> styles() const;
    /*
     * Return the number of events in the script.
     */
    int eventsCount() const;
    /*
     * Returns an iterator on the events of the script.
     */
    const QList<Event *> events() const;
    /*
     * Returns the event at the given index.
     */
    const Event * eventAt(int i) const;
    /*
     * Returns the events matching the specified elapsed time.
     */
    const QList<Event *> currentEvents(qlonglong elapsed) const;
    /*
     * Returns the list of the next events at this elapsed time.
     */
    const QList<Event *> nextEvents(qlonglong elapsed) const;
public slots:
    /*
     * Activates duration correction of events.
     */
    void correctEventsDuration(bool p_state);
protected:
    void loadFromSrt(QStringList content);
    void loadFromAss(QStringList content);
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
