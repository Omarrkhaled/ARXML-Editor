// main_window.hpp
//
// Declares the MainWindow class, which acts as the controller and
// coordinator for the ARXML editor application. The window manages
// user interactions, delegates document operations to ArxmlModel, and
// invokes helpers such as ArxmlValidator, ArxmlNamingChecker and
// ArxmlDiagramGenerator. The UI consists of a tree view for
// navigating the document structure and a property table for
// inspecting and editing element attributes.

#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QDomNode>
#include <QVariant>

// Forward declarations for Qt widgets
class QTreeWidget;
class QTableWidget;
class QPushButton;
class QTreeWidgetItem;
class QTableWidgetItem;

// Forward declarations for helper classes
class ArxmlModel;
class ArxmlValidator;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // File operations
    void openFile();
    void saveFile();
    void saveFileAs();

    // Tree selection
    void onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    // Property table edits
    void onPropertyItemChanged(QTableWidgetItem *item);

    // Context menu actions
    void showContextMenu(const QPoint &pos);
    void addChildElement();
    void deleteElement();

    // Validation, naming check and diagram generation
    void validateDocument();

private:
    // Build the tree view from the model's document
    void populateTree();
    // Populate the property table based on the selected node
    void populatePropertyTable(const QDomNode &node);
    // Update the text shown in the tree item's value column when editing
    void updateTreeItemText(QTreeWidgetItem *item, const QDomNode &node);

    // UI members
    QTreeWidget *m_treeWidget;
    QTableWidget *m_propertyTable;
    QPushButton *m_openButton;
    QPushButton *m_saveButton;
    QPushButton *m_saveAsButton;
    QPushButton *m_validateButton;
    QPushButton *m_checkButton;
    QPushButton *m_diagramButton;

    // State and helpers
    ArxmlModel *m_model;
    ArxmlValidator *m_validator;
    QString m_currentFileName;
    QString m_schemaFileName;
};

#endif // MAIN_WINDOW_HPP