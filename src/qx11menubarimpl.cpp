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
#include "qx11menubarimpl.h"

// dbusmenu-qt
#include <dbusmenuexporter.h>

// Qt
#include <QActionEvent>
#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QMenuBar>
#include <QDBusInterface>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusServiceWatcher>
#include <QTimer>

QT_BEGIN_NAMESPACE

class QX11MenuBarAdapter
{
public:
    QX11MenuBarAdapter(QMenuBar *, const QString &);
    ~QX11MenuBarAdapter();
    void addAction(QAction *, QAction *before=0);
    void removeAction(QAction *);
    void syncAction(QAction *) {}

    bool registerWindow();

    void popupAction(QAction *);

    uint registeredWinId;

private:
    DBusMenuExporter *exporter;
    QMenu *rootMenu;
    QMenuBar *menuBar;
    QString objectPath;
};

QX11MenuBarImpl::~QX11MenuBarImpl()
{
    destroyMenuBar();
}

void QX11MenuBarImpl::init(QMenuBar *_menuBar)
{
    nativeMenuBar = -1;
    menuBar = _menuBar;

    static int menuBarId = 1;
    objectPath = QString(QLatin1String("/MenuBar/%1")).arg(menuBarId++);
    // FIXME: Service name is duplicated in qmenu_x11.cpp
    menuBarServiceWatcher = new QDBusServiceWatcher(
        QLatin1String("com.canonical.AppMenu.Registrar"),
        QDBusConnection::sessionBus(),
        QDBusServiceWatcher::WatchForOwnerChange,
        menuBar);
    // adapter will be created in handleReparent()
    adapter = 0;

    connect(menuBarServiceWatcher, SIGNAL(serviceOwnerChanged(const QString &, const QString &, const QString &)),
        SLOT(slotMenuBarServiceChanged(const QString &, const QString &, const QString &)));
}

bool QX11MenuBarImpl::allowSetVisible() const
{
    return true;
}

void QX11MenuBarImpl::actionEvent(QActionEvent *e)
{
    if (adapter) {
        if(e->type() == QEvent::ActionAdded)
            adapter->addAction(e->action(), e->before());
        else if(e->type() == QEvent::ActionRemoved)
            adapter->removeAction(e->action());
        else if(e->type() == QEvent::ActionChanged)
            adapter->syncAction(e->action());
    }
}

void QX11MenuBarImpl::handleReparent(QWidget *oldParent, QWidget *newParent, QWidget *oldWindow, QWidget *newWindow)
{
    Q_UNUSED(oldParent)
    Q_UNUSED(newParent)
    if (isNativeMenuBar()) {
        if (adapter) {
            if (oldWindow != newWindow) {
                adapter->registerWindow();
            }
        } else {
            createMenuBar();
        }
    }
}

bool QX11MenuBarImpl::allowCornerWidgets() const
{
    return !isNativeMenuBar();
}

void QX11MenuBarImpl::popupAction(QAction *act)
{
    if (act && act->menu()) {
        adapter->popupAction(act);
    }
}

void QX11MenuBarImpl::setNativeMenuBar(bool value)
{
    if (nativeMenuBar == -1 || (value != bool(nativeMenuBar))) {
        nativeMenuBar = value;
        if (!nativeMenuBar) {
            destroyMenuBar();
        }
    }
}

bool QX11MenuBarImpl::isNativeMenuBar() const
{
    if (nativeMenuBar == -1) {
        return !QApplication::instance()->testAttribute(Qt::AA_DontUseNativeMenuBar);
    }
    return nativeMenuBar;
}

bool QX11MenuBarImpl::shortcutsHandledByNativeMenuBar() const
{
    return false;
}

bool QX11MenuBarImpl::menuBarEventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::WinIdChange) {
        if (isNativeMenuBar() && adapter) {
            QMetaObject::invokeMethod(this, "registerWindow", Qt::QueuedConnection);
        }
    }
    return false;
}

void QX11MenuBarImpl::slotMenuBarServiceChanged(const QString &/*serviceName*/, const QString &/*oldOwner*/, const QString &newOwner)
{
    if (newOwner.isEmpty()) {
        destroyMenuBar();
        // This is needed for the menu to come back, but then it will never be
        // moved again to the menubar :/
        //QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
        menuBar->updateGeometry();
        menuBar->setVisible(false);
        menuBar->setVisible(true);
        return;
    }
    if (adapter) {
        adapter->registeredWinId = 0;
        adapter->registerWindow();
    } else
        createMenuBar();
}

void QX11MenuBarImpl::registerWindow()
{
    if (adapter)
        adapter->registerWindow();
}

void QX11MenuBarImpl::checkMenuBar()
{
    menuBar->setVisible(!menuBar->window()->isMaximized());
}

void QX11MenuBarImpl::createMenuBar()
{
    static bool firstCall = true;
    static bool envSaysNo = !qgetenv("QT_X11_NO_NATIVE_MENUBAR").isEmpty();
    static bool envSaysBoth = qgetenv("APPMENU_DISPLAY_BOTH") == "1";

    if (!menuBar->parentWidget())
        return;

    adapter = 0;

    if (!firstCall && QApplication::testAttribute(Qt::AA_DontUseNativeMenuBar))
        return;

    if (envSaysNo) {
        if (firstCall) {
            firstCall = false;
            QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
        }
        return;
    }

    adapter = new QX11MenuBarAdapter(menuBar, objectPath);
    if (!adapter->registerWindow())
        destroyMenuBar();

    if (firstCall) {
        firstCall = false;
        bool dontUseNativeMenuBar = !adapter;

        // Make the rest of Qt think we do not use the native menubar, so
        // that space for the menubar widget is correctly allocated
        dontUseNativeMenuBar = true;
        QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, dontUseNativeMenuBar);
    }

    // HACK!... but there are no signals (I know of) that allow tracking
    // window changes
    if (!envSaysBoth) {
	QTimer *timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(checkMenuBar()));
	timer->start(100);
    }
}

void QX11MenuBarImpl::destroyMenuBar()
{
    delete adapter;
    adapter = 0;
}

QX11MenuBarAdapter::QX11MenuBarAdapter(QMenuBar *_menuBar, const QString &_objectPath)
: registeredWinId(0)
, exporter(0)
, rootMenu(new QMenu)
, menuBar(_menuBar)
, objectPath(_objectPath)
{
}

bool QX11MenuBarAdapter::registerWindow()
{
    if (!menuBar->window()) {
        qWarning() << __FUNCTION__ << "No parent for this menubar";
        return false;
    }

    uint winId = menuBar->window()->winId();
    if (winId == registeredWinId) {
        return true;
    }

    QDBusInterface host(QLatin1String("com.canonical.AppMenu.Registrar"), QLatin1String("/com/canonical/AppMenu/Registrar"), QLatin1String("com.canonical.AppMenu.Registrar"));
    if (!host.isValid()) {
        return false;
    }

    Q_FOREACH(QAction *action, menuBar->actions()) {
        if (!action->isSeparator()) {
            rootMenu->addAction(action);
        }
    }

    if (rootMenu->actions().isEmpty()) {
        return true;
    }

    if (!exporter) {
        exporter = new DBusMenuExporter(objectPath, rootMenu);
    }

    registeredWinId = winId;
    QVariant path = QVariant::fromValue<QDBusObjectPath>(QDBusObjectPath(objectPath));
    host.asyncCall(QLatin1String("RegisterWindow"), QVariant(winId), path);
    return true;
}

QX11MenuBarAdapter::~QX11MenuBarAdapter()
{
    delete exporter;
    exporter = 0;
    delete rootMenu;
    rootMenu = 0;
}

void QX11MenuBarAdapter::addAction(QAction *action, QAction *before)
{
    if (!action->isSeparator()) {
        rootMenu->insertAction(before, action);
    }
    if (!registeredWinId) {
        registerWindow();
    }
}

void QX11MenuBarAdapter::removeAction(QAction *action)
{
    rootMenu->removeAction(action);
}

void QX11MenuBarAdapter::popupAction(QAction *action)
{
    exporter->activateAction(action);
}

QAbstractMenuBarImpl* QX11MenuBarImplFactory::createImpl()
{
    return new QX11MenuBarImpl;
}

Q_EXPORT_PLUGIN2(qx11menubarimpl, QX11MenuBarImplFactory)

QT_END_NAMESPACE

#include <qx11menubarimpl.moc>
