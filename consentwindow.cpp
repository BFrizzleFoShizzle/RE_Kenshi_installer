#include "consentwindow.h"
#include "ui_consentwindow.h"

#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDialogButtonBox>

#include "mainwindow.h"

static std::vector<QString> consentResources = { ":/conditions/PRIVACY.md",
											   ":/conditions/LICENSE" };
static int consentStep = 0;

static QString GetTranslatedResource(const QString &resource)
{
	QString path = QString(resource);
	QFile f(path);
	if (!f.exists())
	{
		QMessageBox m;
		m.setText("Could not find " + resource);
	}
	f.open(QIODevice::ReadOnly | QIODevice::Text);
	QTextStream in(&f);
	in.setCodec("UTF-8");
	return in.readAll();
}

ConsentWindow::ConsentWindow(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConsentWindow)
{
	QString language = QLocale::system().name().mid(0,2);
	baseTranslator.load("./translations/qt_" + language);
	QApplication::instance()->installTranslator(&baseTranslator);
	mainTranslator.load("./translations/RE_Kenshi_" + language);
	QApplication::instance()->installTranslator(&mainTranslator);
	ui->setupUi(this);
	ui->comboBox->addItem("English", "en");
	ui->comboBox->addItem("Deutsch", "de");
	ui->comboBox->addItem("Русский", "ru");
	ui->comboBox->addItem("日本語", "ja");
	ui->comboBox->addItem("Français", "fr");
	if(ui->comboBox->findData(language) != -1)
		ui->comboBox->setCurrentIndex(ui->comboBox->findData(language));

	ui->textEdit->setText(GetTranslatedResource(consentResources[consentStep]));

	ui->buttonBox->addButton(tr("I accept"), QDialogButtonBox::ActionRole);
	ui->buttonBox->addButton(QDialogButtonBox::StandardButton::Cancel);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

void ConsentWindow::on_comboBox_currentIndexChanged(int index)
{
	QString language = QString(ui->comboBox->currentData().toString());
	QApplication::instance()->removeTranslator(&baseTranslator);
	baseTranslator.load("./translations/qt_" + language);
	QApplication::instance()->installTranslator(&baseTranslator);

	QApplication::instance()->removeTranslator(&mainTranslator);
	mainTranslator.load("./translations/RE_Kenshi_" + language);
	QApplication::instance()->installTranslator(&mainTranslator);

	// make resource lookups follow the same language
	QLocale::setDefault(QLocale(language));

	ui->retranslateUi(this);
	// Apparently, have to do this manually...
	ui->textEdit->setText(GetTranslatedResource(consentResources[consentStep]));

}

ConsentWindow::~ConsentWindow()
{
	delete ui;
}

void ConsentWindow::on_buttonBox_clicked(QAbstractButton *button)
{
	if(ui->buttonBox->buttonRole(button) == QDialogButtonBox::ActionRole)
	{
		++consentStep;
		if(consentStep >= consentResources.size())
		{
			this->hide();
			MainWindow* m = new MainWindow(this);
			m->show();
		}
		else
		{
			ui->textEdit->setText(GetTranslatedResource(consentResources[consentStep]));
		}
	}
}

