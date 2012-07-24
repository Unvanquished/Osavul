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

#include "channel.h"
#include "ui_channel.h"

Channel::Channel(QWidget *parent, const QString &channel) :
    QWidget(parent),
    ui(new Ui::Channel),
    m_channel(channel)
{
    ui->setupUi(this);
}

Channel::~Channel()
{
    ircPart(this);
    delete ui;
}

void Channel::addUser(const QString &user)
{
    ui->listWidget->addItem(user);
}

void Channel::removeUser(const QString &user)
{
    qDeleteAll(ui->listWidget->findItems(user, Qt::MatchExactly));
}

void Channel::addString(const QString &string)
{
    static const QString templ = "[HH:mm:ss] ";
    QTime time = QTime::currentTime();

    ui->textEdit->setTextColor(Qt::white);
    ui->textEdit->append(time.toString(templ) + string);
}

void Channel::addMessage(const QString &source, const QString &message)
{
    static const QString templ = "(%1) %2";
    addString(templ.arg(source, message));
}

void Channel::on_lineEdit_returnPressed()
{
    auto text = ui->lineEdit->text();
    if (text.startsWith('/'))
        ircSendRaw(text.right(text.length() - 1));
    else
        ircSay(this, text);
    ui->lineEdit->clear();
}

void Channel::on_listWidget_currentTextChanged(const QString &currentText)
{
    ui->lineEdit->insert(currentText + (ui->lineEdit->text().isEmpty() ? ", " : " "));
}
