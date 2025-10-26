// arxml_model.cpp
//
// Implementation of the ArxmlModel class. This class wraps a QDomDocument
// and provides convenience methods for loading, saving and modifying
// AUTOSAR ARXML documents. Separating the document logic from the UI
// allows us to follow a clean architecture where the model handles data
// storage and manipulation.

#include "arxml_model.hpp"

#include <QFile>
#include <QTextStream>
#include <QDomElement>
#include <QDomText>

bool ArxmlModel::loadFromFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    QString errorMsg;
    int errorLine = 0;
    int errorColumn = 0;
    QDomDocument newDoc;
    if (!newDoc.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
        file.close();
        return false;
    }
    file.close();
    m_document = newDoc;
    return true;
}

bool ArxmlModel::saveToFile(const QString &fileName) const
{
    QFile outFile(fileName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    QTextStream stream(&outFile);
    m_document.save(stream, 4);
    outFile.close();
    return true;
}

QDomNode ArxmlModel::rootNode() const
{
    return m_document.documentElement();
}

QDomNode ArxmlModel::createChild(const QDomNode &parent, const QString &tagName)
{
    if (!parent.isElement()) {
        return QDomNode();
    }
    QDomElement parentElem = parent.toElement();
    QDomElement newElem = m_document.createElement(tagName);
    parentElem.appendChild(newElem);
    return QDomNode(newElem);
}

void ArxmlModel::removeNode(const QDomNode &node)
{
    if (node.isNull()) {
        return;
    }
    QDomNode parentNode = node.parentNode();
    if (!parentNode.isNull()) {
        parentNode.removeChild(node);
    }
}

void ArxmlModel::setNodeText(const QDomNode &node, const QString &text)
{
    if (!node.isElement()) {
        return;
    }
    QDomElement elem = node.toElement();
    // Look for an existing text node
    QDomNode textNode;
    for (QDomNode n = elem.firstChild(); !n.isNull(); n = n.nextSibling()) {
        if (n.isText()) {
            textNode = n;
            break;
        }
    }
    if (!textNode.isNull()) {
        textNode.setNodeValue(text);
    } else {
        QDomText newText = m_document.createTextNode(text);
        elem.appendChild(newText);
    }
}

void ArxmlModel::setNodeAttribute(const QDomNode &node, const QString &name,
                                   const QString &value)
{
    if (!node.isElement()) {
        return;
    }
    QDomElement elem = node.toElement();
    elem.setAttribute(name, value);
}