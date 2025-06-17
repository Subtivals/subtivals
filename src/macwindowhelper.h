// macwindowhelper.h
#pragma once

#include <QWidget>

#if defined(Q_OS_MAC)
void makeWindowCoverMenuBar(QWidget *widget);
#else
inline void makeWindowCoverMenuBar(QWidget *) {}
#endif