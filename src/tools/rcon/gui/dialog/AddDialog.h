/**
 * @file
 */
#pragma once

#include "IDialog.h"

#include <QLineEdit>
#include <QGroupBox>

class AddDialog: public IDialog {
Q_OBJECT
private:
	QLineEdit *_nameText;
	QLineEdit *_typeText;
	QLineEdit *_conditionText;
	QGroupBox *_group;

	QString _name;
	QString _type;
	QString _condition;

public:
	AddDialog();
	virtual ~AddDialog();

	virtual void onApply() override;
	virtual void addMainWidgets(QBoxLayout& boxLayout) override;

	inline const QString& getName() const {
		return _name;
	}

	inline const QString& getType() const {
		return _type;
	}

	inline const QString& getCondition() const {
		return _condition;
	}
};
