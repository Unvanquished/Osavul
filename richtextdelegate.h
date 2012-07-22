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

#ifndef RICHTEXTDELEGATE_H
#define RICHTEXTDELEGATE_H

#include <QStyledItemDelegate>
#include <QLabel>
#include <QTextDocument>
#include <QPainter>
#include <QApplication>

class RichTextDelegate : public QStyledItemDelegate {
public:
    RichTextDelegate(QObject *parent = nullptr);
    Qt::Alignment alignment() { return m_alignment; }
    void setAlignment(Qt::Alignment alignment = Qt::AlignCenter) { m_alignment = alignment; }

protected:
    ~RichTextDelegate();
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    Qt::Alignment m_alignment;
    QLabel *l;
    QTextDocument *doc;
};

#endif // RICHTEXTDELEGATE_H
