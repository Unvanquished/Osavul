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

#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace unv;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    // those are here for connectSlotsByName
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setObjectName("trayIcon");

    trayIconMenu = new QMenu(this);
    trayIconMenu->setObjectName("trayIconMenu");

    chat = new IrcClient(this);
    chat->setObjectName("ircChat");

    // and this is where connectSlotsByName happens
    ui->setupUi(this);

    trayIconMenu->addAction(ui->actionRestore);

    QMenu *favoriteServersMenu = trayIconMenu->addMenu(tr("Favorite servers"));

    trayIconMenu->addSeparator();
    trayIconMenu->addAction(ui->actionQuit);

    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setIcon(QIcon(":images/unvanquished_tray_icon.png"));
    trayIcon->setToolTip(tr("Osavul, the Unvanquished server browser"));
    trayIcon->show();

    // loading the UI settings
    bool optionState = settings.value("ui/showSpectatorList").toBool();
    ui->spectatorWidget->setVisible(optionState);
    ui->showSpectatorsButton->setChecked(optionState);

    ui->serverSplitter->setStretchFactor(0, 3);
    ui->serverSplitter->setStretchFactor(1, 1);

    RichTextDelegate *primaryDelegate = new RichTextDelegate(this);
    primaryDelegate->setAlignment(Qt::AlignCenter);
    ui->serverTable->setItemDelegateForColumn(1, primaryDelegate);
    ui->playerTreeWidget->setItemDelegate(primaryDelegate);
    ui->spectatorList->setItemDelegate(primaryDelegate);

    RichTextDelegate *playerTableDelegate = new RichTextDelegate(this);
    playerTableDelegate->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->alienTable->setItemDelegateForColumn(2, playerTableDelegate);
    ui->humanTable->setItemDelegateForColumn(2, playerTableDelegate);

    QThread *unvThread = new QThread(this);
    connect(qApp, SIGNAL(aboutToQuit()),
            unvThread, SLOT(quit()));
    connect(&msv, SIGNAL(unvQueried(unv::GameServer *)),
            this, SLOT(handle_unvanquished_queried(unv::GameServer *)));
    connect(unvThread, SIGNAL(started()),
            &msv, SLOT(query()));

    msv.moveToThread(unvThread);
    unvThread->start();

    QThread *ircThread = new QThread(this);
    chat->setParent(nullptr);
    connect(qApp, SIGNAL(aboutToQuit()),
            ircThread, SLOT(quit()));
    connect(this, SIGNAL(ircChangeNick(QString)),
            chat, SLOT(setNickName(QString)));

    chat->moveToThread(ircThread);
    ircThread->start();

    ui->serverTable->addAction(ui->actionAdd_to_Favorites);

    int size = settings.beginReadArray("favorites");

    QSignalMapper *mapper = new QSignalMapper(this);

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        unv::FavoriteEntry fav;

        fav.name = settings.value("name").toString();
        fav.host = settings.value("address").toString();
        fav.port = settings.value("port").toUInt();

        favorites.append(fav);

        QAction *action = favoriteServersMenu->addAction(fav.name);
        connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
        mapper->setMapping(action, QString("%1:%2").arg(fav.host).arg(fav.port));
    }

    connect(mapper, SIGNAL(mapped(QString)), this, SLOT(connectTo(QString)));

    settings.endArray();

    qsrand(static_cast<uint>(QTime::currentTime().msec()));
}

MainWindow::~MainWindow()
{
    settings.beginWriteArray("favorites");

    for (int i = 0; i < favorites.length(); ++i) {
        auto fav = favorites.at(i);
        settings.setArrayIndex(i);
        settings.setValue("name", fav.name);
        settings.setValue("address", fav.host);
        settings.setValue("port", fav.port);
    }

    settings.endArray();

    delete ui;
}

void MainWindow::on_ircChat_addStringToChannel(Channel *channel, const QString &string) {
    channel->addString(string);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!trayIcon->isVisible())
        return;

    bool doShow = settings.value("ui/showTrayNotice").toBool();

    if (doShow) {
        QMessageBox mb(QMessageBox::Information,
                       tr("Osavul"),
                       tr("Osavul will keep running in the system tray. To terminate the program, "
                          "choose <b>Quit</b> in the context menu of the system tray entry."),
                       QMessageBox::Ok,
                       this);

        QPushButton *pb = mb.addButton(tr("Do not show again"), QMessageBox::RejectRole);

        mb.exec();

        if (mb.clickedButton() == pb)
            settings.setValue("ui/showTrayNotice", false);
    }

    this->hide();
    event->ignore();
}

void MainWindow::on_ircConnectButton_clicked()
{
    static const QList<QPushButton *> ircButtons =
        { ui->ircJoinButton, ui->ircNickButton, ui->ircQueryButton };

    if (!chat->isConnected()) {
        ConnectionDialog dlg(this);

        if (dlg.exec() != QDialog::Accepted)
            return;

        QString host = dlg.getHost();
        quint16 port = dlg.getPort();
        QString nick = dlg.getNick();

        chat->connectToHost(host, port);
        chat->init(nick);

        ui->ircConnectButton->setText(tr("Disc&onnect"));

        for (QPushButton *b : ircButtons)
            b->setEnabled(true);
    } else {
        chat->die();

        ui->ircConnectButton->setText(tr("C&onnect"));

        for (QPushButton *b : ircButtons)
            b->setEnabled(false);
    }
}

void MainWindow::handle_unvanquished_queried(unv::GameServer *sv)
{
    using namespace unv;
    static const QString templ = tr("%1 servers queried");

    bool isNew = true;

    if (gameServersShown.keys().contains(sv))
        isNew = false;

    int k = isNew ? ui->serverTable->rowCount() : gameServersShown.value(sv)->row();

    if (isNew)
        ui->serverTable->insertRow(k);

    QList<QTableWidgetItem *> items;

    for (int i = 0; i < 5; ++i)
        items << (isNew ? new QTableWidgetItem : ui->serverTable->item(k, i));

    items.at(0)->setText(sv->game());
    items.at(1)->setText(sv->name());
    items.at(2)->setText(sv->map());
    items.at(3)->setText(QString::number(sv->ping()));
    items.at(4)->setText(sv->formattedClientCount());

    if (isNew) {
        QVariant svv = QVariant::fromValue<GameServer *>(sv);

        for (int j = 0; j < ui->serverTable->columnCount(); ++j) {
            auto item = items.at(j);
            item->setData(Qt::UserRole, svv);
            item->setTextAlignment(Qt::AlignHCenter);
            ui->serverTable->setItem(k, j, item);
        }
    }

    if (!isNew)
        if (ui->serverTable->currentItem()->data(Qt::UserRole).value<unv::GameServer *>() == sv)
            updateTeamTables(sv->players());

    ui->serverTable->resizeColumnsToContents();
    ui->serverTable->horizontalHeader()->setStretchLastSection(true);

    if (!ui->refreshButton->isEnabled())
        ui->statusBar->showMessage(templ.arg(ui->serverTable->rowCount()), 3000);
}

void MainWindow::updateTeamTables(const QList<Player> &playerList)
{
    using namespace unv;
    static const QList<QTableWidget *> teamTables = { ui->alienTable, ui->humanTable };

    for (QTableWidget *table : teamTables) {
        table->clearContents();
        table->setRowCount(0);
    }

    ui->spectatorList->clear();

    for (const Player &p : playerList) {
        Player::Team team = p.team();

        if (team == Player::Aliens || team == Player::Humans) {
            auto tbl = teamTables.at(static_cast<int>(team) - 1);
            int j = tbl->rowCount();

            tbl->insertRow(j);
            QTableWidgetItem *scoreItem = new QTableWidgetItem;
            scoreItem->setData(Qt::EditRole, p.score());
            tbl->setItem(j, 0, scoreItem);
            QTableWidgetItem *pingItem = new QTableWidgetItem;
            pingItem->setData(Qt::EditRole, p.ping());
            tbl->setItem(j, 1, pingItem);
            tbl->setItem(j, 2, new QTableWidgetItem(p.name()));

            // sort by score
            tbl->sortByColumn(0);
        } else if (team == Player::Spectators) {
            auto it = new QListWidgetItem(p.name());
            ui->spectatorList->addItem(it);
        }
    }
}

void MainWindow::on_ircChat_serverCommMessage(const QString &s)
{
    static const QString templ = "[HH:mm:ss] ";
    QTime time = QTime::currentTime();

    ui->ircDisplayArea->setTextColor(Qt::white);
    ui->ircDisplayArea->append(time.toString(templ) + s);
}

void MainWindow::on_refreshButton_clicked()
{
    ui->serverTable->clearContents();
    ui->serverTable->setRowCount(0);

    ui->refreshButton->setEnabled(false);
    msv.query();

    // I am _really_ looking forward to C++11 lambda support in Qt; this is an ugly tmp workaround
    QTimer::singleShot(2000, this, SLOT(enableRefreshButton()));
}

void MainWindow::enableRefreshButton()
{
    ui->refreshButton->setEnabled(true);
}

void MainWindow::enableSyncButton()
{
    ui->syncButton->setEnabled(true);
}

void MainWindow::on_joinButton_clicked()
{
    if (!ui->serverTable->currentItem()) {
        ui->statusBar->showMessage(tr("No server selected!"), 3000);
        return;
    }

    auto sv = ui->serverTable->currentItem()->data(Qt::UserRole).value<unv::GameServer *>();

    this->connectTo(sv->formattedAddress());
}

void MainWindow::connectTo(const QString &host)
{
//    QProcess *proc = new QProcess(this);
    QString path = settings.value("unv/clientExecutablePath", "unvanquished").toString();

    ui->statusBar->showMessage(tr("Launching Unvanquished..."), 3000);

    qDebug() << path << "+connect" << host;
    QProcess::startDetached(path, { "+connect", host });
}

void MainWindow::on_syncButton_clicked()
{
    if (!ui->serverTable->currentItem()) {
        ui->statusBar->showMessage("No server selected!", 3000);
        return;
    }

    ui->syncButton->setEnabled(false);
    ui->serverTable->currentItem()->data(Qt::UserRole).value<unv::GameServer *>()->query();
    QTimer::singleShot(2000, this, SLOT(enableSyncButton()));
}

void MainWindow::on_serverTable_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *)
{
    if (!current)
        return;

    auto sv = current->data(Qt::UserRole).value<unv::GameServer *>();
    ui->serverName->setText(sv->name());
    ui->serverHost->setText(sv->formattedAddress());
    updateTeamTables(sv->players());
}

void MainWindow::on_actionRestore_triggered()
{
    this->showNormal();
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_trayIcon_activated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
        case QSystemTrayIcon::Unknown:
        case QSystemTrayIcon::Trigger:
            if (this->isVisible())
                this->hide();
            else
                this->showNormal();

            break;
        case QSystemTrayIcon::Context:
            trayIconMenu->show();
            break;
        default:
            break;
    }
}

void MainWindow::on_showSpectatorsButton_clicked(bool checked)
{
    // this is not the sole action — another connection is made directly in the .ui file.
    settings.setValue("ui/showSpectatorList", checked);
}

void MainWindow::on_playerFilterLineEdit_textEdited(const QString &arg1)
{
    static const QString templ = tr("Found %1 players on %2 servers");
    static int previousLength;
    int playersFound = 0;
    int currentLength = arg1.length();

    // all of collection, generalization and filtering are O(n²), but filtering is made over a
    // smaller dataset. not a problem just yet, seeing as premature optimization is the root of all
    // evil™.

    if ((currentLength == 1 && previousLength == 0) // entering first letter
        || (currentLength < previousLength)) { // initial collection or generalization
        ui->playerTreeWidget->clear();
        for (const GameServer *sv : msv.servers()) {

            if (sv->players().isEmpty())
                continue;

            auto svItem = new QTreeWidgetItem(ui->playerTreeWidget);
            svItem->setText(0, sv->name());

            for (const Player &p : sv->players()) {
                if (p.plainName().contains(arg1, Qt::CaseInsensitive)) {
                    auto pItem = new QTreeWidgetItem(svItem);
                    pItem->setText(0, p.name());
                    pItem->setData(0, Qt::UserRole, p.plainName());
                }
            }

            playersFound += svItem->childCount();
        }

        ui->playerTreeWidget->expandAll();
    } else if (currentLength > previousLength) { // filtering
        for (int i = 0; i < ui->playerTreeWidget->topLevelItemCount(); ++i) {
            auto parent = ui->playerTreeWidget->topLevelItem(i);

            for (int j = 0; j < parent->childCount(); ++j) {
                QString name = parent->child(j)->data(0, Qt::UserRole).toString();

                if (!name.contains(arg1, Qt::CaseInsensitive)) {
                    delete parent->takeChild(j);
                    --j;
                }
            }

            if (parent->childCount() == 0) {
                delete ui->playerTreeWidget->takeTopLevelItem(i);
                --i;
            } else {
                playersFound += parent->childCount();
            }
        }
    }

    ui->statusBar->showMessage(templ
                               .arg(playersFound)
                               .arg(ui->playerTreeWidget->topLevelItemCount()),
                               3000);

    previousLength = currentLength;
}

void MainWindow::on_serverTable_itemChanged(QTableWidgetItem *item)
{
    if (!item)
        return;

    gameServersShown.insertMulti(item->data(Qt::UserRole).value<unv::GameServer *>(), item);
}

void MainWindow::on_ircJoinButton_clicked()
{
    bool ok;
    QString chan = QInputDialog::getText(this,
                                         tr("Join Channel"),
                                         tr("Channel:"),
                                         QLineEdit::Normal,
                                         "",
                                         &ok);

    if (Q_UNLIKELY(!ok))
        return;

    Channel *newTab = new Channel(this, chan);
    chat->join(newTab);

    connect(newTab, SIGNAL(ircPart(Channel*)),
            chat, SLOT(part(Channel*)));
    connect(newTab, SIGNAL(ircSay(Channel*,QString)),
            chat, SLOT(say(Channel*,QString)));
    ui->ircTabWidget->addTab(newTab, chan);
    ui->ircTabWidget->setCurrentWidget(newTab);
}

void MainWindow::on_ircNickButton_clicked()
{
    bool ok;
    QString nick = QInputDialog::getText(this,
                                         tr("Change Nickname"),
                                         tr("New Nickname:"),
                                         QLineEdit::Normal,
                                         "",
                                         &ok);

    if (Q_UNLIKELY(!ok))
        return;

    chat->setNickName(nick);
}

void MainWindow::on_ircQueryButton_clicked()
{
    bool ok;
    QString user = QInputDialog::getText(this,
                                         tr("Query User"),
                                         tr("Username:"),
                                         QLineEdit::Normal,
                                         "",
                                         &ok);

    if (Q_UNLIKELY(!ok))
        return;

    Channel *userTab = new Channel(this, user);
    chat->join(userTab);

    connect(userTab, SIGNAL(ircPart(Channel*)),
            chat, SLOT(part(Channel*)));
    connect(userTab, SIGNAL(ircSay(Channel*,QString)),
            chat, SLOT(say(Channel*,QString)));
    ui->ircTabWidget->addTab(userTab, user);
    ui->ircTabWidget->setCurrentWidget(userTab);
}

void MainWindow::on_ircTabWidget_tabCloseRequested(int index)
{
    if (index == 0 && chat->isConnected()) {
        ui->ircConnectButton->click();
        return;
    }

    if (index == 0) {
        QMessageBox::information(this,
                                 "...",
                                 "Excuse me wtf are you doing",
                                 QMessageBox::Ok);
        return;
    }

    Channel *tab = qobject_cast<Channel *>(ui->ircTabWidget->widget(index));

    if (tab)
        delete tab;
}

void MainWindow::on_actionAdd_to_Favorites_triggered()
{
    auto sv = ui->serverTable->currentItem()->data(Qt::UserRole).value<unv::GameServer *>();

    unv::FavoriteEntry f;
    f.name = sv->name();
    f.host = sv->host();
    f.port = sv->port();
    favorites << f;
}

void MainWindow::on_filterBar_textEdited(const QString &arg1)
{
    for (QTableWidgetItem *it : gameServersShown.values()) {
        unv::GameServer *sv = it->data(Qt::UserRole).value<unv::GameServer *>();

        if (sv->infoString().contains(arg1, Qt::CaseInsensitive))
            ui->serverTable->showRow(it->row());
        else
            ui->serverTable->hideRow(it->row());
    }
}

void MainWindow::on_actionAbout_Osavul_triggered()
{
    QMessageBox::about(this,
                       tr("About Osavul"),
                       tr("<h3>About Osavul</h3>"
                       "<p>Osavul is a server browser application meant specifically for querying "
                       "Unvanquished servers.</p>"
                       "<p>Osavul uses C++ with Qt, a cross-platform widget toolkit. You can read "
                       "more about it in the \"About Qt\" dialog by choosing the respective option "
                       "in the \"Help\" menu.</p>"
                       "<p>Osavul is licensed under the GNU General Public License version 3.</p>"
                       "<p>(C) Qrntz 2012</p>"));
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    qApp->aboutQt();
}

void MainWindow::on_actionPreferences_triggered()
{
    SettingsDialog dlg(this);
    dlg.exec();
}
