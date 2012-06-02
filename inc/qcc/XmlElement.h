/**
 * @file XmlElement.h
 *
 * Extremely simple XML Parser/Generator.
 *
 */

/******************************************************************************
 * Copyright 2010 - 2011, Qualcomm Innovation Center, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 ******************************************************************************/

#ifndef _XMLELEMENT_H
#define _XMLELEMENT_H

#include <qcc/platform.h>

#include <map>
#include <vector>

#include <qcc/String.h>
#include <qcc/BufferedSource.h>
#include <qcc/Stream.h>

namespace qcc {

/** @internal Forward Decl */
struct XmlParseContext;

/**
 * XMLElement is used to generate and parse simple XML fragments.
 * This is not a full-blown XML parser/generator and performs no DTD validation or other advanced features.
 */
class XmlElement {
  public:

    /**
     * Create an XmlElement from an XML document fragment.
     * It is the responsibility of the caller to free the pointer returned in root.
     *
     * @param ctx    Parsing context (see {@link #XmlParseContext})
     * @return    ER_OK if parse was successful,
     *            ER_WOULDBLOCK if parse is partially completed pending more I/O,
     *            Otherwise error
     */
    static QStatus Parse(XmlParseContext& ctx);

    /**
     * Construct an XmlElement with a given name and parent.
     *
     * @param name     XML element name.
     * @param parent   Parent element or NULL if this element is the root.
     */
    XmlElement(const qcc::String& name = String::Empty, XmlElement* parent = NULL) : name(name), parent(parent) { }

    /** Destructor */
    ~XmlElement();

    /**
     * Output an XML fragment of this XmlElement including any childeren.
     *
     * @param outStr  Optional string to be used for output. (Defaults to empty string.)
     * @return  XML Fragment
     */
    qcc::String Generate(qcc::String* outStr = NULL) const;

    /**
     * Get the element name
     *
     * @return XML element name or empty string if not set.
     */
    const qcc::String& GetName() const { return name; }

    /**
     * Get this element's parent or NULL if none exists.
     *
     * @return XML parent or NULL.
     */
    XmlElement* GetParent() const { return parent; }

    /**
     * Set the element name.
     *
     * @param name    Name of XML element.`
     */
    void SetName(const qcc::String& name) { this->name = name; }

    /**
     * Get the attributes for this element.
     * @return  Map of attribute name/value pairs
     */
    const std::map<qcc::String, qcc::String>& GetAttributes() const { return attributes; }

    /**
     * Get an attribute with a given name or empty string if it doesn't exist.
     *
     * @param attName   Name of attribute
     */
    const qcc::String& GetAttribute(const char* attName) const;

    /**
     * Get an attribute with a given name or empty string if it doesn't exist.
     *
     * @param attName   Name of attribute
     */
    const qcc::String& GetAttribute(const qcc::String& attName) const;

    /**
     * Add an Xml Attribute
     *
     * @param name    Attribute name.
     * @param value   Attribute value.
     */
    void AddAttribute(const qcc::String& name, const qcc::String& value) { attributes[name] = value; }

    /**
     * Get the element map.
     */
    const std::vector<XmlElement*>& GetChildren() const { return children; }

    /**
     * Get all children with a given name
     *
     * @param name   XML child elements name to search for.
     * @return  A vector containing the matching elements.
     */
    std::vector<const XmlElement*> GetChildren(const qcc::String& name) const;

    /**
     * Get the child element with a given name if it exists.
     *
     * @param name   XML child element name to search for.
     * @return  Pointer to XML child element or NULL if not found.
     */
    const XmlElement* GetChild(const qcc::String& name) const;

    /**
     * Add a child XmlElement.
     *
     * @param name   Child node name.
     */
    XmlElement& CreateChild(const qcc::String& name);

    /**
     * Get the content.
     */
    const qcc::String& GetContent() const { return content; }

    /**
     * Set the (unesacped) text content.
     *
     * @param  content    Unescaped ("&" not "&amp;") text content for this node.
     */
    void SetContent(const qcc::String& content) { this->content = content; }

    /**
     * Add text content to this node.
     * An XmlElement can only have content or children. Not both. If content is added
     * to an XmlElement that has children, the text content will be silently ignored.
     *
     * @param content   Text content to add to this node.
     */
    void AddContent(const qcc::String& content) { this->content.append(content); }

    /**
     * Get all elements that have the specified path relative to the current element. The path is a
     * series of tag names separated by '/' with an optional attribute specified by an '@' character
     * followed by the the attribute name.
     *
     * Given the XML below GetPath("foo/bar/value@first") will return the the <value> element
     * containing "hello" and GetPath("foo/bar/value@second") will return the <value> element
     * containing "world". GetPath("foo/bar/value") will return both <value> elements.
     *
     * <foo>
     *    <bar>
     *       <value first="hello"/>
     *       <value second="world"/>
     *    </bar>
     * </foo>
     *
     * @param key   The key is a dotted path (with optional attribute) to a value in the XML
     *
     * @param path   The path to elements in the XML tree.
     */
    std::vector<const XmlElement*> GetPath(const qcc::String& path) const;

  private:
    qcc::String name;                                /**< Element name */
    std::vector<XmlElement*> children;               /**< XML child elements */
    std::map<qcc::String, qcc::String> attributes;   /**< XML attributes */
    qcc::String content;                             /**< XML text content (unesacped) */
    XmlElement* parent;                              /**< XML parent element or NULL if root */

    /**
     * Helper used during parsing.
     * @param ctx  Parsing context.
     */
    static void FinalizeElement(XmlParseContext& ctx);
};

/**
 * XmlParseContext contains XML parsing state.
 */
struct XmlParseContext {
    friend class XmlElement;

  public:
    /**
     * Create a parse context that uses a given XML source.
     *
     * @param source  BufferedSource containing XML formatted data.
     */
    XmlParseContext(Source& source) :
        source(source),
        parseState(IN_ELEMENT),
        root(new XmlElement()),
        curElem(NULL),
        attrInQuote(false),
        isEndTag(false),
        skip(false) { }

    /** Reset state of XmlParseContext in preparation for reuse */
    void Reset();

    /**
     * Detach the current root and return it. It is the responsiblity of the caller
     * to free the root when no longer needed.
     */
    XmlElement* DetachRoot() {
        XmlElement* xml = root;
        root = NULL;
        Reset();
        return xml;
    }

    /**
     * Return a const pointer to the current root. The root will become invalid when the context is
     * freed.
     */
    const XmlElement* GetRoot() {
        return root;
    }

    /**
     * Destructor
     */
    ~XmlParseContext() {
        delete root;
    }

  private:

    /** Xml source */
    Source& source;

    /** Parse state */
    enum {
        IN_ELEMENT,
        IN_ELEMENT_START,
        IN_ATTR_NAME,
        IN_ATTR_VALUE,
        PARSE_COMPLETE
    } parseState;

    XmlElement* root;         /**< Parsed root element */
    XmlElement* curElem;      /**< XML element currently being parsed */
    qcc::String rawContent;   /**< Text content for current element */
    qcc::String elemName;     /**< Name of current element */
    qcc::String attrName;     /**< Name of attribute currently being parsed. */
    qcc::String attrValue;    /**< Value of attribute currently being parsed. */
    bool attrInQuote;         /**< true iff inside attribute value quotes */
    bool isEndTag;            /**< true iff currently parsed tag is an end tag */
    bool skip;                /**< true iff elements starts with "<!" */
};

}

#endif
