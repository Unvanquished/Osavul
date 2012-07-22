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

#include "richtextdelegate.h"

RichTextDelegate::RichTextDelegate(QObject *parent) :
    QStyledItemDelegate(parent) {
    l = new QLabel();
    doc = new QTextDocument(this);
    l->setAttribute(Qt::WA_TranslucentBackground);
}

RichTextDelegate::~RichTextDelegate() {
    delete l;
}

void RichTextDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const {
    QStyleOptionViewItemV4 options = option;
    initStyleOption(&options, index);

    if (options.state & QStyle::State_Selected)
        l->setBackgroundRole(QPalette::Highlight);
    else
        l->setBackgroundRole(QPalette::Base);

    l->setText(options.text);
    options.text.clear();

    l->setAlignment(m_alignment);
    l->setFixedSize(options.rect.size());

    painter->save();

    options.widget->style()->drawControl(QStyle::CE_ItemViewItem, &options, painter);

    painter->translate(options.rect.left(), options.rect.top());
    l->render(painter);

    painter->restore();
}

QSize RichTextDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const {
    QStyleOptionViewItemV4 options = option;
    initStyleOption(&options, index);

    doc->setHtml(options.text);
    return QSize(doc->idealWidth(), doc->size().height());
}
