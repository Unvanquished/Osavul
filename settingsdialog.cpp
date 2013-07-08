#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "utils.h"

using namespace Settings;

PreferredIPVersion SettingsDialog::getPreferredIPSetting()
{
    if (ui->preferIPv4->isChecked()) return preferredIPVersion_v4;
    if (ui->preferIPv6->isChecked()) return preferredIPVersion_v6;
    return preferredIPVersion_any;
}

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

    PreferredIPVersion which(settings.value("unv/preferredIPVersion", int(getPreferredIPSetting())).toInt());

    ui->preferIPv4->setChecked(which.v4());
    ui->preferIPv6->setChecked(which.v6());
    ui->preferIPAny->setChecked(which.any());
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
    settings.setValue("unv/preferredIPVersion", int(getPreferredIPSetting()));
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
