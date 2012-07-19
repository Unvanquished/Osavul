#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    ui->ircConnectOnStartupCheckBox->setChecked(settings.value("ircConnectOnStartup").toBool());
    ui->daemonPathLineEdit->setText(settings.value("unv/clientExecutablePath").toString());
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
