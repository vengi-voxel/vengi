#include "AddDialog.h"
#include <QGridLayout>
#include <QLabel>

AddDialog::AddDialog() :
		IDialog(tr("Create new node")), _nameText(nullptr), _typeText(nullptr), _conditionText(nullptr), _group(
				nullptr) {
}

AddDialog::~AddDialog() {
	delete _nameText;
	delete _typeText;
	delete _conditionText;
	delete _group;
}

void AddDialog::addMainWidgets(QBoxLayout& layout) {
	_group = new QGroupBox(tr("Node"));
	QGridLayout *innerLayout = new QGridLayout;
	_nameText = new QLineEdit("NewNode");
	innerLayout->addWidget(new QLabel(tr("Name")), 0, 0);
	innerLayout->addWidget(_nameText, 0, 1);
	_typeText = new QLineEdit("PrioritySelector");
	innerLayout->addWidget(new QLabel(tr("Type")), 1, 0);
	innerLayout->addWidget(_typeText, 1, 1);
	_conditionText = new QLineEdit("True");
	innerLayout->addWidget(new QLabel(tr("Condition")), 2, 0);
	innerLayout->addWidget(_conditionText, 2, 1);
	_group->setLayout(innerLayout);
	layout.addWidget(_group);
}

void AddDialog::onApply() {
	_condition = _conditionText->text();
	_name = _nameText->text();
	_type = _typeText->text();

	close();
}
