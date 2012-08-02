#include "favoritesdialog.h"
#include "ui_favoritesdialog.h"

FavoritesDialog::FavoritesDialog(QList<unv::FavoriteEntry> &favorites, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FavoritesDialog)
{
    ui->setupUi(this);

    // needs additional work
    //    ui->favoritesTable->addAction(ui->actionAdd);
    ui->favoritesTable->addAction(ui->actionDelete);

    for (unv::FavoriteEntry &fav : favorites) {
        int row;
        ui->favoritesTable->insertRow(row = ui->favoritesTable->rowCount());

        QVariant favV = QVariant::fromValue<unv::FavoriteEntry *>(&fav);
        for (int i = 0; i < 2; ++i) {
            auto it = new QTableWidgetItem();
            it->setData(Qt::UserRole, favV);
            it->setText(i == 0 ? fav.name : fav.host);
            ui->favoritesTable->setItem(row, i, it);
        }


        auto portItem = new QTableWidgetItem();
        portItem->setData(Qt::EditRole, fav.port);
        portItem->setData(Qt::UserRole, favV);
        ui->favoritesTable->setItem(row, 2, portItem);
    }

    ui->favoritesTable->resizeColumnsToContents();
}

FavoritesDialog::~FavoritesDialog()
{
    delete ui;
}

void FavoritesDialog::on_favoritesTable_itemChanged(QTableWidgetItem *item)
{
    if (!item)
        return;

    if (!item->isSelected())
        return;

    unv::FavoriteEntry *fav = item->data(Qt::UserRole).value<unv::FavoriteEntry *>();
    if (!fav)
        return;

    switch (item->column()) {
        case 0:
            fav->name = item->text();
            break;
        case 1:
            fav->host = item->text();
            break;
        case 2:
            fav->port = item->text().toUInt();
            break;
        default:
            Q_ASSERT(false);
    }

    QPair<int, unv::FavoriteEntry> pair(item->row(), *fav);

    for (int i = 0; i < m_modifiedFavorites.length(); ++i) {
        const QPair<int, unv::FavoriteEntry> &x = m_modifiedFavorites.at(i);
        if (x == pair)
            m_modifiedFavorites.removeAt(i);
    }

    m_modifiedFavorites << pair;
}

void FavoritesDialog::on_actionAdd_triggered()
{
    ui->favoritesTable->insertRow(ui->favoritesTable->rowCount());
    ui->favoritesTable->editItem(ui->favoritesTable->item(ui->favoritesTable->rowCount() - 1, 0));
}

void FavoritesDialog::on_actionDelete_triggered()
{
    QTableWidgetItem *current = ui->favoritesTable->currentItem();
    if (!current)
        return;

    int row = current->row();
    for (int i = 0; i < 2; ++i)
        delete ui->favoritesTable->item(row, i);

    ui->favoritesTable->removeRow(row);
}
