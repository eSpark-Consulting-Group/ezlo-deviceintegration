#include "luup/vera_plugins/xml_wrapper.h"

XmlNode::XmlNode( IXML_Node* node )
: m_node( node ) {}

bool XmlNode::getTextFromNode( std::string& text ) {
    if( m_node && m_node->firstChild && m_node->firstChild->nodeType == eTEXT_NODE ) {
        text = m_node->firstChild->nodeValue;
        return true;
    }
    return false;
}

std::string XmlNode::tryGetTextFromNode() {
    std::string text;
    if( getTextFromNode( text ) ) {
        return text;
    }
    throw XmlException( "Could not get text from xml node" );
}

XmlNode XmlNode::getChildNode( const std::string& name, bool recurse ) {
    return getChildNode( m_node, name, recurse );
}

XmlNode XmlNode::tryGetChildNode( const std::string& name, bool recurse ) {
    if( auto node = getChildNode( name, recurse ) ) {
        return node;
    }
    throw XmlException( "Could not find node " + name );
}

IXML_Node* XmlNode::operator->() {
    return m_node;
}

XmlNode::operator IXML_Node*() {
    return m_node;
}

IXML_Node* XmlNode::getChildNode( IXML_Node* parent, const std::string& name, bool recurse ) {
    IXML_Node* node = parent->firstChild;
    while( node != nullptr ) {
        if( node->nodeName == name ) {
            return node;
        }
        node = node->nextSibling;
    }

    if( recurse ) {
        node = parent->firstChild;
        while( node != nullptr ) {
            if( auto child = getChildNode( node, name, true ) ) {
                return child;
            }
            node = node->nextSibling;
        }
    }

    return nullptr;
}

XmlDocument::XmlDocument( const std::string& data )
: m_xmlDoc( nullptr ) {
    int result = ixmlParseBufferEx( data.c_str(), &m_xmlDoc );
    if( result != IXML_SUCCESS ) {
        throw XmlException( "Failed parsing xml, error " + std::to_string( result ) );
    }
}

XmlDocument::XmlDocument( IXML_Document* xmlDoc )
: m_xmlDoc( xmlDoc ) {}

XmlDocument::~XmlDocument() {
    if( m_xmlDoc ) {
        ixmlDocument_free( m_xmlDoc );
    }
}

bool XmlDocument::isValid() const {
    return m_xmlDoc != nullptr;
}

XmlNode XmlDocument::getRootNode() {
    if( !isValid() )
        throw XmlException( "Xml document is invalid" );

    XmlNode node = &m_xmlDoc->n;
    if( auto root = node.getChildNode( "root" ) ) {
        return root;
    }
    return node;
}
