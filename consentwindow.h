#ifndef CONSENTWINDOW_H
#define CONSENTWINDOW_H

#include <QDialog>
#include <QAbstractButton>
#include <QTranslator>

namespace Ui {
class ConsentWindow;
}

class ConsentWindow : public QDialog
{
	Q_OBJECT

public:
	explicit ConsentWindow(QWidget *parent = nullptr);
	~ConsentWindow();

private slots:
	void on_comboBox_currentIndexChanged(int index);

	void on_buttonBox_clicked(QAbstractButton *button);

private:
	Ui::ConsentWindow *ui;

	QTranslator mainTranslator;
	QTranslator baseTranslator;
};

#endif // CONSENTWINDOW_H
