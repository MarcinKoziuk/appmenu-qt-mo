/* This file is part of the appmenu-qt project
   Copyright 2011 Canonical
   Author: Aurelien Gateau <aurelien.gateau@canonical.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License (LGPL) as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later
   version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef QX11MENUBARIMPL_X11_H
#define QX11MENUBARIMPL_X11_H

#include <QObject>


#include <private/qabstractmenubarimpl_p.h>

QT_BEGIN_NAMESPACE

class QMenuBar;
class QX11MenuBarAdapter;
class QDBusServiceWatcher;

class QX11MenuBarImpl : public QObject, public QAbstractMenuBarImpl
{
    Q_OBJECT
public:
    ~QX11MenuBarImpl();

    virtual void init(QMenuBar *);

    virtual bool allowSetVisible() const;

    virtual void actionEvent(QActionEvent*);

    virtual void handleReparent(QWidget *oldParent, QWidget *newParent, QWidget *oldWindow, QWidget *newWindow);

    virtual bool allowCornerWidgets() const;

    virtual void popupAction(QAction*);

    virtual void setNativeMenuBar(bool);
    virtual bool isNativeMenuBar() const;

    virtual bool shortcutsHandledByNativeMenuBar() const;
    virtual bool menuBarEventFilter(QObject *, QEvent *event);

private:
    QMenuBar *menuBar;
    QX11MenuBarAdapter *adapter;
    int nativeMenuBar : 3;  // Only has values -1, 0, and 1

    void createMenuBar();
    void destroyMenuBar();
    QDBusServiceWatcher *menuBarServiceWatcher;
    QString objectPath;

private Q_SLOTS:
    void slotMenuBarServiceChanged(const QString &, const QString &, const QString &);
    void checkMenuBar();
    void registerWindow();
};

class Q_GUI_EXPORT QX11MenuBarImplFactory : public QObject, public QMenuBarImplFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QMenuBarImplFactoryInterface:QFactoryInterface)
public:
    virtual QAbstractMenuBarImpl* createImpl();
    QStringList keys() const { return QStringList() << QLatin1String("default"); }
};

QT_END_NAMESPACE

#endif // QX11MENUBARIMPL_X11_H
