#include "style.h"

#include <event.h>
#include <QPainter>

Style::Style(const QString &p_line, QObject *p_parent) :
    QObject(p_parent),
    m_alignment(Qt::AlignVCenter),
    m_marginL(0),
    m_marginR(0),
    m_marginV(0)
{
    QList<QString> subparts = p_line.split(',');
    m_name = subparts[0];
    m_font = QFont(subparts[1], subparts[2].toInt());
    QString c = subparts[3].right(6);
    m_primaryColour.setBlue(c.mid(0, 2).toInt(0, 16));
    m_primaryColour.setGreen(c.mid(2, 2).toInt(0, 16));
    m_primaryColour.setRed(c.mid(4, 2).toInt(0, 16));
    m_marginL = subparts[19].toInt();
    m_marginR = subparts[20].toInt();
    m_marginV = subparts[21].toInt();

    // Alignment after the layout of the numpad (1-3 sub, 4-6 mid, 7-9 top)
    int alignment = subparts[18].right(1).toInt();
    if (alignment <= 3)
        m_alignment = Qt::AlignBottom;
    if (alignment >= 7)
        m_alignment = Qt::AlignTop;
    if (alignment % 3 == 0)
        m_alignment |= Qt::AlignRight;
    if (alignment % 3 == 1)
        m_alignment |= Qt::AlignLeft;
    if (alignment % 3 == 2)
        m_alignment |= Qt::AlignHCenter;
}

Style::Style(const Style &s, int marginL, int marginR, int marginV) {
    // Explicit clone : Shallow copy could have been enough
    this->m_name = s.m_name;
    this->m_font = s.m_font;
    this->m_primaryColour = s.m_primaryColour;
    this->m_alignment = s.m_alignment;
    this->m_marginL = s.m_marginL;
    if (marginL) this->m_marginL = marginL;
    this->m_marginR = s.m_marginR;
    if (marginR) this->m_marginR = marginR;
    this->m_marginV = s.m_marginV;
    if (marginV) this->m_marginV = marginV;
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

void Style::drawEvent(QPainter *painter, const Event &event, const QRectF &area) const
{
    qreal top_margin = 0;
    qreal sub_margin = 0;
    if (m_alignment & Qt::AlignBottom)
        sub_margin = m_marginV;
    if (m_alignment & Qt::AlignTop)
        top_margin = m_marginV;
    QRectF placement = area.adjusted(m_marginL,
                                     top_margin,
                                     -m_marginR,
                                     -sub_margin);
    painter->setFont(m_font);
    painter->setPen(m_primaryColour);
    painter->drawText(placement, event.text(), QTextOption(m_alignment));
}
