#ifndef STYLE_H
#define STYLE_H

#include <QObject>
#include <QString>
#include <QFont>
#include <QColor>

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
};

#endif // STYLE_H
