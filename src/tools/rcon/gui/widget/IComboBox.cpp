/**
 * @file
 */
#include "IComboBox.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QAbstractItemView>

DetailsClickFilter::DetailsClickFilter(QLabel* label, IComboBox* comboBox) :
		_label(label), _comboBox(comboBox) {
}
bool DetailsClickFilter::eventFilter(QObject* watched, QEvent* mouseButtonPressEvent) {
	if (watched != _label) {
		return false;
	}
	if (mouseButtonPressEvent->type() != QEvent::MouseButtonPress) {
		return false;
	}
	const QMouseEvent* const me = static_cast<const QMouseEvent*>(mouseButtonPressEvent);
	_clickedPoint = me->pos();

	_comboBox->onChangeDetails();

	if (_label->pixmap() == nullptr) {
		return false;
	}

	QImage tmp(_label->pixmap()->toImage());
	QPainter painter(&tmp);
	QPen paintpen(Qt::red);
	paintpen.setWidth(4);
	painter.setPen(paintpen);
	painter.drawPoint(_clickedPoint);
	_label->setPixmap(QPixmap::fromImage(tmp));
	_comboBox->onClickedDetails(_clickedPoint);
	return false;
}

const QPoint& DetailsClickFilter::getClickedPoint() const {
	return _clickedPoint;
}

IComboBox::IComboBox(const QString& title, const QString& detailImageSubdir, int flags, QWidget* objParent) :
		QWidget(objParent), _proxy(&_comboBox), _detailsClickFilter(&_details, this),
		_flags(flags), _detailImageSubdir(detailImageSubdir) {
	setLayout(&_vLayout);
	if ((_flags & COMBOBOX_DETAILS) != 0) {
		connect(&_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeDetails()));
		_vLayout.addWidget(&_details);
		_details.installEventFilter(&_detailsClickFilter);
	}
	_vLayout.addWidget(&_container);
	_container.setLayout(&_hLayout);
	_hLayout.addWidget(&_comboBox, 1);
#if 0
	QSizePolicy sp = _comboBox.view()->sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::Minimum);
	_comboBox.view()->setSizePolicy(sp);
#endif
	_comboBox.setDuplicatesEnabled(false);
	_comboBox.setEditable(true);
	_proxy.setSourceModel(_comboBox.model());
	_comboBox.model()->setParent(&_proxy);
	_comboBox.setModel(&_proxy);
	_refresh = new QPushButton(QIcon(":/images/refresh.png"), "");
	_refresh->setToolTip(title);
	if ((_flags & COMBOBOX_NO_REFRESH) == 0) {
		connect(_refresh, SIGNAL(clicked()), this, SLOT(onRefresh()));
		_hLayout.addWidget(_refresh);
	}
}

IComboBox::~IComboBox() {
	delete _refresh;
}

void IComboBox::clear() {
	_comboBox.clear();
	_uniqueSet.clear();
}

void IComboBox::changeDetails() {
	onChangeDetails();
}

void IComboBox::sort() {
	_comboBox.model()->sort(0);
}

bool IComboBox::insert(int key, const QString& value) {
	const QString unique = QString::number(key) + "-" + value;
	if (!_uniqueSet.contains(unique)) {
		_comboBox.addItem(value, QVariant(key));
		_uniqueSet.insert(unique);
		return true;
	}
	return false;
}

void IComboBox::onRefresh() {
	clear();
	load();
	sort();
}

void IComboBox::onChangeDetails() {
	if (_detailImageSubdir.isEmpty()) {
		return;
	}
	const int id = getId();
	const QString& imagePath(_detailImageSubdir + QDir::separator() + id + ".png");
	const QPixmap pixmap(imagePath);
	_details.setPixmap(pixmap);
}

int IComboBox::getId() const {
	if (_comboBox.count() == 0 || _comboBox.currentIndex() == -1) {
		return -1;
	}
	const int id = _comboBox.itemData(_comboBox.currentIndex()).toString().toInt();
	return id;
}
