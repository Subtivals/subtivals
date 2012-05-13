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
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include "script.h"

enum SectionType { SECTION_NONE, SECTION_INFOS, SECTION_STYLES, SECTION_EVENTS };

Script::Script(const QString &p_fileName, QObject *p_parent) :
    QObject(p_parent),
    m_fileName (p_fileName)
{
    // Read and process each line of the input file
    QFile file(m_fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QStringList content;
    while (!file.atEnd()) {
        content.append(QString::fromUtf8(file.readLine()));
    }

    QFileInfo fileInfo(p_fileName);
    QString ext = fileInfo.suffix().toLower();
    if (ext == "ass") {
        loadFromAss(content);
    }
    else if (ext == "srt") {
        loadFromSrt(content);
    }
}


const QString &Script::fileName() const
{
    return m_fileName;
}

const QString &Script::title() const
{
    return m_title;
}

Style *Script::style(const QString &p_name) const
{
    // If style is unknown, return first one.
    if (!m_styles.contains(p_name))
        return m_styles[m_styles.keys().at(0)];
    return m_styles[p_name];
}

QList<Style *> Script::styles() const
{
    return m_styles.values();
}

int Script::eventsCount() const
{
    return m_events.size();
}

const QList<Event *> Script::events() const
{
    return QList<Event *>(m_events);
}

const Event *Script::eventAt(int i) const
{
    return m_events[i];
}

const QList<Event *> Script::currentEvents(qlonglong elapsed) const
{
    QList<Event *> l;
    foreach(Event* e, m_events) {
        if (e->match(elapsed)) {
            l.append(e);
        }
    }
    return l;
}

const QList<Event *> Script::nextEvents(qlonglong elapsed) const
{
    // Look for the first event among next ones
    int i=0;
    for(; i<m_events.count(); i++) {
        Event* e = m_events.at(i);
        if (e->msseStart() >= elapsed) {
            break;
        }
    }
    // If not any, return empty
    if (i >= m_events.count()) {
        const QList<Event*> l;
        return l;
    }
    // Return the list of events starting at this time
    qlonglong nextStart = m_events.at(i)->msseStart();
    return currentEvents(nextStart + 1);
}

void Script::correctEventsDuration(bool p_state)
{
    foreach(Event* e, m_events) {
        e->correct(p_state);
    }
}

void Script::loadFromAss(QStringList content)
{
    SectionType section = SECTION_NONE;

    foreach(QString line, content) {
        if (line.contains("[Script Info]")) {
            section = SECTION_INFOS;
        } else if (line.contains("[V4+ Styles]")) {
            section = SECTION_STYLES;
        } else if (line.contains("[Events]")) {
            section = SECTION_EVENTS;
        } else {
            line = line.trimmed().replace("\n", "");
            if (!line.startsWith(";") && !line.isEmpty()) {
                if (section == SECTION_INFOS) {
                    QList<QString> parts = line.split(':');
                    if(parts.size() == 2) {
                        QString key = parts[0].trimmed().toLower();
                        QString value = parts[1].trimmed();
                        if (key == "title") {
                            m_title = value;
                        }
                    }
                } else if (section == SECTION_STYLES) {
                    QList<QString> parts = line.split(':');
                    if(parts.size() == 2) {
                        QString key = parts[0].trimmed().toLower();
                        QString value = parts[1].trimmed();
                        if (key == "style") {
                            Style *style = new Style(value, this);
                            m_styles[style->name()] = style;
                        }
                    }
                } else if (section == SECTION_EVENTS) {
                    QList<QString> parts = line.split(':');
                    if(parts.size() > 1) {
                        QString key = parts[0].trimmed().toLower();
                        QString value = line.mid(key.length() + 1).trimmed();
                        if (key == "dialogue") {

                            QList<QString> subparts = value.split(',');
                            qint64 start = QTime().msecsTo(QTime::fromString(subparts[1], "h:mm:ss.z"));
                            qint64 end = QTime().msecsTo(QTime::fromString(subparts[2], "h:mm:ss.z"));
                            Style *style = this->style(subparts[3].trimmed());
                            int marginL = subparts[5].toInt();
                            int marginR = subparts[6].toInt();
                            int marginV = subparts[7].toInt();

                            int p = value.indexOf(",");
                            for (int i = 0; i < 8; i++) {
                                p = value.indexOf(",", p+1);
                            }
                            QString text = value.mid(p+1);

                            // Transform the hints in the text into HTML:
                            // New ligne HTML-ification
                            text = text.replace("\\N", "<br/>");
                            text = text.replace("\\n", "<br/>");
                            // Italic HTML-ification
                            text = text.replace("{\\i1}", "<i>");
                            text = text.replace("{\\i0}", "</i>");
                            // Color HTML-ification
                            {
                                int idxAccOpenColor = text.indexOf("{\\1c&H");
                                while (idxAccOpenColor != -1) {
                                    int idxAccCloseColor = text.indexOf("}", idxAccOpenColor);
                                    if (idxAccCloseColor != -1) {
                                        QString htmlColor = "<font color=\"#" + text.mid(idxAccOpenColor + 6 + 4, 2) + text.mid(idxAccOpenColor + 6 + 2, 2) + text.mid(idxAccOpenColor + 6, 2) + "\">";
                                        text = text.left(idxAccOpenColor) + htmlColor + text.mid(idxAccCloseColor+1) + "</font>";
                                    }
                                    idxAccOpenColor = text.indexOf("{\\1c&H");
                                }
                            }
                            // Drop others hints that cannot be translated in HTML
                            {
                                int idxAccOpenDrop = text.indexOf("{\\");
                                while (idxAccOpenDrop != -1) {
                                    int idxAccCloseDrop = text.indexOf("}", idxAccOpenDrop);
                                    if (idxAccCloseDrop != -1) {
                                        text = text.left(idxAccOpenDrop) + text.mid(idxAccCloseDrop+1);
                                    }
                                    idxAccOpenDrop = text.indexOf("{\\");
                                }
                            }
                            text = text.trimmed();
                            while(text.startsWith("\n"))
                            {
                                text = text.mid(1);
                            }

                            // Instantiate event !
                            Event *event = new Event(m_events.size(), text, start, end, this);
                            event->setStyle(style);
                            event->setMargins(marginL, marginR, marginV);
                            m_events.append(event);
                        }
                    }
                }
            }
        }
    }
}

void Script::loadFromSrt(QStringList content)
{

}
