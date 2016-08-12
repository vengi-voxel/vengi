/**
 * @file
 */
#pragma once

#include <QDialog>
#include <QBoxLayout>
#include <QDebug>
#include <QList>

#define DIALOG_NO_APPLY_BUTTON (1 << 0)

class IDialog: public QDialog {
Q_OBJECT
protected:
	QString _title;

	QPushButton *_applyButton;
	QPushButton *_closeButton;
	QHBoxLayout *_buttonLayout;
	QWidget *_buttons;
	QVBoxLayout *_mainLayout;
	int _flags;

private slots:
	void apply() {
		onApply();
		emit accept();
	}

public:
	IDialog(const QString &title, int flags = 0);
	virtual ~IDialog();
	const QString& getTitle() const;

	int run();

	virtual void addButtons(QBoxLayout& layout);
	virtual void onApply() {}
	virtual void addMainWidgets(QBoxLayout& /* layout */) = 0;
};

inline const QString& IDialog::getTitle() const {
	return _title;
}
