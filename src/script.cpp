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
#include <QtCore/QRegExp>
#include <QtCore/QTime>
#include <QtXml/QDomDocument>
#include <QtXml/QDomNodeList>

#include "script.h"

#define DEFAULT_FONT_SIZE 18


enum SectionType { SECTION_NONE, SECTION_INFOS, SECTION_STYLES, SECTION_EVENTS };

bool compareSubtitleStartTime(const Subtitle* s1, const Subtitle* s2)
{
     return s1->msseStart() < s2->msseStart();
}

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

        //Mac OS Classic end line support (CR)
        QStringList macOsLineList = line.split("\r");
        if(macOsLineList.size() > 1){
            content.append(macOsLineList);
        }else{
            line = line.trimmed().replace("\n", "");
            if (!line.startsWith(";")) {
                content.append(line);
            }
        }
    }

    QFileInfo fileInfo(p_fileName);
    QString ext = fileInfo.suffix().toLower();
    if (ext == "ass") {
        m_format = ASS;
        loadFromAss(content);
    }
    else if (ext == "srt") {
        m_format = SRT;
        loadFromSrt(content);
    }
    else if (ext == "txt") {
        m_format = TXT;
        loadFromTxt(content);
    }
    else if (ext == "xml") {
        m_format = XML;
        loadFromXml(content.join(""));
    }
    qSort(m_subtitles.begin(), m_subtitles.end(), compareSubtitleStartTime);
}

Script::ScriptFormat Script::format() const
{
    return m_format;
}

double Script::totalDuration() const
{
    return m_subtitles.last()->msseEnd();
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

QSize Script::resolution() const
{
    return m_resolution;
}

bool Script::hasComments() const
{
    foreach(Subtitle* subtitle, m_subtitles) {
        if (!subtitle->comments().isEmpty())
            return true;
    }
    return false;
}

const QList<Subtitle *> Script::subtitles() const
{
    return QList<Subtitle *>(m_subtitles);
}

const Subtitle *Script::subtitleAt(int i) const
{
    Q_ASSERT(i>=0 && i<m_subtitles.count());
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
    QList<Subtitle*> comments;

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
                    if (key == "playresx") {
                        m_resolution.setWidth(value.toInt());
                    }
                    if (key == "playresy") {
                        m_resolution.setHeight(value.toInt());
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
                        QFont font = QFont(subparts[1]);
                        font.setPixelSize(subparts[2].toInt());
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
                QList<QString> subparts = value.split(',');

                if (key != "comment" && key != "dialogue")
                    continue;

                qint64 start = QTime(0, 0, 0).msecsTo(QTime::fromString(subparts[1], "h:mm:ss.z"));
                qint64 end = QTime(0, 0, 0).msecsTo(QTime::fromString(subparts[2], "h:mm:ss.z"));

                Style *style = this->style(subparts[3].trimmed());
                int marginL = subparts[5].toInt();
                int marginR = subparts[6].toInt();
                int marginV = subparts[7].toInt();

                int p = value.indexOf(",");
                for (int i = 0; i < 8; i++) {
                    p = value.indexOf(",", p+1);
                }
                QString text = value.mid(p+1);
                text = text.trimmed();
                // New-lines : ignore at start
                text = text.replace(QRegExp("^(\\\\[nN])+"), "");

                // Transform the hints in the text into HTML:
                // Italic HTML-ification
                text = text.replace("{\\i1}", "<i>");
                text = text.replace("{\\i0}", "</i>");
                // Bold HTML-ification
                text = text.replace("{\\b1}", "<b>");
                text = text.replace("{\\b0}", "</b>");

                QList<SubtitleLine> lines;
                foreach(QString line, text.split(QRegExp("\\\\[nN]"))) {
                    // Absolute positioning
                    int x = -1;
                    int y = -1;
                    //{\pos(x,y)} or {\pos(x)} in the beginning of the line
                    QRegExp rx("\\{\\\\pos\\((\\d+)(,(\\d+))?\\)\\}");
                    if (rx.indexIn(line) >= 0){
                        QStringList strpos = rx.capturedTexts();
                        x = strpos[1].toInt();
                        if (!strpos[3].isEmpty()) y = strpos[3].toInt();
                    }

                    // Color HTML-ification
                    {
                        int idxAccOpenColor = line.indexOf("{\\1c&H");
                        while (idxAccOpenColor != -1) {
                            int idxAccCloseColor = line.indexOf("}", idxAccOpenColor);
                            if (idxAccCloseColor != -1) {
                                QString htmlColor = "<font color=\"#" + line.mid(idxAccOpenColor + 6 + 4, 2) + line.mid(idxAccOpenColor + 6 + 2, 2) + line.mid(idxAccOpenColor + 6, 2) + "\">";
                                line = line.left(idxAccOpenColor) + htmlColor + line.mid(idxAccCloseColor+1) + "</font>";
                            }
                            idxAccOpenColor = line.indexOf("{\\1c&H");
                        }
                    }

                    // Drop others hints that cannot be translated in HTML
                    lines.append(SubtitleLine(line.replace(QRegExp("\\{.*\\}"), ""),
                                              QPoint(x, y)));
                }

                // Instantiate subtitle !
                Subtitle* subtitle = new Subtitle(m_subtitles.size(), QStringList(), start, end, this, this);

                if (key == "dialogue") {
                    subtitle->setText(lines);
                    subtitle->setStyle(style);
                    subtitle->setMargins(marginL, marginR, marginV);
                    m_subtitles.append(subtitle);
                }
                else if (key == "comment") {
                    subtitle->setComments(text);
                    comments.append(subtitle);
                }
            }
        }
    }

    // Merge comments into subtitles
    foreach(Subtitle* comment, comments) {
        foreach(Subtitle* subtitle, m_subtitles) {
            if (comment->msseStart() == subtitle->msseStart() && 
                comment->msseEnd() == subtitle->msseEnd()) {
                subtitle->setComments(comment->comments());
                break;
            }
        }
    }
}

void Script::loadFromSrt(QStringList content)
{
    SectionType section = SECTION_NONE;

#ifdef WIN32
    QFont font("MS Sans Serif");
#else
    QFont font("Sans");
#endif
    font.setPixelSize(DEFAULT_FONT_SIZE);

    Style *style = new Style(tr("Default"), font, Qt::white, this);
    m_styles[style->name()] = style;

    // Make sure its ends with empty line
    if (!content.last().isEmpty())
        content.append(QString());

    QStringList text;
    QString comments;
    qint64 start = 0;
    qint64 end = 0;
    foreach(QString line, content) {
        if (section == SECTION_NONE && QRegExp("^[0-9]+$").exactMatch(line)) {
            section = SECTION_INFOS;
        }
        else if (section == SECTION_INFOS) {
            QStringList subparts = line.split(QRegExp("\\s+\\-\\->\\s+"));
            start = QTime(0, 0, 0).msecsTo(QTime::fromString(subparts[0], "h:mm:ss,z"));
            end = QTime(0, 0, 0).msecsTo(QTime::fromString(subparts[1], "h:mm:ss,z"));
            section = SECTION_EVENTS;
        }
        else if (section == SECTION_EVENTS) {
            if (line.isEmpty()) {
                // Instantiate subtitle !
                Subtitle *subtitle = new Subtitle(m_subtitles.size(), text, start, end, this, this);
                subtitle->setStyle(style);
                subtitle->setComments(comments);
                m_subtitles.append(subtitle);
                text.clear();

                section = SECTION_NONE;
            }
            else if (line.startsWith("#")) {
                comments = line.replace("#", "");
            }
            else {
                text.append(line);
            }
        }
    }
}

void Script::loadFromTxt(QStringList content)
{
#ifdef WIN32
    QFont font("MS Sans Serif");
#else
    QFont font("Sans");
#endif
    font.setPixelSize(DEFAULT_FONT_SIZE);

    Style *style = new Style(tr("Default"), font, Qt::white, this);
    m_styles[style->name()] = style;

    // Make sure its ends with empty line
    if (!content.last().isEmpty())
        content.append(QString());

    QStringList text;
    QString comments;
    qint64 start = 0;
    qint64 end = 0;
    SectionType section = SECTION_NONE;
    foreach(QString line, content) {
        if (section == SECTION_NONE) {
            section = SECTION_EVENTS;

            QRegExp times("^([0-9:]+) ([0-9:]+)");
            if (times.indexIn(line) >= 0) {
                QStringList subparts = times.capturedTexts();
                start = QTime(0, 0, 0).msecsTo(QTime::fromString(subparts[1], "h:mm:ss:z"));
                end = QTime(0, 0, 0).msecsTo(QTime::fromString(subparts[2], "h:mm:ss:z"));
                continue;
            }
        }
        if (section == SECTION_EVENTS) {
            if (line.isEmpty()) {
                // Instantiate subtitle !
                Subtitle *subtitle = new Subtitle(m_subtitles.size(), text, start, end, this, this);
                subtitle->setStyle(style);
                subtitle->setComments(comments);
                m_subtitles.append(subtitle);
                text.clear();
                comments.clear();

                section = SECTION_NONE;
            }
            else if (line.startsWith("#")) {
                comments = line.replace("#", "");
            }
            else {
                // In TXT format : <word> is equivalent to <i>word</i>
                line = line.replace(QRegExp("<([^i/][^>]+)>"), "<i>\\1</i>");
                // and *word* is equivalent to <b>word</b>
                line = line.replace(QRegExp("\\*([^\\*]+)\\*"), "<b>\\1</b>");
                text.append(line);
            }
        }
    }
}

void Script::loadFromXml(QString content)
{
    QColor defaultColor(Qt::white);
#ifdef WIN32
    QFont defaultFont("MS Sans Serif");
#else
    QFont defaultFont("Sans");
#endif
    defaultFont.setPixelSize(DEFAULT_FONT_SIZE);

    QDomDocument doc;
    doc.setContent(content);

    QString nameSpace;
    // Interop.
    QDomNodeList subtitles = doc.elementsByTagName("Subtitle");
    if (subtitles.length() == 0) {
        // SMTPE.
        nameSpace = "dcst:";
        subtitles = doc.elementsByTagName(nameSpace + "Subtitle");
    }

    QString movieName = tr("Default");
    QDomNodeList movies = doc.elementsByTagName(nameSpace.isEmpty() ? "MovieTitle" : nameSpace + "ContentTitleText");
    if (movies.length() > 0) {
        movieName = movies.at(0).toElement().text();
    }

    QString defaultStyleName(tr("Default"));

    // XXX: Take first font tag.
    QDomNodeList fonts = doc.elementsByTagName(nameSpace + "Font");
    if (fonts.length() > 0) {
        // Font is set, show specific name for style.
        defaultStyleName = movieName;

        QDomNode fontNode = fonts.at(0);
        defaultFont.setFamily(fontNode.toElement().attribute("Id"));
        defaultFont.setItalic((fontNode.toElement().attribute("Italic", "no") != "no"));
        defaultFont.setBold((fontNode.toElement().attribute("Weight", "normal") == "bold"));
        defaultFont.setPixelSize(fontNode.toElement().attribute("Size", "DEFAULT_FONT_SIZE").toInt());
        defaultFont.setUnderline((fontNode.toElement().attribute("Underlined", "no") != "no"));
        defaultColor.setNamedColor(QString("#%1").arg(fontNode.toElement().attribute("Color", "FFFFFFFF")));
    }

    Style *defaultStyle = new Style(defaultStyleName, defaultFont, defaultColor, this);
    m_styles[defaultStyle->name()] = defaultStyle;

    for(int i=0; i<subtitles.length(); i++) {
        QDomNode node = subtitles.at(i);

        QString timeIn = node.toElement().attribute("TimeIn");
        QString timeOut = node.toElement().attribute("TimeOut");
        qint64 start = QTime(0, 0, 0).msecsTo(QTime::fromString(timeIn, "h:mm:ss:z"));
        qint64 end = QTime(0, 0, 0).msecsTo(QTime::fromString(timeOut, "h:mm:ss:z"));

        // Each <Text> becomes a subtitle with its own style (duplicated start/end).
        QDomNodeList textLines = node.childNodes();
        for(int j=0; j<textLines.length(); j++) {
            QDomNode line = textLines.at(j);
            if (line.nodeName().toLower() != (nameSpace + "text")) {
                continue;
            }

            Style *style = defaultStyle;
            QFont font(style->font());
            QString color;

            // Find top level font.
            QDomNodeList fonts = line.toElement().elementsByTagName(nameSpace + "Font");
            if (fonts.length() > 0) {
                QDomElement fontNode = fonts.at(0).toElement();
                if (fontNode.hasAttribute("Color")) {
                    color = "#" + fontNode.attribute("Color");
                }
                if (fontNode.hasAttribute("Id")) {
                    font.setFamily(fontNode.attribute("Id"));
                }
                if (fontNode.hasAttribute("Weight")) {
                    font.setBold((fontNode.attribute("Weight") == "bold"));
                }
                if (fontNode.hasAttribute("Size")) {
                    font.setPixelSize(fontNode.attribute("Size").toInt());
                }
                if (fontNode.hasAttribute("Italic")) {
                    font.setItalic((fontNode.attribute("Italic") != "no"));
                }
                if (fontNode.hasAttribute("Underlined")) {
                    font.setUnderline((fontNode.attribute("Underlined") != "no"));
                }
            }

            // Vertical/horizontal alignment.
            QString hAlign = line.toElement().attribute("HAlign");
            QString vAlign = line.toElement().attribute("VAlign");

            // Offsets from baseline/center.
            QString vPosition = line.toElement().attribute("VPosition");
            QString hPosition = line.toElement().attribute("HPosition");

            // Build new style based on alignment+color.
            bool isOverriden = ((!color.isEmpty()) ||
                                (!hAlign.isEmpty() && hAlign != "center") ||
                                (!vAlign.isEmpty() && vAlign != "center") ||
                                (!vPosition.isEmpty()) ||
                                (!hPosition.isEmpty()));
            if (isOverriden) {
                QStringList tokens;
                tokens << defaultStyle->name()  << hAlign << vAlign << vPosition << hPosition << color;
                QString styleName = tokens.join("-");
                if (!m_styles.contains(styleName)) {
                    // Not yet encountered. Built it!
                    Style *newStyle = new Style(*style, font);
                    newStyle->setName(styleName);

                    if (!color.isEmpty()) {
                        newStyle->setPrimaryColour(QColor(color));
                    }

                    Qt::Alignment alignment = Qt::AlignVCenter;
                    if (hAlign.toLower() == "left")
                        alignment = Qt::AlignLeft;
                    if (hAlign.toLower() == "right")
                        alignment = Qt::AlignRight;
                    if (vAlign.toLower() == "bottom")
                        alignment |= Qt::AlignBottom;
                    if (vAlign.toLower() == "top")
                        alignment |= Qt::AlignTop;

                    // XXX: should inherit.
                    newStyle->setAlignment(alignment);

                    // XXX: should inherit.
                    newStyle->setOffsets(hPosition.toDouble()/100.0,
                                         vPosition.toDouble()/100.0);

                    m_styles[styleName] = newStyle;
                }
                style = m_styles[styleName];
            }

            Subtitle *subtitle = new Subtitle(m_subtitles.size(), QStringList(line.toElement().text()), start, end, this, this);
            subtitle->setStyle(style);
            m_subtitles.append(subtitle);
        }

    }
}

const QString Script::exportList(Script::ScriptFormat p_format) const
{
    QString output;

    if (p_format == Script::CSV) {
        QString dateFormat("hh:mm:ss");

        // CSV Headers
        QStringList headers;
        headers << tr("Row") << tr("Start") << tr("End") << tr("Style") << tr("Text") << tr("Comments");
        output.append(QString("\"%1\"\n").arg(headers.join("\" ; \"")));

        // Group subtitles by display order.
        int rowIndex = 1;
        QList<Subtitle*> next = nextSubtitles(0);
        while(next.size() > 0) {
            QStringList row;
            row << QString("%1").arg(rowIndex);
            row << QTime(0, 0, 0).addMSecs(next.first()->msseStart()).toString(dateFormat);
            row << QTime(0, 0, 0).addMSecs(next.last()->msseEnd()).toString(dateFormat);

            QStringList styles;
            QStringList texts;
            QStringList comments;
            foreach(Subtitle* subtitle, next) {
                styles << subtitle->style()->name();
                texts << QString(subtitle->prettyText()).replace("\"", "");
                if (!subtitle->comments().isEmpty())
                    comments << QString(subtitle->comments()).replace("\n", "");
            }
            styles.removeDuplicates();
            row << styles.join(" # ");
            row << texts.join(" # ");
            row << comments.join(" # ");

            output.append(QString("\"%1\"\n").arg(row.join("\" ; \"")));

            next = nextSubtitles(next.last()->msseStart () + 1);
            rowIndex++;
        }
    }
    return output;
}
