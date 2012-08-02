#ifndef FAVORITESDIALOG_H
#define FAVORITESDIALOG_H

#include "unv.h"
#include <QtGui/QDialog>
#include <QtGui/QTableWidgetItem>

namespace Ui {
    class FavoritesDialog;
}

class FavoritesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FavoritesDialog(QList<unv::FavoriteEntry> &favorites, QWidget *parent = 0);
    ~FavoritesDialog();
    const QList< QPair<int, unv::FavoriteEntry> > &modifiedFavorites() const
        { return m_modifiedFavorites; }

private slots:
    void on_favoritesTable_itemChanged(QTableWidgetItem *item);
    void on_actionAdd_triggered();
    void on_actionDelete_triggered();

private:
    Ui::FavoritesDialog *ui;
    QList< QPair<int, unv::FavoriteEntry> > m_modifiedFavorites;
};

#endif // FAVORITESDIALOG_H
