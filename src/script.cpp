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
        QString line = QString::fromUtf8(file.readLine());
        line = line.trimmed().replace("\n", "");
        if (!line.startsWith(";")) {
            content.append(line);
        }
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

int Script::subtitlesCount() const
{
    return m_subtitles.size();
}

const QList<Subtitle *> Script::subtitles() const
{
    return QList<Subtitle *>(m_subtitles);
}

const Subtitle *Script::subtitleAt(int i) const
{
    return m_subtitles[i];
}

const QList<Subtitle *> Script::currentSubtitles(qlonglong elapsed) const
{
    QList<Subtitle *> l;
    foreach(Subtitle* e, m_subtitles) {
        if (e->match(elapsed)) {
            l.append(e);
        }
    }
    return l;
}

const QList<Subtitle *> Script::nextSubtitles(qlonglong elapsed) const
{
    // Look for the first subtitle among next ones
    int i=0;
    for(; i<m_subtitles.count(); i++) {
        Subtitle* e = m_subtitles.at(i);
        if (e->msseStart() >= elapsed) {
            break;
        }
    }
    // If not any, return empty
    if (i >= m_subtitles.count()) {
        const QList<Subtitle*> l;
        return l;
    }
    // Return the list of subtitles starting at this time
    qlonglong nextStart = m_subtitles.at(i)->msseStart();
    return currentSubtitles(nextStart + 1);
}

const QList<Subtitle *> Script::previousSubtitles(qlonglong elapsed) const
{
    // Look for the last subtitle among previous ones
    int i = m_subtitles.count()-1;
    for(; i>=0; i--) {
        Subtitle* e = m_subtitles.at(i);
        if (e->msseEnd() < elapsed) {
            break;
        }
    }
    // If not any, return empty
    if (i < 0) {
        const QList<Subtitle*> l;
        return l;
    }
    // Return the list of subtitles starting at this time
    qlonglong previousEnd = m_subtitles.at(i)->msseEnd();
    return currentSubtitles(previousEnd - 1);
}

void Script::correctSubtitlesDuration(bool p_state)
{
    foreach(Subtitle* e, m_subtitles) {
        e->correct(p_state);
    }
}

void Script::loadFromAss(QStringList content)
{
    SectionType section = SECTION_NONE;

    foreach(QString line, content) {
        if (line.isEmpty())
            continue;
        if (line.contains("[Script Info]")) {
            section = SECTION_INFOS;
        } else if (line.contains("[V4+ Styles]")) {
            section = SECTION_STYLES;
        } else if (line.contains("[Events]")) {
            section = SECTION_EVENTS;
        } else {
             if (section == SECTION_INFOS) {
                QList<QString> parts = line.split(':');
                if(parts.size() == 2) {
                    QString key = parts[0].trimmed().toLower();
                    QString value = parts[1].trimmed();
                    if (key == "title") {
                        m_title = value;
                    }
                }
            }
            else if (section == SECTION_STYLES) {
                QList<QString> parts = line.split(':');
                if(parts.size() == 2) {
                    QString key = parts[0].trimmed().toLower();
                    QString value = parts[1].trimmed();
                    if (key == "style") {

                        QList<QString> subparts = value.split(',');
                        QString name = subparts[0];
                        QFont font = QFont(subparts[1], subparts[2].toInt());
                        QString c = subparts[3].right(6);
                        QColor color;
                        color.setBlue(c.mid(0, 2).toInt(0, 16));
                        color.setGreen(c.mid(2, 2).toInt(0, 16));
                        color.setRed(c.mid(4, 2).toInt(0, 16));
                        int marginL = subparts[19].toInt();
                        int marginR = subparts[20].toInt();
                        int marginV = subparts[21].toInt();

                        // Alignment after the layout of the numpad (1-3 sub, 4-6 mid, 7-9 top)
                        Qt::Alignment position = Qt::AlignVCenter;
                        int alignment = subparts[18].right(1).toInt();
                        if (alignment <= 3)
                            position = Qt::AlignBottom;
                        if (alignment >= 7)
                            position = Qt::AlignTop;
                        if (alignment % 3 == 0)
                            position |= Qt::AlignRight;
                        if (alignment % 3 == 1)
                            position |= Qt::AlignLeft;
                        if (alignment % 3 == 2)
                            position |= Qt::AlignHCenter;

                        Style *style = new Style(name, font, color, this);
                        style->setAlignment(position);
                        style->setMargins(marginL, marginR, marginV);
                        m_styles[style->name()] = style;
                    }
                }
            }
            else if (section == SECTION_EVENTS) {
                QList<QString> parts = line.split(':');
                if(parts.size() <= 1)
                    continue;
                QString key = parts[0].trimmed().toLower();
                QString value = line.mid(key.length() + 1).trimmed();
                if (key != "dialogue")
                    continue;

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
                // Bold HTML-ification
                text = text.replace("{\\b1}", "<b>");
                text = text.replace("{\\b0}", "</b>");
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
                while(text.startsWith("\n")) {
                    text = text.mid(1);
                }

                // Instantiate subtitle !
                Subtitle *subtitle = new Subtitle(m_subtitles.size(), text, start, end, this, this);
                subtitle->setStyle(style);
                subtitle->setMargins(marginL, marginR, marginV);
                m_subtitles.append(subtitle);
            }
        }
    }
}

void Script::loadFromSrt(QStringList content)
{
    SectionType section = SECTION_NONE;

    Style *style = new Style(tr("Default"), QFont("Sans", 18), Qt::white, this);
    m_styles[style->name()] = style;

    QStringList text;
    foreach(QString line, content) {
        qint64 start = 0;
        qint64 end = 0;
        if (section == SECTION_NONE && QRegExp("^[0-9]+$").exactMatch(line)) {
            section = SECTION_INFOS;
        }
        else if (section == SECTION_INFOS) {
            QStringList subparts = line.split(" --> ");
            start = QTime().msecsTo(QTime::fromString(subparts[0], "h:mm:ss.z"));
            end = QTime().msecsTo(QTime::fromString(subparts[1], "h:mm:ss.z"));
            section = SECTION_EVENTS;
        }
        else if (section == SECTION_EVENTS) {
            if (line.isEmpty()) {
                // Instantiate subtitle !
                Subtitle *subtitle = new Subtitle(m_subtitles.size(), text.join("<br/>"), start, end, this, this);
                subtitle->setStyle(style);
                m_subtitles.append(subtitle);
                text.clear();

                section = SECTION_NONE;
            }
            else {
                text.append(line);
            }
        }
    }
}
