#pragma once

#include <QSettings>

class Settings {
private:
	QSettings _settings;
	Settings() :
			_settings("simpleai", "simpleai") {
	}

public:
	static QSettings& getSettings() {
		static Settings s;
		return s._settings;
	}

	static void setIfAbsent(const QString& key, const QString& value) {
		if (getSettings().allKeys().contains(key))
			return;
		getSettings().setValue(key, value);
	}

};
