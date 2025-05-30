/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * LXQt - a lightweight, Qt based, desktop toolset
 * https://lxqt.org/
 *
 * Copyright: 2010-2015 LXQt team
 * Authors:
 *   Paulo Lieuthier <paulolieuthier@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "leavedialog.h"

#include <QListWidgetItem>
#include <QGuiApplication>
#include <QRect>
#include <QScreen>
#include <QWindow>
#include <QShowEvent>

#include <LayerShellQt/shell.h>
#include <LayerShellQt/window.h>


LeaveDialog::LeaveDialog(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint),
    ui(new Ui::LeaveDialog),
    mPower(new LXQt::Power(this)),
    mPowerManager(new LXQt::PowerManager(this)),
    mScreensaver(new LXQt::ScreenSaver(this))
{
    ui->setupUi(this);

    /* This is a special dialog. We want to make it hard to ignore.
       We make it:
           * Undraggable
           * Frameless
           * Stays on top of all other windows
           * Present in all desktops
    */
    setWindowFlags((Qt::CustomizeWindowHint | Qt::FramelessWindowHint |
                    Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint));

    // populate the items
    QListWidgetItem * item = new QListWidgetItem{QIcon::fromTheme(QStringLiteral("system-log-out")), tr("Logout")};
    item->setData(Qt::UserRole, LXQt::Power::PowerLogout);
    if (!mPower->canAction(LXQt::Power::PowerLogout))
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    ui->listWidget->addItem(item);
    item = new QListWidgetItem{QIcon::fromTheme(QStringLiteral("system-shutdown")), tr("Shutdown")};
    item->setData(Qt::UserRole, LXQt::Power::PowerShutdown);
    if (!mPower->canAction(LXQt::Power::PowerShutdown))
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    ui->listWidget->addItem(item);
    item = new QListWidgetItem{QIcon::fromTheme(QStringLiteral("system-suspend")), tr("Suspend")};
    item->setData(Qt::UserRole, LXQt::Power::PowerSuspend);
    if (!mPower->canAction(LXQt::Power::PowerSuspend))
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    ui->listWidget->addItem(item);
    item = new QListWidgetItem{QIcon::fromTheme(QStringLiteral("system-lock-screen")), tr("Lock screen")};
    item->setData(Qt::UserRole, -1);
    ui->listWidget->addItem(item);
    item = new QListWidgetItem{QIcon::fromTheme(QStringLiteral("system-reboot")), tr("Reboot")};
    item->setData(Qt::UserRole, LXQt::Power::PowerReboot);
    if (!mPower->canAction(LXQt::Power::PowerReboot))
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    ui->listWidget->addItem(item);
    item = new QListWidgetItem{QIcon::fromTheme(QStringLiteral("system-suspend-hibernate")), tr("Hibernate")};
    item->setData(Qt::UserRole, LXQt::Power::PowerHibernate);
    if (!mPower->canAction(LXQt::Power::PowerHibernate))
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    ui->listWidget->addItem(item);

    ui->listWidget->setRows(2);
    ui->listWidget->setColumns(3);

    connect(ui->listWidget, &QAbstractItemView::activated, this, [this] (const QModelIndex & index) {
        bool ok = false;
        const int action = index.data(Qt::UserRole).toInt(&ok);
        if (!ok)
        {
            qWarning("Invalid internal logic, no UserRole set!?");
            return;
        }
        close();
        switch (action)
        {
        case LXQt::Power::PowerLogout:
                mPowerManager->logout();
                break;
            case LXQt::Power::PowerShutdown:
                mPowerManager->shutdown();
                break;
            case LXQt::Power::PowerSuspend:
                mPowerManager->suspend();
                break;
            case -1:
                {
                    QEventLoop loop;
                    connect(mScreensaver, &LXQt::ScreenSaver::done, &loop, &QEventLoop::quit);
                    mScreensaver->lockScreen();
                    loop.exec();
                }
                break;
            case LXQt::Power::PowerReboot:
                mPowerManager->reboot();
                break;
            case LXQt::Power::PowerHibernate:
                mPowerManager->hibernate();
                break;
        }
    });
    connect(ui->cancelButton, &QAbstractButton::clicked, this, [this] { close(); });
}

LeaveDialog::~LeaveDialog()
{
    delete ui;
}

void LeaveDialog::showEvent(QShowEvent *event)
{
    // set the layer and centered position under Wayland
    if (QGuiApplication::platformName() == QStringLiteral("wayland"))
    {
        winId();
        if(QWindow *win = windowHandle())
        {
            if (LayerShellQt::Window *layershell = LayerShellQt::Window::get(win))
            {
                layershell->setLayer(LayerShellQt::Window::Layer::LayerOverlay);
                layershell->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
                layershell->setScreenConfiguration(LayerShellQt::Window::ScreenConfiguration::ScreenFromCompositor);
                LayerShellQt::Window::Anchors anchors = {LayerShellQt::Window::AnchorTop};
                layershell->setAnchors(anchors);
                layershell->setExclusiveZone(-1);
                layershell->setScope(QStringLiteral("dialog"));
                if (QScreen *screen = win->screen())
                {
                    QRect ag = screen->availableGeometry();
                    // NOTE: Since QShowEvent is non-spontaneous here, the window is not shown yet,
                    // and so, this screen may not be the one with the cursor. Unfortunately, Qt
                    // provides no way of knowing the screen with the cursor. Therefore, to prevent
                    // an offscreen window, we should find the smallest geometry and center the
                    // window only on it, at the cost of positioning the window above the centers
                    // of the other screens.
                    const auto allScreens = QGuiApplication::screens();
                    for (const auto &screen : allScreens)
                    {
                        QRect sag = screen->availableGeometry();
                        if(ag.width() > sag.width())
                            ag.setWidth(sag.width());
                        if(ag.height() > sag.height())
                            ag.setHeight(sag.height());
                    }
                    int topMargin = (ag.height() - sizeHint().height()) / 2;
                    topMargin = qMax(topMargin, 0);
                    layershell->setMargins(QMargins(0, topMargin, 0, 0));
                }
            }
        }
    }
    QDialog::showEvent(event);
}
