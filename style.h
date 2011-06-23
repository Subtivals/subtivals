#ifndef STYLE_H
#define STYLE_H

#include <QObject>
#include <QString>
#include <QFont>
#include <QColor>

class Style : public QObject
{
    Q_OBJECT
public:
    explicit Style(const QString &p_line, QObject *p_parent = 0);
    const QString &name() const;
    const QFont &font() const;
    const QColor &primaryColour() const;
private:
    QString m_name;
    QFont m_font;
    QColor m_primaryColour;
};

#endif // STYLE_H
