#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->ircConnectOnStartupCheckBox->setChecked(settings.value("ircConnectOnStartup").toBool());
    ui->daemonPathLineEdit->setText(settings.value("unv/clientExecutablePath").toString());
    ui->daemonPathLineEdit->adjustSize();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_buttonBox_accepted()
{
    settings.setValue("irc/connectOnStartup", ui->ircConnectOnStartupCheckBox->isChecked());
    settings.setValue("unv/clientExecutablePath", ui->daemonPathLineEdit->text());
}

void SettingsDialog::on_daemonPathBrowseButton_clicked()
{
    QString path =
        QFileDialog::getOpenFileName(this,
                                     "Path to Daemon",
                                     "",
                                     "");

    ui->daemonPathLineEdit->setText(path);
    ui->daemonPathLineEdit->adjustSize();
}
