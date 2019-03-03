/**
 * @file
 */

#pragma once

#include <QtGui>
#include <QLabel>
#include <QToolBar>
#include <QStatusBar>
#include <QComboBox>
#include <QMenu>
#include <QLineEdit>
#include <QTreeView>

#include "BehaviourTreeModel.h"
#include "AICompressorProxy.h"
#include "AINodeStaticResolver.h"
#include "BehaviourTreeModelItem.h"

namespace ai {
namespace debug {

class StateTable;
class EntityList;
class AggroTable;
class NodeTreeView;
class MapView;
class AIDebugger;

/**
 * @brief The widget that represents the whole ai debugger
 */
class AIDebuggerWidget: public QWidget {
Q_OBJECT
public:
	/**
	 * @param[in] standalone If this is @c true, the widget will e.g. contribute a quit action to the file menu.
	 * If this false, the widget will behave as if it would be part of an already existing appliation.
	 */
	AIDebuggerWidget(AIDebugger& debugger, AINodeStaticResolver& resolver, bool standalone);
	virtual ~AIDebuggerWidget();

	void connectToAIServer(const QString& hostname, short port);

	/**
	 * @brief Call this with an existing @c QStatusBar to add additional ai debugger information to it
	 */
	void contributeToStatusBar(QStatusBar* statusBar);
	/**
	 * @brief Call this with an existing @c QToolbar to add addition ai debugger buttons to it
	 */
	void contributeToToolBar(QToolBar* toolBar);
	/**
	 * @brief Call this with an existing @c QToolbar to add addition ai debugger entries to it
	 */
	void contributeToFileMenu(QMenu *fileMenu);
	/**
	 * @brief Call this with an existing @c QToolbar to add addition ai debugger entries to it
	 */
	void contributeToHelpMenu(QMenu *helpMenu);
	/**
	 * @brief Call this with an existing @c QToolbar to add addition ai debugger entries to it
	 */
	void contributeToSettingsMenu(QMenu *settingsMenu);

	/**
	 * @brief If you let the ai debugger contribute to the status bar, call if to remove the contribution
	 */
	void removeFromStatusBar(QStatusBar* statusBar);
	/**
	 * @brief If you let the ai debugger contribute to the tool bar, call if to remove the contribution
	 */
	void removeFromToolBar(QToolBar* toolBar);
	/**
	 * @brief If you let the ai debugger contribute to the file menu, call if to remove the contribution
	 */
	void removeFromFileMenu(QMenu *fileMenu);
	/**
	 * @brief If you let the ai debugger contribute to the help menu, call if to remove the contribution
	 */
	void removeFromHelpMenu(QMenu *helpMenu);
	/**
	 * @brief If you let the ai debugger contribute to the settings menu, call if to remove the contribution
	 */
	void removeFromSettingsMenu(QMenu *settingsMenu);

private slots:
	void about();
	void documentation();
	void bug();
	void settings();
	void toggleTreeView();
	void connectToAIServer();
	void disconnectFromAIServer();
	void quitApplication();
	void requestPause();
	void requestStep();
	void requestReset();
	void setPause(bool pause);
	void change(const QString &);
	void onNamesReceived();
	void onDisconnect();
	void onEntitiesUpdated();
	void onSelected();
	void showContextMenu(const QPoint& contextMenuPos);
	void onDeleteNode(int nodeId);
	void onAddNode(int parentNodeId, const QVariant& name, const QVariant& type, const QVariant& condition);

private:
	void createView();
	void createActions();

	QWidget *createTopWidget();
	QWidget *createBottomWidget();
	QWidget *createTreePanelWidget();
	QLabel *createLabel(const QString &text) const;

	NodeTreeView *_nodeTree;
	StateTable *_stateTable;
	MapView *_mapWidget;
	EntityList *_entityList;
	QLineEdit *_entityFilter;
	AggroTable *_aggroTable;
	QAction *_connectAction;
	QAction *_disconnectAction;
	QAction *_pauseAction;
	QAction *_quitAction;
	QAction *_stepAction;
	QAction *_resetAction;
	QAction *_aboutAction;
	QAction *_documentationAction;
	QAction *_bugAction;
	QAction *_settingsAction;
	QLabel *_statusBarLabel;
	QLabel *_selectedLabel;
	QComboBox *_namesComboBox;
	QTreeView *_tree;
	AINodeStaticResolver& _resolver;
	BehaviourTreeModel _model;

	AIDebugger& _debugger;
	QString _name;
	CompressorProxy _proxy;
	bool _standalone;
};

}
}
