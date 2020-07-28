/**
 * @file
 */

#include "tb_widget_value.h"
#include "tb_widgets.h"

namespace tb {

void TBWidgetValueConnection::connect(TBWidgetValue *value, TBWidget *widget) {
	unconnect();
	m_widget = widget;
	m_value = value;
	m_value->m_connections.addLast(this);
	m_value->syncToWidget(m_widget);
}

void TBWidgetValueConnection::unconnect() {
	if (m_value != nullptr) {
		m_value->m_connections.remove(this);
	}
	m_value = nullptr;
	m_widget = nullptr;
}

void TBWidgetValueConnection::syncFromWidget(TBWidget *sourceWidget) {
	if (m_value != nullptr) {
		m_value->setFromWidget(sourceWidget);
	}
}

TBWidgetValue::TBWidgetValue(const TBID &name, TBValue::TYPE type) : m_name(name), m_value(type), m_syncing(false) {
}

TBWidgetValue::~TBWidgetValue() {
	while (m_connections.getFirst() != nullptr) {
		m_connections.getFirst()->unconnect();
	}
}

void TBWidgetValue::setFromWidget(TBWidget *sourceWidget) {
	if (m_syncing) {
		return; // We ended up here because syncing is in progress.
	}

	// Get the value in the format
	core::String text;
	switch (m_value.getType()) {
	case TBValue::TYPE_STRING:
		if (!sourceWidget->getText(text)) {
			return;
		}
		m_value.setString(text, TBValue::SET_NEW_COPY);
		break;
	case TBValue::TYPE_NULL:
	case TBValue::TYPE_INT:
		m_value.setInt(sourceWidget->getValue());
		break;
	case TBValue::TYPE_FLOAT:
		// FIX: TBValue should use double instead of float?
		m_value.setFloat((float)sourceWidget->getValueDouble());
		break;
	default:
		core_assert(!"Unsupported value type!");
	}

	syncToWidgets(sourceWidget);
}

bool TBWidgetValue::syncToWidgets(TBWidget *excludeWidget) {
	// FIX: Assign group to each value. Currently we only have one global group.
	g_value_group.invokeOnValueChanged(this);

	bool fail = false;
	TBLinkListOf<TBWidgetValueConnection>::Iterator iter = m_connections.iterateForward();
	while (TBWidgetValueConnection *connection = iter.getAndStep()) {
		if (connection->m_widget != excludeWidget) {
			fail |= !syncToWidget(connection->m_widget);
		}
	}
	return !fail;
}

bool TBWidgetValue::syncToWidget(TBWidget *dstWidget) {
	if (m_syncing) {
		return true; // We ended up here because syncing is in progress.
	}

	m_syncing = true;
	bool ret = true;
	switch (m_value.getType()) {
	case TBValue::TYPE_STRING:
		ret = dstWidget->setText(m_value.getString());
		break;
	case TBValue::TYPE_NULL:
	case TBValue::TYPE_INT:
		dstWidget->setValue(m_value.getInt());
		break;
	case TBValue::TYPE_FLOAT:
		// FIX: TBValue should use double instead of float?
		dstWidget->setValueDouble(m_value.getFloat());
		break;
	default:
		core_assert(!"Unsupported value type!");
	}
	m_syncing = false;
	return ret;
}

void TBWidgetValue::setInt(int value) {
	m_value.setInt(value);
	syncToWidgets(nullptr);
}

bool TBWidgetValue::setText(const char *text) {
	m_value.setString(text, TBValue::SET_NEW_COPY);
	return syncToWidgets(nullptr);
}

void TBWidgetValue::setDouble(double value) {
	// FIX: TBValue should use double instead of float?
	m_value.setFloat((float)value);
	syncToWidgets(nullptr);
}

// == TBValueGroup ================================================================================

/*extern*/ TBValueGroup g_value_group;

TBWidgetValue *TBValueGroup::createValueIfNeeded(const TBID &name, TBValue::TYPE type) {
	if (TBWidgetValue *val = getValue(name)) {
		return val;
	}
	if (TBWidgetValue *val = new TBWidgetValue(name, type)) {
		if (m_values.add(name, val)) {
			return val;
		}
		{ delete val; }
	}
	return nullptr;
}

void TBValueGroup::invokeOnValueChanged(const TBWidgetValue *value) {
	TBLinkListOf<TBValueGroupListener>::Iterator iter = m_listeners.iterateForward();
	while (TBValueGroupListener *listener = iter.getAndStep()) {
		listener->onValueChanged(this, value);
	}
}

} // namespace tb
