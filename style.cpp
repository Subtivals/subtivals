#include "style.h"
#include <QDebug>

Style::Style(const QString &p_line, QObject *p_parent) :
    QObject(p_parent)
{
    QList<QString> subparts = p_line.split(',');
    m_name = subparts[0];
    qDebug() << "New style=" << m_name;
    m_font = QFont(subparts[1], subparts[2].toInt());
    QString c = subparts[3].right(6);
    m_primaryColour.setRed(c.mid(0, 2).toInt(0, 16));
    m_primaryColour.setGreen(c.mid(2, 2).toInt(0, 16));
    m_primaryColour.setBlue(c.mid(4, 2).toInt(0, 16));
}

const QString &Style::name() const {
    return m_name;
}

const QFont &Style::font() const {
    return m_font;
}

const QColor &Style::primaryColour() const {
    return m_primaryColour;
}

