#pragma once

#include <QSettings>

class Settings {
private:
	QSettings _settings;

public:
	static QSettings& getSettings() {
		static Settings s;
		return s._settings;
	}

	static QString getHostname(const QString& defaultVal) {
		return getSettings().value("connect/hostname", defaultVal).toString();
	}

	static int getPort(int defaultVal) {
		return getSettings().value("connect/port", defaultVal).toInt();
	}

	static void setHostname(const QString& val) {
		getSettings().setValue("connect/hostname", val);
	}

	static void setPort(int val) {
		getSettings().setValue("connect/port", val);
	}

	static int getGridInterval(int defaultVal = 100) {
		return getSettings().value("mapview/gridinterval", defaultVal).toInt();
	}

	static void setGridInterval(int val) {
		getSettings().setValue("mapview/gridinterval", val);
	}

	static float getItemSize(float defaultVal = 30.0) {
		return getSettings().value("mapview/itemsize", defaultVal).toFloat();
	}

	static void setItemSize(float val) {
		getSettings().setValue("mapview/itemsize", val);
	}

	static QColor getGridColor(const QColor& defaultVal = QColor(80, 80, 80)) {
		return getSettings().value("mapview/gridcolor", defaultVal).value<QColor>();
	}

	static QColor getBackgroundColor(const QColor& defaultVal = QColor(50, 50, 50)) {
		return getSettings().value("mapview/bgcolor", defaultVal).value<QColor>();
	}

	static QColor getNameColor(const QColor& defaultVal = QColor(255, 255, 255)) {
		return getSettings().value("mapview/namecolor", defaultVal).value<QColor>();
	}

	static bool getGrid(bool defaultVal = true) {
		return getSettings().value("mapview/showgrid", defaultVal).toBool();
	}

	static QString getNameAttribute(const QString& defaultVal) {
		return getSettings().value("mapview/nameattribute", defaultVal).toString();
	}

	static void setNameAttribute(const QString& attribute) {
		return getSettings().setValue("mapview/nameattribute", attribute);
	}

	static QString getGroupAttribute(const QString& defaultVal) {
		return getSettings().value("mapview/groupattribute", defaultVal).toString();
	}

	static void setGroupAttribute(const QString& attribute) {
		return getSettings().setValue("mapview/groupattribute", attribute);
	}

	static bool getCenterOnSelection(bool defaultVal = false) {
		return getSettings().value("mapview/centeronselection", defaultVal).toBool();
	}

	static void setCenterOnSelection(bool val) {
		getSettings().setValue("mapview/centeronselection", val);
	}

	static void setGridColor(const QColor& val) {
		getSettings().setValue("mapview/gridcolor", val);
	}

	static void setBackgroundColor(const QColor& val) {
		getSettings().setValue("mapview/bgcolor", val);
	}

	static void setNameColor(const QColor& val) {
		getSettings().setValue("mapview/namecolor", val);
	}

	static void setGrid(bool val) {
		getSettings().setValue("mapview/showgrid", val);
	}
};
