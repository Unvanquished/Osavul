#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->ircConnectOnStartupCheckBox->setChecked(settings.value("irc/connectOnStartup").toBool());
    ui->daemonPathLineEdit->setText(settings.value("unv/clientExecutablePath",
                                                   ui->daemonPathLineEdit->text()).toString());
    ui->braveUserCheckBox->setChecked(settings.value("braveUser",
                                                     ui->braveUserCheckBox->isChecked()).toBool());
    ui->masterServerLineEdit->setText(settings.value("unv/masterServerAddress",
                                                     ui->masterServerLineEdit->text()).toString());
    ui->portSpinBox->setValue(settings.value("unv/masterServerPort",
                                             ui->portSpinBox->value()).toUInt());
    ui->protocolSpinBox->setValue(settings.value("unv/protocolNumber",
                                                 ui->protocolSpinBox->value()).toUInt());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_buttonBox_accepted()
{
    settings.setValue("irc/connectOnStartup", ui->ircConnectOnStartupCheckBox->isChecked());
    settings.setValue("unv/clientExecutablePath", ui->daemonPathLineEdit->text());
    settings.setValue("braveUser", ui->braveUserCheckBox->isChecked());
    settings.setValue("unv/masterServerAddress", ui->masterServerLineEdit->text());
    settings.setValue("unv/masterServerPort", ui->portSpinBox->value());
    settings.setValue("unv/protocolNumber", ui->protocolSpinBox->value());
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
