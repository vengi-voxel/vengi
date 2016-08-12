/**
 * @file
 */
#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSet>
#include <QDir>
#include <QSortFilterProxyModel>

#define COMBOBOX_NO_REFRESH (1 << 0)
#define COMBOBOX_DETAILS (1 << 1)

class IComboBox;

class DetailsClickFilter: public QObject {
Q_OBJECT
protected:
	QLabel* _label;
	IComboBox* _comboBox;
	QPoint _clickedPoint;

public:
	DetailsClickFilter(QLabel* label, IComboBox* comboBox);
	bool eventFilter(QObject* watched, QEvent* event) override;

	const QPoint& getClickedPoint () const;
};

class IComboBox: public QWidget {
Q_OBJECT
protected:
	QWidget _container;
	QComboBox _comboBox;
	QSortFilterProxyModel _proxy;
	QHBoxLayout _hLayout;
	QVBoxLayout _vLayout;
	QLabel _details;
	DetailsClickFilter _detailsClickFilter;
	QPushButton *_refresh;
	int _flags;
	QSet<QString> _uniqueSet;
	QString _detailImageSubdir;

private slots:
	void onRefresh();
	void changeDetails();
public:
	IComboBox(const QString& title, const QString& detailImageSubdir, int flags = 0, QWidget* parent = nullptr);
	virtual ~IComboBox();

	// inserts an object and eliminate duplicates, returns true if the item was really inserted
	bool insert(int key, const QString& value);
	void sort();
	void clear();
	// loads the data from the given server. Before calling this manually you should also invoke @c clear
	virtual void load() = 0;
	virtual void onChangeDetails();
	virtual void onClickedDetails(const QPoint& /* pos */) {}
	// returns the id of the current selected item
	int getId() const;
};
