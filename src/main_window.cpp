// main_window.cpp
//
// Defines MainWindow, which ties together the user interface and the
// underlying ARXML document operations. The window sets up the
// navigation tree and property editor, responds to user actions such
// as opening or saving files, and delegates document manipulation to
// helper classes. The separation of concerns between the view
// (widgets) and the model (ArxmlModel) makes it easy to adjust
// functionality without entangling UI and data logic.

#include "main_window.hpp"

#include "arxml_model.hpp"
#include "arxml_validator.hpp"

#include <QTreeWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QHeaderView>
#include <QMenu>
#include <QInputDialog>
#include <QDialog>
#include <QLabel>
#include <QPixmap>
#include <QDir>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_treeWidget(new QTreeWidget),
      m_propertyTable(new QTableWidget),
      m_openButton(new QPushButton(tr("Open"))),
      m_saveButton(new QPushButton(tr("Save"))),
      m_saveAsButton(new QPushButton(tr("Save As"))),
      m_validateButton(new QPushButton(tr("Validate"))),
      m_model(new ArxmlModel),
      m_validator(new ArxmlValidator)
{
    // Central widget and layout
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Top row of buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_openButton);
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_saveAsButton);
    buttonLayout->addWidget(m_validateButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Splitter for tree and property views
    QSplitter *splitter = new QSplitter;
    splitter->setOrientation(Qt::Horizontal);

    // Configure tree widget
    m_treeWidget->setHeaderLabels(QStringList() << tr("Element") << tr("Value"));
    m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    splitter->addWidget(m_treeWidget);

    // Configure property table
    m_propertyTable->setColumnCount(2);
    m_propertyTable->setHorizontalHeaderLabels(QStringList() << tr("Field") << tr("Value"));
    m_propertyTable->horizontalHeader()->setStretchLastSection(true);
    m_propertyTable->verticalHeader()->setVisible(false);
    // Allow editing of value cells only; row labels remain read-only
    m_propertyTable->setEditTriggers(QAbstractItemView::AllEditTriggers);
    splitter->addWidget(m_propertyTable);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    // Connect UI signals to slots
    connect(m_openButton, &QPushButton::clicked, this, &MainWindow::openFile);
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::saveFile);
    connect(m_saveAsButton, &QPushButton::clicked, this, &MainWindow::saveFileAs);
    connect(m_validateButton, &QPushButton::clicked, this, &MainWindow::validateDocument);
    connect(m_treeWidget, &QTreeWidget::currentItemChanged,
            this, &MainWindow::onCurrentItemChanged);
    connect(m_propertyTable, &QTableWidget::itemChanged,
            this, &MainWindow::onPropertyItemChanged);
    // Context menu on tree
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeWidget, &QTreeWidget::customContextMenuRequested,
            this, &MainWindow::showContextMenu);

    // Basic window settings
    resize(900, 600);
    setWindowTitle(tr("ARXML Editor"));
}

MainWindow::~MainWindow()
{
    delete m_validator;
    delete m_model;
}

void MainWindow::openFile()
{
    const QString fileName = QFileDialog::getOpenFileName(
        this, tr("Open ARXML File"), {}, tr("AUTOSAR XML (*.arxml);;All Files (*)"));
    if (fileName.isEmpty())
        return;
    if (!m_model->loadFromFile(fileName)) {
        QMessageBox::warning(this, tr("Error"), tr("Cannot open %1").arg(fileName));
        return;
    }
    m_currentFileName = fileName;
    populateTree();
}

void MainWindow::populateTree()
{
    m_treeWidget->clear();
    QDomNode root = m_model->rootNode();
    if (root.isNull())
        return;
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(m_treeWidget);
    rootItem->setText(0, root.nodeName());
    rootItem->setData(0, Qt::UserRole, QVariant::fromValue(root));
    // Recursive lambda to build tree
    std::function<void(const QDomNode &, QTreeWidgetItem *)> build;
    build = [&](const QDomNode &node, QTreeWidgetItem *parentItem) {
        for (QDomNode child = node.firstChild(); !child.isNull(); child = child.nextSibling()) {
            if (!child.isElement())
                continue;
            QTreeWidgetItem *item = new QTreeWidgetItem(parentItem);
            item->setText(0, child.nodeName());
            // For leaf elements display text value
            bool hasChildElement = false;
            for (QDomNode n = child.firstChild(); !n.isNull(); n = n.nextSibling()) {
                if (n.isElement()) {
                    hasChildElement = true;
                    break;
                }
            }
            if (!hasChildElement) {
                QString textValue = child.toElement().text().simplified();
                if (!textValue.isEmpty()) {
                    item->setText(1, textValue);
                }
            }
            item->setData(0, Qt::UserRole, QVariant::fromValue(child));
            build(child, item);
        }
    };
    build(root, rootItem);
    m_treeWidget->expandItem(rootItem);
    m_treeWidget->setCurrentItem(rootItem);
}

void MainWindow::saveFile()
{
    if (m_model->document().isNull()) {
        QMessageBox::information(this, tr("No Document"),
                                 tr("Open an ARXML file before saving."));
        return;
    }
    if (m_currentFileName.isEmpty()) {
        saveFileAs();
        return;
    }
    if (m_model->saveToFile(m_currentFileName)) {
        QMessageBox::information(this, tr("Saved"),
                                 tr("File saved to %1").arg(m_currentFileName));
    }
}

void MainWindow::saveFileAs()
{
    if (m_model->document().isNull()) {
        QMessageBox::information(this, tr("No Document"),
                                 tr("Open an ARXML file before saving."));
        return;
    }
    const QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save ARXML File"), {}, tr("AUTOSAR XML (*.arxml);;All Files (*)"));
    if (fileName.isEmpty())
        return;
    if (m_model->saveToFile(fileName)) {
        m_currentFileName = fileName;
        QMessageBox::information(this, tr("Saved"),
                                 tr("File saved to %1").arg(fileName));
    }
}

void MainWindow::onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);
    if (!current) {
        m_propertyTable->clearContents();
        m_propertyTable->setRowCount(0);
        return;
    }
    QVariant v = current->data(0, Qt::UserRole);
    if (!v.canConvert<QDomNode>()) {
        m_propertyTable->clearContents();
        m_propertyTable->setRowCount(0);
        return;
    }
    QDomNode node = v.value<QDomNode>();
    populatePropertyTable(node);
}

void MainWindow::populatePropertyTable(const QDomNode &node)
{
    m_propertyTable->blockSignals(true);
    m_propertyTable->clearContents();
    m_propertyTable->setRowCount(0);
    int row = 0;
    // Tag row: not editable
    m_propertyTable->insertRow(row);
    QTableWidgetItem *fieldItem = new QTableWidgetItem(tr("Tag"));
    fieldItem->setFlags(fieldItem->flags() & ~Qt::ItemIsEditable);
    m_propertyTable->setItem(row, 0, fieldItem);
    QTableWidgetItem *valueItem = new QTableWidgetItem(node.nodeName());
    valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
    m_propertyTable->setItem(row, 1, valueItem);
    row++;
    // Attributes
    if (node.isElement()) {
        const QDomNamedNodeMap attrs = node.toElement().attributes();
        for (int i = 0; i < attrs.count(); ++i) {
            QDomAttr attr = attrs.item(i).toAttr();
            if (attr.isNull())
                continue;
            m_propertyTable->insertRow(row);
            QTableWidgetItem *attrNameItem = new QTableWidgetItem(attr.name());
            attrNameItem->setFlags(attrNameItem->flags() & ~Qt::ItemIsEditable);
            m_propertyTable->setItem(row, 0, attrNameItem);
            QTableWidgetItem *attrValueItem = new QTableWidgetItem(attr.value());
            // value is editable
            m_propertyTable->setItem(row, 1, attrValueItem);
            row++;
        }
    }
    // Text content
    QString text;
    if (node.isElement()) {
        text = node.toElement().text().simplified();
    }
    if (!text.isEmpty()) {
        m_propertyTable->insertRow(row);
        QTableWidgetItem *textNameItem = new QTableWidgetItem(tr("Text"));
        textNameItem->setFlags(textNameItem->flags() & ~Qt::ItemIsEditable);
        m_propertyTable->setItem(row, 0, textNameItem);
        QTableWidgetItem *textValueItem = new QTableWidgetItem(text);
        m_propertyTable->setItem(row, 1, textValueItem);
    }
    m_propertyTable->blockSignals(false);
}

void MainWindow::onPropertyItemChanged(QTableWidgetItem *item)
{
    if (!item || item->column() != 1)
        return;
    int row = item->row();
    QString newValue = item->text();
    QTreeWidgetItem *current = m_treeWidget->currentItem();
    if (!current)
        return;
    QVariant v = current->data(0, Qt::UserRole);
    if (!v.canConvert<QDomNode>())
        return;
    QDomNode domNode = v.value<QDomNode>();
    if (!domNode.isElement())
        return;
    QTableWidgetItem *fieldItem = m_propertyTable->item(row, 0);
    if (!fieldItem)
        return;
    QString fieldName = fieldItem->text();
    // Don't allow editing the tag name
    if (fieldName == tr("Tag"))
        return;
    if (fieldName == tr("Text")) {
        m_model->setNodeText(domNode, newValue);
    } else {
        m_model->setNodeAttribute(domNode, fieldName, newValue);
    }
    // Refresh tree value column if needed
    updateTreeItemText(current, domNode);
}

void MainWindow::updateTreeItemText(QTreeWidgetItem *item, const QDomNode &node)
{
    // Only update the value column for leaf elements
    bool hasChildElement = false;
    for (QDomNode n = node.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.isElement()) {
            hasChildElement = true;
            break;
        }
    }
    if (!hasChildElement) {
        QString text = node.toElement().text().simplified();
        item->setText(1, text);
    } else {
        item->setText(1, QString());
    }
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = m_treeWidget->itemAt(pos);
    if (!item)
        return;
    QMenu menu(this);
    QAction *addAction = menu.addAction(tr("Add Child"));
    QAction *delAction = nullptr;
    if (item->parent()) {
        delAction = menu.addAction(tr("Delete"));
    }
    QAction *selected = menu.exec(m_treeWidget->viewport()->mapToGlobal(pos));
    if (!selected)
        return;
    if (selected == addAction) {
        addChildElement();
    } else if (selected == delAction) {
        deleteElement();
    }
}

void MainWindow::addChildElement()
{
    QTreeWidgetItem *current = m_treeWidget->currentItem();
    if (!current)
        return;
    QVariant v = current->data(0, Qt::UserRole);
    if (!v.canConvert<QDomNode>())
        return;
    QDomNode parentNode = v.value<QDomNode>();
    if (!parentNode.isElement())
        return;
    bool ok = false;
    QString tagName = QInputDialog::getText(
        this, tr("Add Child Element"), tr("Element Tag Name:"), QLineEdit::Normal,
        QString(), &ok);
    if (!ok || tagName.isEmpty())
        return;
    QDomNode newNode = m_model->createChild(parentNode, tagName);
    // Create corresponding tree item
    QTreeWidgetItem *newItem = new QTreeWidgetItem(current);
    newItem->setText(0, tagName);
    newItem->setData(0, Qt::UserRole, QVariant::fromValue(newNode));
    // Remove any text from parent item now that it has children
    current->setText(1, QString());
    current->setExpanded(true);
}

void MainWindow::deleteElement()
{
    QTreeWidgetItem *current = m_treeWidget->currentItem();
    if (!current)
        return;
    if (!current->parent()) {
        QMessageBox::information(this, tr("Cannot Delete"),
                                 tr("The root element cannot be deleted."));
        return;
    }
    QVariant v = current->data(0, Qt::UserRole);
    if (!v.canConvert<QDomNode>())
        return;
    QDomNode node = v.value<QDomNode>();
    m_model->removeNode(node);
    delete current;
}

void MainWindow::validateDocument()
{
    if (m_model->document().isNull()) {
        QMessageBox::information(this, tr("Validate"),
                                 tr("Open an ARXML file before validating."));
        return;
    }
    // Ask user for schema if not already set
    if (m_schemaFileName.isEmpty()) {
        QString fileName = QFileDialog::getOpenFileName(
            this, tr("Select AUTOSAR Schema"), {}, tr("XSD Files (*.xsd);;All Files (*)"));
        if (fileName.isEmpty())
            return;
        m_schemaFileName = fileName;
    }
    QString result = m_validator->validate(*m_model, m_schemaFileName);
    if (result.isEmpty()) {
        QMessageBox::information(this, tr("Validate"), tr("The document is valid."));
    } else {
        QMessageBox::warning(this, tr("Validation Failed"), result);
    }
}