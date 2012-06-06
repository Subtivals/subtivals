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
#include "subtitle.h"

/*
 * Scipts from a subtitle file : union of subtitles and styles.
 */
class Script : public QObject
{
    Q_OBJECT
public:
    /*
     * Constructs a script from an subtitle file.
     */
    explicit Script(const QString &p_fileName, QObject *parent = 0);
    /*
     * Returns the name of the subtitle file used to create the script.
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
     * Return the number of subtitles in the script.
     */
    int subtitlesCount() const;
    /*
     * Returns an iterator on the subtitles of the script.
     */
    const QList<Subtitle *> subtitles() const;
    /*
     * Returns the subtitle at the given index.
     */
    const Subtitle * subtitleAt(int i) const;
    /*
     * Returns the subtitles matching the specified elapsed time.
     */
    const QList<Subtitle *> currentSubtitles(qlonglong elapsed) const;
    /*
     * Returns the list of the next subtitles at this elapsed time.
     */
    const QList<Subtitle *> nextSubtitles(qlonglong elapsed) const;
    const QList<Subtitle *> previousSubtitles(qlonglong elapsed) const;
public slots:
    /*
     * Activates duration correction of subtitles.
     */
    void correctSubtitlesDuration(bool p_state);
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
     * Script subtitles list.
     */
    QList<Subtitle *> m_subtitles;
};

#endif // SCRIPT_H
