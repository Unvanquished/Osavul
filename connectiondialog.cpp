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

#include "connectiondialog.h"
#include "ui_connectiondialog.h"

ConnectionDialog::ConnectionDialog(QWidget *parent) :
    QDialog(parent), ui(new Ui::ConnectionDialog)
{
    ui->setupUi(this);
    settings.beginGroup("ircConnectionDialog");
    for (QLineEdit *l : { ui->serverHost, ui->serverPort, ui->nickName })
        l->setText(settings.contains(l->objectName())
                   ? settings.value(l->objectName()).toString()
                   : l->text());
}

ConnectionDialog::~ConnectionDialog()
{
    delete ui;
}

QString ConnectionDialog::getHost()
{
    return ui->serverHost->text();
}

quint16 ConnectionDialog::getPort()
{
    return ui->serverPort->text().toUInt();
}

QString ConnectionDialog::getNick()
{
    return ui->nickName->text().toUtf8();
}

void ConnectionDialog::on_buttonBox_accepted()
{
    for (QLineEdit *l : { ui->serverHost, ui->serverPort, ui->nickName })
        settings.setValue(l->objectName(), l->text());
}
