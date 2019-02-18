/**
 * @file
 */

#pragma once

#include "tb_core.h"
#include "tb_hashtable.h"
#include "tb_id.h"
#include "tb_linklist.h"
#include "tb_value.h"

namespace tb {

class TBWidget;
class TBWidgetValue;
class TBValueGroup;

/** TBWidgetValueConnection maintains a connection between TBWidget and TBWidgetValue. */

class TBWidgetValueConnection : public TBLinkOf<TBWidgetValueConnection> {
public:
	TBWidgetValueConnection() {
	}
	~TBWidgetValueConnection() {
		unconnect();
	}

	/** Connect the value and widget. */
	void connect(TBWidgetValue *value, TBWidget *m_widget);

	/** Unconnect the value and widget if it is connected. */
	void unconnect();

	/** Synchronize the value of the widget to the TBWidgetValue and all other
		connected widgets. */
	void syncFromWidget(TBWidget *source_widget);

private:
	friend class TBWidgetValue;
	TBWidgetValue *m_value = nullptr;
	TBWidget *m_widget = nullptr;
};

/** TBWidgetValue stores a TBValue that will be synchronized with all widgets connected to it.
	It has a TBID name, that can be used to identify this value within its TBValueGroup.

	It will synchronize with widgets when any of the connected widgets change and trig the
	EVENT_TYPE_CHANGED event, and when the value is changed with any of the setters here.

	The synchronization with widgets is done through the generic TBWidget setters/getters,
	TBWidget::setValue/getValue/setValueDouble/getValueDouble/getText/setText.

	The type that is synchronized is determined by the TBValue::TYPE specified in the
	constructor.

	Note: The type that is synchronized changes if you request it in a different format!
*/
class TBWidgetValue {
public:
	TBWidgetValue(const TBID &name, TBValue::TYPE type = TBValue::TYPE_INT);
	~TBWidgetValue();

	/** Set integer value and sync to connected widgets. */
	void setInt(int value);

	/** Set text value and sync to connected widgets. */
	bool setText(const char *text);

	/** Set double value and sync to connected widgets. */
	void setDouble(double value);

	/** Set the value from the given widget. Using the current format type.*/
	void setFromWidget(TBWidget *source_widget);

	/** Get value as integer. */
	int getInt() const {
		return m_value.getInt();
	}

	/** Get value as text. Return false on fail. */
	bool getText(TBStr &text) {
		return text.set(m_value.getString());
	}

	/** Get value as text. */
	TBStr getText() {
		TBStr text;
		getText(text);
		return text;
	}

	/** Get the value as double. */
	double getDouble() const {
		return m_value.getFloat();
	}

	/** Get the TBValue used to store the value. */
	const TBValue &getValue() const {
		return m_value;
	}

	/** Get the name id. */
	TBID getName() const {
		return m_name;
	}

private:
	friend class TBWidgetValueConnection;
	TBID m_name;
	TBValue m_value;
	TBLinkListOf<TBWidgetValueConnection> m_connections;
	bool m_syncing;

	bool syncToWidget(TBWidget *dst_widget);
	bool syncToWidgets(TBWidget *exclude_widget);
};

/** Listener that will be notified when any of the values in a TBValueGroup is changed. */
class TBValueGroupListener : public TBLinkOf<TBValueGroupListener> {
public:
	virtual ~TBValueGroupListener() {
		if (linklist)
			linklist->remove(this);
	}

	/** Called when a value has changed and all widgets connected to it has been updated. */
	virtual void onValueChanged(const TBValueGroup *group, const TBWidgetValue *value) = 0;
};

/** TBValueGroup is a collection of widget values (TBWidgetValue) that can be fetched
	by name (using a TBID). It also keeps a list of TBValueGroupListener that listens to
	changes to any of the values. */
class TBValueGroup {
public:
	/** Create a TBWidgetValue with the given name if it does not already exist.
		Returns nullptr if out of memory. */
	TBWidgetValue *createValueIfNeeded(const TBID &name, TBValue::TYPE type = TBValue::TYPE_INT);

	/** Get the TBWidgetValue with the given name, or nullptr if no match is found. */
	TBWidgetValue *getValue(const TBID &name) const {
		return m_values.get(name);
	}

	/** Add listener to this group. It will be removed automatically when deleted,
		but can also be removed by RemoveListener. */
	void AddListener(TBValueGroupListener *listener) {
		m_listeners.addLast(listener);
	}

	/** Remove listener from this group. */
	void removeListener(TBValueGroupListener *listener) {
		m_listeners.remove(listener);
	}

private:
	friend class TBWidgetValue;
	void invokeOnValueChanged(const TBWidgetValue *value);

	TBHashTableAutoDeleteOf<TBWidgetValue> m_values; ///< Hash table of values
	TBLinkListOf<TBValueGroupListener> m_listeners;  ///< List of listeners
};

/** The global value group. */
extern TBValueGroup g_value_group;

} // namespace tb
