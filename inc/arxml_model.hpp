// arxml_model.hpp
//
// This header defines ArxmlModel, a thin wrapper around QDomDocument that
// encapsulates the logic for loading, saving and manipulating AUTOSAR ARXML
// documents. By separating DOM management into a distinct class, the main
// window can focus on presentation and user interactions. This design
// improves readability and follows the single responsibility principle.

#ifndef ARXML_MODEL_HPP
#define ARXML_MODEL_HPP

#include <QDomDocument>
#include <QString>

class ArxmlModel
{
public:
    // Load an ARXML file into the internal document. Returns true on success.
    bool loadFromFile(const QString &fileName);

    // Save the current document to the specified file. Returns true on success.
    bool saveToFile(const QString &fileName) const;

    // Access the underlying QDomDocument. Used by other components to query
    // or modify the model. Modifications made directly to this document will
    // affect the model state.
    QDomDocument &document() { return m_document; }
    const QDomDocument &document() const { return m_document; }

    // Return the root node of the document. A null node is returned if
    // the document is empty.
    QDomNode rootNode() const;

    // Create a new child element under the given parent node with the
    // specified tag name. Returns the newly created node.
    QDomNode createChild(const QDomNode &parent, const QString &tagName);

    // Remove the specified node from its parent. Does nothing if the node
    // has no parent or is null.
    void removeNode(const QDomNode &node);

    // Set the text content of an element. If the element already has
    // a text node, it will be replaced; otherwise a new text node is
    // appended.
    void setNodeText(const QDomNode &node, const QString &text);

    // Set or update an attribute on an element. If the attribute exists
    // it will be updated; otherwise it will be added.
    void setNodeAttribute(const QDomNode &node, const QString &name,
                          const QString &value);

private:
    QDomDocument m_document;
};

#endif // ARXML_MODEL_HPP