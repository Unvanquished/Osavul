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

#ifndef CHANNEL_H
#define CHANNEL_H

#include <QWidget>
#include <QListWidgetItem>
#include <QTime>

namespace Ui {
    class Channel;
}

class Channel : public QWidget
{
    Q_OBJECT

public:
    explicit Channel(QWidget *parent = nullptr, const QString &channel = "#unv-lobby");
    ~Channel();

    const QString &channel() const { return m_channel; }

public slots:
    void addUser(const QString &user);
    void removeUser(const QString &user);
    void addString(const QString &string);
    void addMessage(const QString &source, const QString &message);

private slots:
    void on_lineEdit_returnPressed();
    void on_listWidget_currentTextChanged(const QString &currentText);

signals:
    void ircSay(Channel *, const QString &text);
    void ircJoin(Channel *);
    void ircPart(Channel *);

private:
    Ui::Channel *ui;
    const QString m_channel;
};

#endif // CHANNEL_H
