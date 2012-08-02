/*
* This file is part of Osavul.
*
* Osavul is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Osavul is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Osavul.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "channel.h"
#include "ircclient.h"
#include "unv.h"
#include "connectiondialog.h"
#include "richtextdelegate.h"
#include "settingsdialog.h"
#include "favoritesdialog.h"
#include <QtGui/QMainWindow>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QProcess>
#include <QtGui/QTableWidgetItem>
#include <QtCore/QSettings>
#include <QtGui/QSystemTrayIcon>
#include <QtGui/QMessageBox>
#include <QtGui/QCloseEvent>
#include <QtCore/QSignalMapper>
#include <QtGui/QInputDialog>
#include <QtCore/QXmlStreamReader>
#include <QtXmlPatterns/QXmlSchema>
#include <QtXmlPatterns/QXmlSchemaValidator>
#include <QtGui/QDesktopServices>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void on_refreshButton_clicked();
    void enableRefreshButton();
    void enableSyncButton();
    void on_joinButton_clicked();
    void on_syncButton_clicked();
    void on_serverTable_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *);

    void on_actionRestore_triggered();
    void on_actionQuit_triggered();
    void on_trayIcon_activated(QSystemTrayIcon::ActivationReason reason);

    void on_showSpectatorsButton_clicked(bool checked);
    void on_playerFilterLineEdit_textEdited(const QString &arg1);
    void on_serverTable_itemChanged(QTableWidgetItem *item);

    void on_ircConnectButton_clicked();
    void on_ircJoinButton_clicked();
    void on_ircNickButton_clicked();
    void on_ircQueryButton_clicked();
    void on_ircTabWidget_tabCloseRequested(int index);
    void on_actionAdd_to_Favorites_triggered();
    void on_ircChat_addStringToChannel(Channel *channel, const QString &string);
    void on_ircChat_highlight(Channel *channel);
    void on_filterBar_textEdited(const QString &arg1);
    void on_actionAbout_Osavul_triggered();
    void on_actionAbout_Qt_triggered();
    void connectTo(const QString &host);
    void on_actionPreferences_triggered();
    void on_ircTabWidget_currentChanged(QWidget *arg1);
    void on_clanTableWidget_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *);
    void on_actionManage_Favorites_triggered();

public slots:
    void on_ircChat_serverCommMessage(const QString &s);
    void on_masterServer_serverQueried(unv::GameServer *svNew);

private:
    void loadClanList();
    void loadFavorites();
    void updateTeamTables(const QList<unv::Player> &playerList);
    Channel *openChannel(const QString &channel);
    void addFavorite(unv::FavoriteEntry fav);

signals:
    void ircChangeNick(const QString &nick);
    void ircOpenQuery(const QString &user);

private:
    QList<unv::FavoriteEntry> favorites;
    QSettings settings;

    Ui::MainWindow *ui;
    QHash<unv::GameServer *, QTableWidgetItem *> gameServersShown;

    QMenu *trayIconMenu;
    QSystemTrayIcon *trayIcon;

    unv::MasterServer *msv;
    IrcClient *chat;
};

Q_DECLARE_METATYPE(QSignalMapper *)

#endif // MAINWINDOW_H
