#pragma once
#include "upnp/ixml.h"
#include <string>
#include <stdexcept>

class XmlException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class XmlNode final {
public:
    XmlNode( IXML_Node* node );

    bool getTextFromNode( std::string& text );
    std::string tryGetTextFromNode();

    XmlNode getChildNode( const std::string& name, bool recurse = false );
    XmlNode tryGetChildNode( const std::string& name, bool recurse = false );

    operator IXML_Node*();
    IXML_Node* operator->();

private:
    static IXML_Node* getChildNode( IXML_Node* parent, const std::string& name, bool recurse = false );

private:
    IXML_Node* m_node;
};

class XmlDocument final {
public:
    XmlDocument( const std::string& data );
    XmlDocument( IXML_Document* xmlDoc = nullptr );
    ~XmlDocument();

    bool isValid() const;
    XmlNode getRootNode();

private:
    IXML_Document* m_xmlDoc;
};
