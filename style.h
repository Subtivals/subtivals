#ifndef STYLE_H
#define STYLE_H

#include <QObject>
#include <QString>
#include <QFont>
#include <QColor>
#include <QRect>


class Event;

/*
 * Style to use to show an event.
 */
class Style : public QObject
{
    Q_OBJECT
public:
    /*
     * Constructs a style from an style line of an ASS file.
     */
    explicit Style(const QString &p_line, QObject *p_parent = 0);
    /*
     * Copy constructor
     */
    explicit Style(const Style &s, int marginL = 0, int marginR = 0, int marginV = 0);
    /*
     * Returns the style name.
     */
    const QString &name() const;
    /*
     * Returns the style font.
     */
    const QFont &font() const;
    /*
     * Returns the style primary colou.
     * Others colours from the ASS file are ignored.
     */
    const QColor &primaryColour() const;
    /*
     * Draws an Event with this style within the specified area.
     */
    void drawEvent(QPainter*, const Event&, const QRectF&) const;
private:
    /*
     * Style name.
     */
    QString m_name;
    /*
     * Style font.
     */
    QFont m_font;
    /*
     * Style primary colour.
     */
    QColor m_primaryColour;
    /*
     * Alignment : after the layout of the numpad (1-3 sub, 4-6 mid, 7-9 top)
     */
    Qt::Alignment m_alignment;
    /*
     * MarginL, MarginR : left right margins
     */
    int m_marginL;
    int m_marginR;
    /*
     * MarginV : subtitle : margin from bottom; toptitle : margin from top, midtitle : ignored
     */
    int m_marginV;
};

#endif // STYLE_H
