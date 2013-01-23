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

#include <QtCore/QLibraryInfo>
#include <QtCore/QTranslator>
#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setOrganizationName("Unvanquished Development");
    QApplication::setOrganizationDomain("unvanquished.net");
    QApplication::setApplicationName("Osavul");
    QApplication::setApplicationVersion("0.1alpha");
    QApplication::setWindowIcon(QIcon(":images/unvanquished_tray_icon.png"));
    QApplication::setQuitOnLastWindowClosed(false);

    QString locale = QLocale::system().name();
    QString localeBase = locale.split("_")[0];

    // load stock translations
    QTranslator qtTranslator;
    QString path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    if (!qtTranslator.load("qt_" + locale, path)) {
        qtTranslator.load("qt_" + localeBase, path);
    }
    a.installTranslator(&qtTranslator);

    // load Osavul's own translations
    QTranslator aTranslator;

    if (!aTranslator.load("osavul_" + locale)) {
        aTranslator.load("osavul_" + localeBase);
    }
    a.installTranslator(&aTranslator);

    MainWindow w;
    w.setWindowTitle("Osavul");
    w.show();

    return a.exec();
}
