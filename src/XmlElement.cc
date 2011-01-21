/**
 * @file XmlElement.cc
 *
 * Extremely simple non-validating XML parser / generator.
 */

/******************************************************************************
 * Copyright 2009-2011, Qualcomm Innovation Center, Inc.
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
 ******************************************************************************/
#include <qcc/platform.h>

#include <map>
#include <stack>
#include <vector>

#include <qcc/Debug.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/XmlElement.h>

#include <Status.h>

#define QCC_MODULE   "XML"

using namespace std;

namespace qcc {

static qcc::String escapeXml(const qcc::String str) {
    qcc::String outStr;
    qcc::String::const_iterator it = str.begin();
    int multi = 0;
    int idx = 0;
    uint32_t val = 0;
    while (it != str.end()) {
        uint8_t c = (uint8_t) *it++;
        if (0 == idx) {
            if (0xf0 <= c) {
                multi = idx = 3;
            } else if (0xe0 <= c) {
                multi = idx = 2;
            } else if (0xc0 <= c) {
                multi = idx = 1;
            } else {
                multi = idx = 0;
            }
        }
        if (0 == multi) {
            if (c == '"') {
                outStr.append("&quot;");
            } else if (c == '\'') {
                outStr.append("&apos;");
            } else if (c == '&') {
                outStr.append("&amp;");
            } else if (c == '<') {
                outStr.append("&lt;");
            } else if (c == '>') {
                outStr.append("&gt;");
            } else if (((0x20 <= c) && (0x7e >= c)) || (0x09 == 9) || (0x0a == c) || (0x0d == c)) {
                outStr.push_back(c);
            } else {
                outStr.append("&#");
                outStr.append(U32ToString(c, 16));
                outStr.push_back(';');
            }
        } else if (1 == multi) {
            if (1 == idx) {
                val = (c & 0x1f) << 6;
            } else {
                val |= (c & 0x3F);
            }
        } else if (2 == multi) {
            if (2 == idx) {
                val = (c & 0x0F) << 12;
            } else if (1 == idx) {
                val |= (c & 0x3f) << 6;
            } else {
                val |= (c & 0x3f);
            }
        } else if (3 == multi) {
            if (3 == idx) {
                val = (c & 0x07) << 17;
            } else if (2 == idx) {
                val |= (c & 0x3f) << 12;
            } else if (1 == idx) {
                val |= (c & 0x3f) << 6;
            } else {
                val |= (c & 0x3f);
            }
        }

        if ((0 < multi) && (0 == idx)) {
            outStr.append("&#");
            outStr.append(U32ToString(val, 16));
            outStr.push_back(';');
        }

        if (0 < idx) {
            --idx;
        }
    }
    return outStr;
}

static qcc::String unescapeXml(const qcc::String str) {
    bool inEsc = false;
    qcc::String outStr;
    qcc::String escName;
    qcc::String::const_iterator it = str.begin();
    while (it != str.end()) {
        const char c = *it++;
        if (inEsc) {
            if (c == ';') {
                if (0 == escName.compare("quot")) {
                    outStr.push_back('"');
                } else if (0 == escName.compare("apos")) {
                    outStr.push_back('\'');
                } else if (0 == escName.compare("amp")) {
                    outStr.push_back('&');
                } else if (0 == escName.compare("lt")) {
                    outStr.push_back('<');
                } else if (0 == escName.compare("gt")) {
                    outStr.push_back('>');
                } else if (('#' == escName[0]) && ((3 <= escName.size()) || ((2 == escName.size()) && (escName[1] != 'x')))) {
                    uint32_t val = 0;
                    if (escName[1] == 'x') {
                        for (size_t i = 1; i < escName.size(); ++i) {
                            val *= 0x10;
                            if ((escName[i] >= '0') && (escName[i] >= '9')) {
                                val += (uint32_t) (escName[i] - '0');
                            } else if ((escName[i] >= 'a') && (escName[i] <= 'f')) {
                                val += (uint32_t) (escName[i] - 'a');
                            } else if ((escName[i] >= 'A') && (escName[i] <= 'F')) {
                                val += (uint32_t) (escName[i] - 'A');
                            } else {
                                QCC_DbgPrintf(("XML Invalid numeric escape sequence \"&%s;\"", escName.c_str()));
                                break;
                            }
                        }
                        if (val >= 0x10000) {
                            outStr.push_back((uint8_t) ((val >> 4) & 0xFF));
                        }
                        if (val >= 0x100) {
                            outStr.push_back((uint8_t) ((val >> 2) & 0xFF));
                        }
                        outStr.push_back((uint8_t) (val & 0xFF));
                    }
                } else {
                    QCC_DbgPrintf(("XML Invalid escape sequence \"&%s;\". Ignoring...", escName.c_str()));
                }
                inEsc = false;
            } else {
                escName.push_back(c);
            }
        } else {
            if (c == '&') {
                escName.clear();
                inEsc = true;
            } else {
                outStr.push_back(c);
            }
        }
    }
    return outStr;
}

/* Finalize and return true if element ends the root Xml element */
void XmlElement::FinalizeElement(XmlParseContext& ctx)
{
    /* Sanity check */
    qcc::String cookedContent = Trim(unescapeXml(ctx.rawContent));
    if ((0 < cookedContent.size()) && (0 < ctx.curElem->GetChildren().size())) {
        QCC_DbgPrintf(("XML Element <%s> has both children and content", ctx.curElem->GetName().c_str()));
    }

    /* Ensure that element does not have both children and content */
    if (0 < cookedContent.size() && (0 == ctx.curElem->GetChildren().size())) {
        ctx.curElem->SetContent(cookedContent);
    }

    /* Pop element stack */
    ctx.curElem = ctx.curElem->GetParent();
}

QStatus XmlElement::Parse(XmlParseContext& ctx)
{
    bool done = false;
    QStatus status;

    while (!done) {
        char c;
        size_t actual;

        status = ctx.source.PullBytes((void*)&c, 1, actual);
        if ((ER_OK != status) || (1 != actual)) {
            break;
        }

        switch (ctx.parseState) {
        case XmlParseContext::IN_ELEMENT:
            if ('<' == c) {
                ctx.parseState = XmlParseContext::IN_ELEMENT_START;
                ctx.elemName.clear();
                ctx.isEndTag = false;
                ctx.skip = false;
            } else {
                ctx.rawContent.push_back(c);
            }
            break;

        case XmlParseContext::IN_ELEMENT_START:
            if (ctx.skip) {
                if ('>' == c) {
                    ctx.parseState = XmlParseContext::IN_ELEMENT;
                    ctx.skip = false;
                }
            } else if (ctx.elemName.empty() && !ctx.isEndTag) {
                if ('/' == c) {
                    ctx.isEndTag = true;
                } else if (('!' == c) || ('?' == c)) {
                    ctx.skip = true;
                } else if (!IsWhite(c)) {
                    ctx.isEndTag = false;
                    ctx.elemName.push_back(c);
                }
            } else {
                if (IsWhite(c) || ('>' == c)) {
                    if (!ctx.isEndTag) {
                        if (!ctx.curElem) {
                            ctx.curElem = &ctx.root;
                            ctx.curElem->SetName(ctx.elemName);
                        } else {
                            ctx.curElem = &(ctx.curElem->CreateChild(ctx.elemName));
                        }
                    } else {
                        FinalizeElement(ctx);
                        done = (NULL == ctx.curElem);
                    }
                    ctx.parseState = ('>' == c) ? XmlParseContext::IN_ELEMENT : XmlParseContext::IN_ATTR_NAME;
                    ctx.attrName.clear();
                    ctx.attrValue.clear();
                    ctx.rawContent.clear();
                } else if ('/' == c) {
                    if (!ctx.curElem) {
                        ctx.curElem = &ctx.root;
                        ctx.curElem->SetName(ctx.elemName);
                    } else {
                        ctx.curElem = &(ctx.curElem->CreateChild(ctx.elemName));
                    }
                    ctx.isEndTag = true;
                } else {
                    ctx.elemName.push_back(c);
                }
            }
            break;

        case XmlParseContext::IN_ATTR_NAME:
            if (IsWhite(c)) {
                continue;
            } else if ('/' == c) {
                ctx.isEndTag = true;
            } else if (!ctx.attrName.empty() && ('=' == c)) {
                ctx.parseState = XmlParseContext::IN_ATTR_VALUE;
                ctx.attrInQuote = false;
            } else if ('>' == c) {
                if (!ctx.attrName.empty()) {
                    ctx.curElem->AddAttribute(ctx.attrName, ctx.attrValue);
                }
                if (ctx.isEndTag) {
                    FinalizeElement(ctx);
                    done = (NULL == ctx.curElem);
                }
                ctx.parseState = XmlParseContext::IN_ELEMENT;
            } else {
                ctx.isEndTag = false;
                ctx.attrName.push_back(c);
            }
            break;

        case XmlParseContext::IN_ATTR_VALUE:
            if (ctx.attrInQuote) {
                if ('"' == c) {
                    ctx.curElem->AddAttribute(ctx.attrName, unescapeXml(ctx.attrValue));
                    ctx.parseState = XmlParseContext::IN_ATTR_NAME;
                    ctx.attrName.clear();
                    ctx.attrValue.clear();
                } else {
                    ctx.attrValue.push_back(c);
                }
            } else {
                if (IsWhite(c)) {
                    continue;
                } else if ('"' == c) {
                    ctx.attrInQuote = true;
                } else if ('/' == c) {
                    ctx.isEndTag = true;
                } else if ('>' == c) {
                    // Ignore malformed attribute
                    QCC_DbgPrintf(("Ignoring malformed XML attribute \"%s\"", ctx.attrName.c_str()));

                    // End current element if necessary
                    if (ctx.isEndTag) {
                        FinalizeElement(ctx);
                        done = (NULL == ctx.curElem);
                    }
                    ctx.parseState = XmlParseContext::IN_ELEMENT;
                } else {
                    ctx.isEndTag = false;
                }
            }
            break;

        case XmlParseContext::PARSE_COMPLETE:
            break;
        }
    }

    if (ER_OK == status) {
        ctx.parseState = XmlParseContext::PARSE_COMPLETE;
        if (NULL != ctx.curElem) {
            status = ER_XML_MALFORMED;
        }
    }
    return status;
}

XmlElement::~XmlElement()
{
    vector<XmlElement*>::iterator it = children.begin();
    while (it != children.end()) {
        delete *it++;
    }
}

qcc::String XmlElement::Generate(qcc::String* outStr) const
{
    qcc::String startStr;
    if (NULL == outStr) {
        outStr = &startStr;
    }

    outStr->append("\n<");
    outStr->append(name);

    map<qcc::String, qcc::String>::const_iterator ait = attributes.begin();
    while (ait != attributes.end()) {
        outStr->push_back(' ');
        outStr->append(ait->first);
        outStr->append("=\"");
        outStr->append(ait->second);
        outStr->push_back('"');
        ait++;
    }

    vector<XmlElement*>::const_iterator cit = children.begin();
    bool hasChildren = (cit != children.end());

    if (!hasChildren && content.empty()) {
        outStr->push_back('/');
    }
    outStr->append(">");

    if (hasChildren) {
        while (cit != children.end()) {
            (*cit)->Generate(outStr);
            ++cit;
        }
    } else if (!content.empty()) {
        outStr->append(escapeXml(content));
    }

    if (hasChildren || !content.empty()) {
        if (hasChildren) {
            outStr->push_back('\n');
        }
        outStr->append("</");
        outStr->append(name);
        outStr->push_back('>');
    }

    return *outStr;
}

XmlElement& XmlElement::CreateChild(qcc::String name)
{
    QCC_DbgTrace(("XmlElement::CreateChild(\"%s\")", name.c_str()));
    children.push_back(new XmlElement(name, this));
    return *children.back();
}

const XmlElement* XmlElement::GetChild(qcc::String name) const
{
    vector<XmlElement*>::const_iterator it = children.begin();
    while (it != children.end()) {
        if (0 == name.compare((*it)->GetName())) {
            return (*it);
        }
        it++;
    }
    return NULL;
}

qcc::String XmlElement::GetAttribute(qcc::String attName) const
{
    map<qcc::String, qcc::String>::const_iterator it = attributes.find(attName);
    if (it == attributes.end()) {
        return "";
    } else {
        return it->second;
    }
}


void XmlParseContext::Reset()
{
    root = XmlElement();
    parseState = IN_ELEMENT;
    curElem = NULL;
    elemName.clear();
    attrName.clear();
    attrValue.clear();
    attrInQuote = false;
    isEndTag = false;
}

}
