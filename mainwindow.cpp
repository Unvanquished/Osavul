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

#include <functional>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#ifdef __unix__
#include <unistd.h>
#include <fcntl.h>
#endif

#define TIMEOUT 3000

using namespace unv;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow),
    trayIcon(0), trayIconMenu(0)
{
    this->detectSystemTray();

    chat = new IrcClient(this);
    chat->setObjectName("ircChat");

    msv = new MasterServer(settings.value("unv/masterServerAddress",
                                          "unvanquished.net").toString(),
                           settings.value("unv/masterServerPort",
                                          27950).toUInt(),
                           settings.value("unv/masterServerProtocol",
                                          86).toUInt());
    msv->setParent(this);
    msv->setObjectName("masterServer");

    // this is where connectSlotsByName happens
    ui->setupUi(this);

    // calling isSystemTrayAvailable() in such cases will return true if the system tray started
    // being available _after_ it is found out that it isn't, which is not the desired behavior.
    // on the other hand, trayIcon and trayIconMenu are only initialized once in Osavul's lifetime.
    if (trayIcon && trayIconMenu) {
        trayIconMenu->addAction(ui->actionRestore);
        trayIconMenu->addSeparator();
        trayIconMenu->addMenu(ui->menuFavorites);
        trayIconMenu->addAction(ui->actionQuit);

        trayIcon->setContextMenu(trayIconMenu);
        trayIcon->setIcon(QIcon(":images/unvanquished_tray_icon.png"));
        trayIcon->setToolTip(tr("Osavul, the Unvanquished server browser"));
        trayIcon->show();
    }

    // loading the UI settings
    bool optionState = settings.value("mainWindow/showSpectatorList").toBool();
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
    msv->setParent(nullptr);
    connect(qApp, SIGNAL(aboutToQuit()),
            unvThread, SLOT(quit()));
    connect(unvThread, SIGNAL(started()),
            msv, SLOT(query()));

    msv->moveToThread(unvThread);
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

    this->loadFavorites();

    // restore window geometry
    settings.beginGroup("mainWindow");
    this->resize(settings.value("size", this->size()).toSize());
    this->move(settings.value("pos", this->pos()).toPoint());
    settings.endGroup();

    this->loadClanList();

    qsrand(static_cast<uint>(QTime::currentTime().msec()));
}

void MainWindow::detectSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        if (!settings.value("mainWindow/doNotShowTrayUnavailableNotice").toBool()) {
            QMessageBox mb(QMessageBox::Warning,
                           tr("Systray"),
                           tr("Osavul could not detect any system tray on this system. "
                              "Systray support will be disabled."),
                           QMessageBox::Ok,
                           this);

            QPushButton *pb = mb.addButton(tr("Do not show again"), QMessageBox::RejectRole);

            mb.exec();
            if (mb.clickedButton() == pb)
                settings.setValue("mainWindow/doNotShowTrayUnavailableNotice", true);
        }

        QApplication::setQuitOnLastWindowClosed(true);
    } else {
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setObjectName("trayIcon");

        trayIconMenu = new QMenu(this);
        trayIconMenu->setObjectName("trayIconMenu");

        connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                this, SLOT(when_trayIcon_activated(QSystemTrayIcon::ActivationReason)));
    }
}

void MainWindow::loadFavorites()
{
    int size = settings.beginReadArray("favorites");

    if (size)
        ui->menuFavorites->insertSeparator(ui->actionManage_Favorites);

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        unv::FavoriteEntry fav;

        fav.name = settings.value("name").toString();
        fav.host = settings.value("address").toString();
        fav.port = settings.value("port").toUInt();

        this->addFavorite(fav);
    }

    settings.endArray();
}

void MainWindow::addFavorite(FavoriteEntry fav)
{
    static QSignalMapper *mapper;

    if (!mapper) {
        ui->menuFavorites->insertSeparator(ui->actionManage_Favorites);

        mapper = new QSignalMapper(this);
        connect(mapper, SIGNAL(mapped(QString)), this, SLOT(connectTo(QString)));
    }

    favorites << fav;
    QAction *action = new QAction(fav.name, ui->menuFavorites);
    action->setData(QVariant::fromValue<QSignalMapper *>(mapper));
    ui->menuFavorites->insertAction(ui->actionManage_Favorites, action);
    connect(action, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(action, QString("%1:%2").arg(fav.host).arg(fav.port));
}

MainWindow::~MainWindow()
{
    // save window geometry
    settings.beginGroup("mainWindow");
    settings.setValue("size", this->size());
    settings.setValue("pos", this->pos());
    settings.endGroup();

    // save favorites
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

void MainWindow::loadClanList()
{
    static const QString who = "Clan List Parser";

    QXmlSchema schema;
    if (!schema.load(QUrl::fromLocalFile(":/clans/clan_schema.xsd"))) {
        QMessageBox::critical(this,
                              who,
                              "Clan list XML schema is invalid!");
        return;
    }

    QFile clanList(":/clans/clans.xml");
    if (!clanList.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this,
                              who,
                              "Couldn't open clan list");
        return;
    }

    QXmlSchemaValidator validator(schema);
    if (!validator.validate(&clanList)) {
        QMessageBox::critical(this,
                              who,
                              "Clan list data is invalid");
        return;
    }

    // the validation changes position in the file
    clanList.reset();

    QXmlStreamReader xml(&clanList);

    xml.readNextStartElement();
    Q_ASSERT(xml.isStartElement() && xml.name() == "clanList");

    int row;
    while (xml.readNextStartElement()) {
        ui->clanTableWidget->insertRow(row = ui->clanTableWidget->rowCount());
        while (xml.readNextStartElement()) {
            int column;

            if (xml.name() == "name")
                column = 0;
            else if (xml.name() == "tag")
                column = 1;
            else if (xml.name() == "irc")
                column = 2;
            else if (xml.name() == "url")
                column = 3;
            else {
                xml.skipCurrentElement();
                continue;
            }

            auto it = new QTableWidgetItem(xml.readElementText());

            ui->clanTableWidget->setItem(row, column, it);
            ui->clanTableWidget->resizeColumnsToContents();
        }
        continue;
    }
}

void MainWindow::on_clanTableWidget_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *)
{
    if (!current)
        return;

    switch (current->column()) {
        case 3:
            QDesktopServices::openUrl(QUrl(current->text()));
            break;
        case 2:
            if (chat->isConnected())
                this->openChannel(current->text());
            break;
        default:
            break;
    }
}

void MainWindow::on_ircChat_addStringToChannel(Channel *channel, const QString &string)
{
    channel->addString(string);
}

void MainWindow::on_ircChat_highlight(Channel *channel)
{
    if (ui->ircTabWidget->currentWidget() == channel)
        return;

    int i = ui->ircTabWidget->indexOf(channel);
    QString tabText = ui->ircTabWidget->tabText(i);
    channel->setProperty("previousText", tabText);
    channel->setProperty("highlighted", true);
    ui->ircTabWidget->setTabText(i, "(!) " + tabText);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if ((!trayIcon && !trayIconMenu) || !trayIcon->isVisible()) {
        event->accept();
        return;
    }

    if (!settings.value("mainWindow/doNotShowHideToTrayNotice").toBool()) {
        QMessageBox mb(QMessageBox::Information,
                       tr("Osavul"),
                       tr("Osavul will keep running in the system tray. To terminate the program, "
                          "choose <b>Quit</b> in the context menu of the system tray entry."),
                       QMessageBox::Ok,
                       this);

        QPushButton *pb = mb.addButton(tr("Do not show again"), QMessageBox::RejectRole);

        mb.exec();

        if (mb.clickedButton() == pb)
            settings.setValue("mainWindow/doNotShowHideToTrayNotice", true);
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

void MainWindow::on_masterServer_serverQueried(unv::GameServer *sv)
{
    using namespace unv;

    bool isNew = true;

    bool sorting = ui->serverTable->isSortingEnabled();
    ui->serverTable->setSortingEnabled(false);

    if (gameServersShown.keys().contains(sv))
        isNew = false;

    int k = isNew ? ui->serverTable->rowCount() : gameServersShown.value(sv)->row();

    if (isNew)
        ui->serverTable->insertRow(k);

    QList<QTableWidgetItem *> items;

    for (int i = 0; i < 6; ++i)
        items << (isNew ? new QTableWidgetItem : ui->serverTable->item(k, i));

    bool ipv4 = sv->ipv4();
    bool ipv6 = sv->ipv6();
    bool linked = ipv4 & ipv6;

    items.at(0)->setText(sv->game());
    items.at(1)->setText(sv->name());
    items.at(2)->setText(linked ? "v4/v6" : ipv6 ? "v6" : "v4");
    items.at(3)->setText(sv->map());
    items.at(4)->setData(Qt::EditRole, sv->ping());
    items.at(5)->setText(sv->formattedClientCount());

    QFont udp_font = items.at(2)->font();
    udp_font.setBold(ipv6);
    udp_font.setItalic(linked);
    items.at(2)->setFont(udp_font);

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
            updateTeamTables(sv->players(), sv->lastUpdateTime());

    ui->serverTable->resizeColumnsToContents();
    ui->serverTable->horizontalHeader()->setStretchLastSection(true);

    if (!ui->refreshButton->isEnabled()) {
        int count = ui->serverTable->rowCount();
        ui->statusBar->showMessage(tr("%n server(s) queried", "", count), TIMEOUT);
    }

    ui->serverTable->setSortingEnabled(sorting);
}

void MainWindow::clearTeamTables()
{
    using namespace unv;
    static const QList<QTableWidget *> teamTables = { ui->alienTable, ui->humanTable };

    for (QTableWidget *table : teamTables) {
        table->clearContents();
        table->setRowCount(0);
    }

    ui->spectatorList->clear();
    ui->serverLastUpdate->setText("");
}

void MainWindow::updateTeamTables(const QList<Player> &playerList, const QDateTime &lastUpdateTime)
{
    using namespace unv;
    static const QList<QTableWidget *> teamTables = { ui->alienTable, ui->humanTable };

    clearTeamTables();

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
            if (p.isBot())
                pingItem->setData(Qt::EditRole, tr("bot"));
            else
                pingItem->setData(Qt::EditRole, p.ping());
            tbl->setItem(j, 1, pingItem);
            QTableWidgetItem *nameItem = new QTableWidgetItem(p.name());
            tbl->setItem(j, 2, nameItem);

            // sort by score
            tbl->sortByColumn(0);
        } else if (team == Player::Spectators) {
            auto it = new QListWidgetItem(p.name());
            ui->spectatorList->addItem(it);
        }
    }

    ui->serverLastUpdate->setText(lastUpdateTime.toString(tr("'Last update: 'dddd hh:mm:ss")));
}

void MainWindow::on_ircChat_serverCommMessage(const QString &s)
{
    static const QString templ = "[HH:mm:ss] ";

    ui->ircDisplayArea->append(QTime::currentTime().toString(templ) + s);
}

void MainWindow::on_refreshButton_clicked()
{
    static const auto infoLabels = { ui->serverName, ui->serverHost, ui->serverLastUpdate };

    ui->serverTable->clearContents();
    ui->serverTable->setRowCount(0);
    gameServersShown.clear();

    clearTeamTables();

    for (QLabel *l : infoLabels)
        l->text().clear();

    ui->refreshButton->setEnabled(false);
    msv->query();

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
        ui->statusBar->showMessage(tr("No server selected!"), TIMEOUT);
        return;
    }

    auto sv = ui->serverTable->currentItem()->data(Qt::UserRole).value<unv::GameServer *>();

    this->connectTo(sv->formattedAddress(true));
}

void MainWindow::connectTo(const QString &host)
{
    QString path = settings.value("unv/clientExecutablePath", "unvanquished").toString();
    QString url = "unv://" + host;

    ui->statusBar->showMessage(tr("Launching Unvanquished..."), TIMEOUT);

#ifdef __unix__
    fcntl(STDIN_FILENO,  F_SETFD, FD_CLOEXEC, 1);
    fcntl(STDOUT_FILENO, F_SETFD, FD_CLOEXEC, 1);
    fcntl(STDERR_FILENO, F_SETFD, FD_CLOEXEC, 1);
#endif

    QString cwd = QDir::toNativeSeparators(path.left(path.lastIndexOf('/')));
    if (!QProcess::startDetached(path, { url }, cwd))
        ui->statusBar->showMessage(tr("Daemon failed to start!"), TIMEOUT);
}

void MainWindow::on_syncButton_clicked()
{
    if (!ui->serverTable->currentItem()) {
        ui->statusBar->showMessage(tr("No server selected!"), TIMEOUT);
        return;
    }

    ui->syncButton->setEnabled(false);
    ui->serverTable->currentItem()->data(Qt::UserRole).value<unv::GameServer *>()->query();
    QTimer::singleShot(2000, this, SLOT(enableSyncButton()));
}

void MainWindow::on_statsButton_clicked()
{
    QString url = ui->statsButton->toolTip();

    if (url != nullptr)
        QDesktopServices::openUrl(QUrl(url));
}

void MainWindow::on_serverTable_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *)
{
    if (!current)
        return;

    auto sv = current->data(Qt::UserRole).value<unv::GameServer *>();
    bool ipv4 = sv->ipv4();
    bool ipv6 = sv->ipv6();
    bool linked = ipv4 & ipv6;

    ui->serverName->setText(sv->name());
    if (linked)
        ui->serverHost->setText(sv->formattedAddress(0) + "<br />" + sv->formattedAddress(1));
    else
        ui->serverHost->setText(sv->formattedAddress());

    ui->joinButton->setEnabled(true);
    ui->syncButton->setEnabled(true);

    QString stats = sv->statsURL();
    ui->statsButton->setEnabled(stats != nullptr);
    ui->statsButton->setToolTip(stats);

    updateTeamTables(sv->players(), sv->lastUpdateTime());
}

void MainWindow::on_actionRestore_triggered()
{
    this->showNormal();
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MainWindow::when_trayIcon_activated(QSystemTrayIcon::ActivationReason reason)
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
    settings.setValue("mainWindow/showSpectatorList", checked);
}

void MainWindow::on_playerFilterLineEdit_textEdited(const QString &arg1)
{
    static int previousLength;
    int playersFound = 0;
    int currentLength = arg1.length();

    const auto directPredicate =   [&] (const QString &s) {
        return s.contains(arg1, Qt::CaseInsensitive);
    };
    const auto negativePredicate = [&] (const QString &s) {
        return !s.contains(arg1.right(arg1.size() - 1), Qt::CaseInsensitive);
    };

    static std::function<bool (const QString &)> pred;

    bool negSearch;
    if ((negSearch = arg1.startsWith('!', Qt::CaseInsensitive)))
        pred = negativePredicate;
    else
        pred = directPredicate;

    // all of collection, generalization and filtering are O(n²), but filtering is made over a
    // smaller dataset. not a problem just yet, seeing as premature optimization is the root of all
    // evil™.

    if ((currentLength == 1 && previousLength == 0) // entering first letter
        || (currentLength < previousLength)
        || negSearch) { // initial collection or generalization
        ui->playerTreeWidget->clear();
        for (const GameServer *sv : msv->servers()) {

            if (sv->players().isEmpty())
                continue;

            auto svItem = new QTreeWidgetItem(ui->playerTreeWidget);
            svItem->setText(0, sv->name());

            for (const Player &p : sv->players()) {
                if (pred(p.plainName())) {
                    auto pItem = new QTreeWidgetItem(svItem);
                    pItem->setText(0, p.name());
                    pItem->setData(0, Qt::UserRole, p.plainName());
                }
            }

            if (svItem->childCount() == 0)
                    delete svItem;

            playersFound += svItem->childCount();
        }

        ui->playerTreeWidget->expandAll();
    } else if (currentLength > previousLength) { // filtering
        for (int i = 0; i < ui->playerTreeWidget->topLevelItemCount(); ++i) {
            auto parent = ui->playerTreeWidget->topLevelItem(i);

            for (int j = 0; j < parent->childCount(); ++j) {
                QString name = parent->child(j)->data(0, Qt::UserRole).toString();

                if (!name.contains(arg1, Qt::CaseInsensitive))
                    delete parent->takeChild(j), --j;
            }

            if (parent->childCount() == 0)
                delete ui->playerTreeWidget->takeTopLevelItem(i), --i;
            else
                playersFound += parent->childCount();
        }
    }

    int serversFound = ui->playerTreeWidget->topLevelItemCount();
    QString msg = tr("Found %n player(s) ", "Found <x> players on <y> servers [split as needed, one number each]", playersFound)
                + tr("on %n server(s)", "Found <x> players on <y> servers [split as needed, one number each]", serversFound);
    ui->statusBar->showMessage(msg, TIMEOUT);

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
    connect(newTab, SIGNAL(ircSendRaw(QString)),
            chat, SLOT(send(QString)));
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
    userTab->addUser(user);
}

void MainWindow::on_ircTabWidget_tabCloseRequested(int index)
{
    if (index == 0 && chat->isConnected()) {
        ui->ircConnectButton->click();
        return;
    }

    Channel *tab = qobject_cast<Channel *>(ui->ircTabWidget->widget(index));

    if (tab)
        delete tab;
}

void MainWindow::on_actionAdd_to_Favorites_triggered()
{
    if (!ui->serverTable->currentItem()) {
        ui->statusBar->showMessage(tr("No server selected!"), TIMEOUT);
        return;
    }

    auto sv = ui->serverTable->currentItem()->data(Qt::UserRole).value<unv::GameServer *>();

    unv::FavoriteEntry f;
    f.name = sv->name();
    f.host = sv->host();
    f.port = sv->port();

    this->addFavorite(f);
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
                       "<p>Osavul is licensed under the GNU General Public License version 3.</p>")
                       + "<p>© Qrntz 2012, Unvanquished Developers 2012-2013</p>");
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

void MainWindow::on_ircTabWidget_currentChanged(QWidget *arg1)
{
    if (!arg1)
        return;

    if (arg1->property("highlighted").toBool())
        if (arg1->property("previousText").isValid())
            ui->ircTabWidget->setTabText(ui->ircTabWidget->indexOf(arg1),
                                         arg1->property("previousText").toString());
    arg1->setProperty("highlighted", false);
}

Channel *MainWindow::openChannel(const QString &channel)
{
    Channel *newChannel = new Channel(this, channel);
    chat->join(newChannel);

    connect(newChannel, SIGNAL(ircPart(Channel*)),
            chat, SLOT(part(Channel*)));
    connect(newChannel, SIGNAL(ircSay(Channel*,QString)),
            chat, SLOT(say(Channel*,QString)));
    connect(newChannel, SIGNAL(ircSendRaw(QString)),
            chat, SLOT(send(QString)));
    ui->ircTabWidget->addTab(newChannel, channel);
    ui->ircTabWidget->setCurrentWidget(newChannel);

    return newChannel;
}

void MainWindow::on_actionManage_Favorites_triggered()
{
    FavoritesDialog dlg(favorites, this);

    if (dlg.exec() == FavoritesDialog::Accepted) {
        QList<QAction *> actions = ui->menuFavorites->actions();
        for (QAction *action : actions)
            if (!action->data().value<QSignalMapper *>())
                actions.removeOne(action);

        for (const QPair<int, FavoriteEntry> &pair : dlg.modifiedFavorites()) {
            favorites.replace(pair.first, pair.second);
            QAction *action = actions.at(pair.first);
            QSignalMapper *mapper = action->data().value<QSignalMapper *>();
            action->setText(pair.second.name);
            mapper->removeMappings(action);
            mapper->setMapping(action, QString("%1:%2").arg(pair.second.host).arg(pair.second.port));
        }
    }
}
