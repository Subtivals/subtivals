#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QFile>

#include "script.h"

enum SectionType { SECTION_NONE, SECTION_INFOS, SECTION_STYLES, SECTION_EVENTS };

Script::Script(const QString &p_fileName, QObject *p_parent) :
    QObject(p_parent), m_fileName (p_fileName)
{
    SectionType section = SECTION_NONE;

    // Read and process each line of the input file
    QFile file(p_fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    while (!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine());
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
                            m_events.append(new Event(value, this, m_events.size()));
                        }
                    }
                }
            }
        }
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

const QListIterator<Event *> Script::events() const
{
    return QListIterator<Event *>(m_events);
}

const Event *Script::eventAt(int i) const
{
    return m_events[i];
}
