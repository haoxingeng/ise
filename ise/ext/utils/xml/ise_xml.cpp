/****************************************************************************\
*                                                                            *
*  ISE (Iris Server Engine) Project                                          *
*  http://github.com/haoxingeng/ise                                          *
*                                                                            *
*  Copyright 2013 HaoXinGeng (haoxingeng@gmail.com)                          *
*  All rights reserved.                                                      *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
\****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// 文件名称: ise_xml.cpp
// 功能描述: XML文档支持
///////////////////////////////////////////////////////////////////////////////

#include "ise/ext/utils/xml/ise_xml.h"
#include "ise/main/ise_exceptions.h"

namespace ise
{

namespace utils
{

///////////////////////////////////////////////////////////////////////////////
// 杂项函数

//-----------------------------------------------------------------------------
// 描述: 任意字符串 -> 合法的XML字符串
//-----------------------------------------------------------------------------
std::string strToXml(const std::string& str)
{
    std::string s = str;

    for (int i = (int)s.size() - 1; i >= 0; i--)
    {
        switch (s[i])
        {
        case '<':
            s.insert(i + 1, "lt;");
            s[i] = '&';
            break;

        case '>':
            s.insert(i + 1, "gt;");
            s[i] = '&';
            break;

        case '&':
            s.insert(i + 1, "amp;");
            s[i] = '&';
            break;

        case '\'':
            s.insert(i + 1, "apos;");
            s[i] = '&';
            break;

        case '\"':
            s.insert(i + 1, "quot;");
            s[i] = '&';
            break;

        case 10:
        case 13:
            std::string strRep;
            strRep = std::string("#") + ise::intToStr(s[i]) + ";";
            s.insert(i + 1, strRep);
            s[i] = '&';
            break;
        };
    }

    return s;
}

//-----------------------------------------------------------------------------
// 描述: XML字符串 -> 实际字符串
// 备注:
//   &gt;   -> '>'
//   &lt;   -> '<'
//   &quot; -> '"'
//   &#XXX; -> 十进制值所对应的ASCII字符
//-----------------------------------------------------------------------------
std::string xmlToStr(const std::string& str)
{
    std::string s = str;
    int i, j, h, n;

    i = 0;
    n = (int)s.size();
    while (i < n - 1)
    {
        if (s[i] == '&')
        {
            if (i + 3 < n && s[i + 1] == '#')
            {
                j = i + 3;
                while (j < n && s[j] != ';') j++;
                if (s[j] == ';')
                {
                    h = strToInt(s.substr(i + 2, j - i - 2).c_str());
                    s.erase(i, j - i);
                    s[i] = static_cast<char>(h);
                    n -= (j - i);
                }
            }
            else if (i + 3 < n && s.substr(i + 1, 3) == "gt;")
            {
                s.erase(i, 3);
                s[i] = '>';
                n -= 3;
            }
            else if (i + 3 < n && s.substr(i + 1, 3) == "lt;")
            {
                s.erase(i, 3);
                s[i] = '<';
                n -= 3;
            }
            else if (i + 4 < n && s.substr(i + 1, 4) == "amp;")
            {
                s.erase(i, 4);
                s[i] = '&';
                n -= 4;
            }
            else if (i + 5 < n && s.substr(i + 1, 5) == "apos;")
            {
                s.erase(i, 5);
                s[i] = '\'';
                n -= 5;
            }
            else if (i + 5 < n && s.substr(i + 1, 5) == "quot;")
            {
                s.erase(i, 5);
                s[i] = '\"';
                n -= 5;
            }
        }

        i++;
    }

    return s;
}

//-----------------------------------------------------------------------------
// 描述: 在串s中查找 chars 中的任一字符，若找到则返回其位置(0-based)，否则返回-1。
//-----------------------------------------------------------------------------
int findChars(const std::string& s, const std::string& chars)
{
    int result = -1;
    int len = (int)s.length();

    for (int i = 0; i < len; i++)
        if (chars.find(s[i], 0) != std::string::npos)
        {
            result = i;
            break;
        }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// XmlNode

XmlNode::XmlNode() :
    parentNode_(NULL),
    childNodes_(NULL),
    props_(NULL)
{
    initObject();
}

XmlNode::XmlNode(const XmlNode& src)
{
    initObject();
    *this = src;
}

XmlNode::~XmlNode()
{
    clear();
    delete props_;
    if (parentNode_ && parentNode_->childNodes_)
        parentNode_->childNodes_->remove(this);
}

void XmlNode::initObject()
{
    props_ = new XmlNodeProps();
}

void XmlNode::assignNode(XmlNode& src, XmlNode& dest)
{
    dest.name_ = src.name_;
    dest.dataString_ = src.dataString_;
    *(dest.props_) = *(src.props_);
    for (int i = 0; i < src.getChildCount(); i++)
        assignNode(*(src.getChildNodes(i)), *(dest.addNode()));
}

XmlNode& XmlNode::operator = (const XmlNode& rhs)
{
    if (this == &rhs) return *this;

    clear();
    assignNode((XmlNode&)rhs, *this);
    return *this;
}

//-----------------------------------------------------------------------------
// 描述: 在当前节点下添加子节点(Node)
//-----------------------------------------------------------------------------
void XmlNode::addNode(XmlNode *node)
{
    if (!childNodes_)
        childNodes_ = new PointerList();

    childNodes_->add(node);
    if (node->parentNode_ != NULL)
        node->parentNode_->childNodes_->remove(node);
    node->parentNode_ = this;
}

//-----------------------------------------------------------------------------
// 描述: 在当前节点下添加一个新的子节点
//-----------------------------------------------------------------------------
XmlNode* XmlNode::addNode()
{
    XmlNode *node = new XmlNode();
    addNode(node);
    return node;
}

//-----------------------------------------------------------------------------
// 描述: 在当前节点下添加一个新的子节点
//-----------------------------------------------------------------------------
XmlNode* XmlNode::addNode(const std::string& name, const std::string& dataString)
{
    XmlNode *node = addNode();
    node->setName(name);
    node->setDataString(dataString);
    return node;
}

//-----------------------------------------------------------------------------
// 描述: 在当前节点下插入子节点
//-----------------------------------------------------------------------------
void XmlNode::insertNode(int index, XmlNode *node)
{
    addNode(node);
    childNodes_->del(childNodes_->getCount() - 1);
    childNodes_->insert(index, node);
}

//-----------------------------------------------------------------------------
// 描述: 根据子节点名称，查找子节点。若未找到则返回 NULL。
//-----------------------------------------------------------------------------
XmlNode* XmlNode::findChildNode(const std::string& name)
{
    XmlNode *result;
    int i;

    i = indexOf(name);
    if (i >= 0)
        result = getChildNodes(i);
    else
        result = NULL;

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 在当前节点下查找名称为 name 的节点，返回其序号(0-based)。若未找到则返回-1。
//-----------------------------------------------------------------------------
int XmlNode::indexOf(const std::string& name)
{
    int result = -1;

    for (int i = 0; i < getChildCount(); i++)
        if (sameText(name, getChildNodes(i)->getName()))
        {
            result = i;
            break;
        }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 清空当前节点及其所有子节点的数据
//-----------------------------------------------------------------------------
void XmlNode::clear()
{
    if (childNodes_)
    {
        while (childNodes_->getCount() > 0)
            delete (XmlNode*)(*childNodes_)[0];
        delete childNodes_;
        childNodes_ = NULL;
    }

    name_.clear();
    dataString_.clear();
    props_->clear();
}

XmlNode* XmlNode::getRootNode() const
{
    XmlNode *result = (XmlNode*)this;

    while (result->getParentNode() != NULL)
        result = result->getParentNode();

    return result;
}

int XmlNode::getChildCount() const
{
    if (!childNodes_)
        return 0;
    else
        return childNodes_->getCount();
}

XmlNode* XmlNode::getChildNodes(int index) const
{
    if (!childNodes_)
        return NULL;
    else
        return (XmlNode*)(*childNodes_)[index];
}

void XmlNode::setParentNode(XmlNode *node)
{
    if (parentNode_ && parentNode_->childNodes_)
        parentNode_->childNodes_->remove(this);
    parentNode_ = node;
}

void XmlNode::setDataString(const std::string& value)
{
    dataString_ = xmlToStr(value);
}

///////////////////////////////////////////////////////////////////////////////
// XmlNodeProps

XmlNodeProps::XmlNodeProps()
{
    // nothing
}

XmlNodeProps::XmlNodeProps(const XmlNodeProps& src)
{
    *this = src;
}

XmlNodeProps::~XmlNodeProps()
{
    clear();
}

XmlNodeProps& XmlNodeProps::operator = (const XmlNodeProps& rhs)
{
    if (this == &rhs) return *this;

    clear();
    for (int i = 0; i < rhs.getCount(); i++)
    {
        XmlNodePropItem item = rhs.getItems(i);
        add(item.name, item.value);
    }

    return *this;
}

bool XmlNodeProps::add(const std::string& name, const std::string& value)
{
    bool result;

    result = (indexOf(name) < 0);
    if (result)
    {
        XmlNodePropItem *item = new XmlNodePropItem();
        item->name = name;
        item->value = value;
        items_.add(item);
    }

    return result;
}

void XmlNodeProps::remove(const std::string& name)
{
    int i;

    i = indexOf(name);
    if (i >= 0)
    {
        delete getItemPtr(i);
        items_.del(i);
    }
}

void XmlNodeProps::clear()
{
    for (int i = 0; i < items_.getCount(); i++)
        delete getItemPtr(i);
    items_.clear();
}

int XmlNodeProps::indexOf(const std::string& name)
{
    int result = -1;

    for (int i = 0; i < items_.getCount(); i++)
        if (sameText(name, getItemPtr(i)->name))
        {
            result = i;
            break;
        }

    return result;
}

bool XmlNodeProps::propExists(const std::string& name)
{
    return (indexOf(name) >= 0);
}

std::string& XmlNodeProps::valueOf(const std::string& name)
{
    int i;

    i = indexOf(name);
    if (i >= 0)
        return getItemPtr(i)->value;
    else
    {
        add(name, "");
        return getItemPtr(getCount()-1)->value;
    }
}

XmlNodePropItem XmlNodeProps::getItems(int index) const
{
    XmlNodePropItem result;

    if (index >= 0 && index < items_.getCount())
        result = *(((XmlNodeProps*)this)->getItemPtr(index));

    return result;
}

std::string XmlNodeProps::getPropString() const
{
    std::string result;
    XmlNodePropItem *item;

    for (int i = 0; i < items_.getCount(); i++)
    {
        item = ((XmlNodeProps*)this)->getItemPtr(i);
        if (i > 0) result = result + " ";
        result = result + formatString("%s=\"%s\"", item->name.c_str(), item->value.c_str());
    }

    return result;
}

void XmlNodeProps::setPropString(const std::string& propString)
{
    parsePropString(propString);
}

void XmlNodeProps::parsePropString(const std::string& propStr)
{
    std::string s, name, value;

    clear();
    s = propStr;
    while (true)
    {
        std::string::size_type i = s.find('=', 0);
        if (i != std::string::npos)
        {
            name = trimString(s.substr(0, i));
            s.erase(0, i + 1);
            s = trimString(s);
            if (s.size() > 0 && s[0] == '\"')
            {
                s.erase(0, 1);
                i = s.find('\"', 0);
                if (i != std::string::npos)
                {
                    value = xmlToStr(s.substr(0, i));
                    s.erase(0, i + 1);
                }
                else
                    iseThrowException(SEM_INVALID_XML_FILE_FORMAT);
            }
            else
                iseThrowException(SEM_INVALID_XML_FILE_FORMAT);

            add(name, value);
        }
        else
        {
            if (trimString(s).size() > 0)
                iseThrowException(SEM_INVALID_XML_FILE_FORMAT);
            else
                break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// XmlDocument

XmlDocument::XmlDocument() :
    autoIndent_(true),
    indentSpaces_(DEF_XML_INDENT_SPACES)
{
    // nothing
}

XmlDocument::XmlDocument(const XmlDocument& src)
{
    *this = src;
}

XmlDocument::~XmlDocument()
{
    // nothing
}

XmlDocument& XmlDocument::operator = (const XmlDocument& rhs)
{
    if (this == &rhs) return *this;

    autoIndent_ = rhs.autoIndent_;
    indentSpaces_ = rhs.indentSpaces_;
    rootNode_ = rhs.rootNode_;

    return *this;
}

bool XmlDocument::saveToStream(Stream& stream)
{
    try
    {
        XmlWriter writer(this, &stream);
        writer.writeHeader(S_DEF_XML_DOC_VER, encoding_);
        writer.writeRootNode(&rootNode_);
        return true;
    }
    catch (Exception&)
    {
        return false;
    }
}

bool XmlDocument::loadFromStream(Stream& stream)
{
    try
    {
        std::string version, encoding;
        XmlReader reader(this, &stream);

        rootNode_.clear();
        reader.readHeader(version, encoding);
        reader.readRootNode(&rootNode_);

        encoding_ = encoding;
        return true;
    }
    catch (Exception&)
    {
        return false;
    }
}

bool XmlDocument::saveToString(std::string& str)
{
    MemoryStream stream;
    bool result = saveToStream(stream);
    str.assign(stream.getMemory(), (int)stream.getSize());
    return result;
}

bool XmlDocument::loadFromString(const std::string& str)
{
    MemoryStream stream;
    stream.write(str.c_str(), (int)str.size());
    stream.setPosition(0);
    return loadFromStream(stream);
}

bool XmlDocument::saveToFile(const std::string& fileName)
{
    FileStream fs;
    bool result = fs.open(fileName, FM_CREATE | FM_SHARE_DENY_WRITE);
    if (result)
        result = saveToStream(fs);
    return result;
}

bool XmlDocument::loadFromFile(const std::string& fileName)
{
    FileStream fs;
    bool result = fs.open(fileName, FM_OPEN_READ | FM_SHARE_DENY_NONE);
    if (result)
        result = loadFromStream(fs);
    return result;
}

void XmlDocument::clear()
{
    rootNode_.clear();
}

///////////////////////////////////////////////////////////////////////////////
// XmlReader

XmlReader::XmlReader(XmlDocument *owner, Stream *stream)
{
    owner_ = owner;
    stream_ = stream;

    buffer_.setSize((int)stream->getSize());
    stream->setPosition(0);
    stream->read(buffer_.data(), buffer_.getSize());

    position_ = 0;
}

XmlReader::~XmlReader()
{
    // nothing
}

void XmlReader::readHeader(std::string& version, std::string& encoding)
{
    std::string s1, s2, s3;

    readXmlData(s1, s2, s3);
    if (s1.find("xml", 0) != 0)
        iseThrowException(SEM_INVALID_XML_FILE_FORMAT);

    XmlNode Node;
    const char* STR_VERSION = "version";
    const char* STR_ENCODING = "encoding";

    Node.getProps()->setPropString(s2);
    if (Node.getProps()->propExists(STR_VERSION))
        version = Node.getProps()->valueOf(STR_VERSION);
    if (Node.getProps()->propExists(STR_ENCODING))
        encoding = Node.getProps()->valueOf(STR_ENCODING);
}

void XmlReader::readRootNode(XmlNode *node)
{
    readNode(node);
}

//-----------------------------------------------------------------------------
// 描述: 读取 XML 标签
//-----------------------------------------------------------------------------
XML_TAG_TYPES XmlReader::readXmlData(std::string& name, std::string& prop, std::string& data)
{
    XML_TAG_TYPES result = XTT_START_TAG;
    enum { FIND_LEFT, FIND_RIGHT, FIND_DATA, FIND_COMMENT, DONE } state;
    char c;
    int i, n, bufferSize, comment;

    name = "";
    prop = "";
    data = "";
    comment = 0;
    bufferSize = buffer_.getSize();
    state = FIND_LEFT;

    while (position_ < bufferSize && state != DONE)
    {
        c = buffer_[position_];
        position_++;

        switch (state)
        {
        case FIND_LEFT:
            if (c == '<') state = FIND_RIGHT;
            break;

        case FIND_RIGHT:
            if (c == '>')
                state = FIND_DATA;
            else if (c == '<')
                iseThrowException(SEM_INVALID_XML_FILE_FORMAT);
            else
            {
                name += c;
                if (name == "!--")
                {
                    state = FIND_COMMENT;
                    comment = 0;
                    name.clear();
                }
            }
            break;

        case FIND_DATA:
            if (c == '<')
            {
                position_--;
                state = DONE;
                break;
            }
            else
                data += c;
            break;

        case FIND_COMMENT:
            if (comment == 2)
            {
                if (c == '>') state = FIND_LEFT;
            }
            else if (c == '-')
                comment++;
            else
                comment = 0;
            break;

        default:
            break;
        }
    }

    if (state == FIND_RIGHT)
        iseThrowException(SEM_INVALID_XML_FILE_FORMAT);

    name = trimString(name);
    n = (int)name.size();
    if (n > 0 && name[n - 1] == '/')
    {
        name.resize(n - 1);
        n--;
        result |= XTT_END_TAG;
    }
    if (n > 0 && name[0] == '/')
    {
        name.erase(0, 1);
        n--;
        result = XTT_END_TAG;
    }
    if (n <= 0)
        result = 0;

    if (result != XTT_START_TAG)
        data.clear();

    if ((result & XTT_START_TAG) != 0)
    {
        i = findChars(name, " \t");
        if (i >= 0)
        {
            prop = trimString(name.substr(i + 1));
            if (!prop.empty() && prop[prop.size()-1] == '?')
                prop.erase(prop.size() - 1);

            name.erase(i);
            name = trimString(name);
            if (!name.empty() && name[0] == '?')
                name.erase(0, 1);
        }
        data = trimString(data);
    }

    return result;
}

XML_TAG_TYPES XmlReader::readNode(XmlNode *node)
{
    XML_TAG_TYPES result, tagTypes;
    XmlNode *childNode;
    std::string name, prop, data;
    std::string endName;

    result = readXmlData(name, prop, data);

    node->setName(name);
    if ((result & XTT_START_TAG) != 0)
    {
        node->getProps()->setPropString(prop);
        node->setDataString(data);
    }
    else
        return result;

    if ((result & XTT_END_TAG) != 0) return result;

    do
    {
        childNode = new XmlNode();
        try
        {
            tagTypes = 0;
            tagTypes = readNode(childNode);
        }
        catch (Exception&)
        {
            delete childNode;
            throw;
        }

        if ((tagTypes & XTT_START_TAG) != 0)
            node->addNode(childNode);
        else
        {
            endName = childNode->getName();
            delete childNode;
        }
    }
    while ((tagTypes & XTT_START_TAG) != 0);

    if (!sameText(name, endName))
        iseThrowException(SEM_INVALID_XML_FILE_FORMAT);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// XmlWriter

XmlWriter::XmlWriter(XmlDocument *owner, Stream *stream)
{
    owner_ = owner;
    stream_ = stream;
}

XmlWriter::~XmlWriter()
{
    // nothing
}

void XmlWriter::writeHeader(const std::string& version, const std::string& encoding)
{
    std::string header, ver, enc;

    ver = formatString("version=\"%s\"", version.c_str());
    if (!encoding.empty())
        enc = formatString("encoding=\"%s\"", encoding.c_str());

    header = "<?xml";
    if (!ver.empty()) header += " ";
    header += ver;
    if (!enc.empty()) header += " ";
    header += enc;
    header += "?>";

    writeLn(header);
}

void XmlWriter::writeRootNode(XmlNode *node)
{
    writeNode(node, 0);
    flushBuffer();
}

void XmlWriter::flushBuffer()
{
    if (!buffer_.empty())
        stream_->writeBuffer(buffer_.c_str(), (int)buffer_.size());
    buffer_.clear();
}

void XmlWriter::writeLn(const std::string& str)
{
    std::string s = str;

    buffer_ += s;
    if (owner_->getAutoIndent())
        buffer_ += S_CRLF;
    flushBuffer();
}

void XmlWriter::writeNode(XmlNode *node, int indent)
{
    std::string s;

    if (!owner_->getAutoIndent())
        indent = 0;

    if (node->getProps()->getCount() > 0)
    {
        s = node->getProps()->getPropString();
        if (s.empty() || s[0] != ' ')
            s = std::string(" ") + s;
    }

    if (node->getDataString().size() > 0 && node->getChildCount() == 0)
        s = s + ">" + strToXml(node->getDataString()) + "</" + node->getName() + ">";
    else if (node->getChildCount() == 0)
        s = s + "/>";
    else
        s = s + ">";

    s = std::string(indent, ' ') + "<" + node->getName() + s;
    writeLn(s);

    for (int i = 0; i < node->getChildCount(); i++)
        writeNode(node->getChildNodes(i), indent + owner_->getIndentSpaces());

    if (node->getChildCount() > 0)
    {
        s = std::string(indent, ' ') + "</" + node->getName() + ">";
        writeLn(s);
    }
}

///////////////////////////////////////////////////////////////////////////////
// class XmlDocParser

XmlDocParser::XmlDocParser()
{
    // nothing
}

XmlDocParser::~XmlDocParser()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 从外部配置文件载入配置信息
// 参数:
//   fileName - 配置文件名(含路径)
//-----------------------------------------------------------------------------
bool XmlDocParser::loadFromFile(const std::string& fileName)
{
    XmlDocument xmlDoc;
    bool result = xmlDoc.loadFromFile(fileName);
    if (result)
        xmlDoc_ = xmlDoc;
    return result;
}

//-----------------------------------------------------------------------------
// 描述: 根据名称路径取得配置字符串
//-----------------------------------------------------------------------------
std::string XmlDocParser::getString(const std::string& namePath)
{
    std::string result;
    StrList nameList;
    XmlNode *node, *resultNode;

    splitNamePath(namePath, nameList);
    if (nameList.getCount() == 0) return result;

    node = xmlDoc_.getRootNode();
    for (int i = 0; (i < nameList.getCount() - 1) && node; i++)
        node = node->findChildNode(nameList[i]);

    if (node)
    {
        std::string strLastName = nameList[nameList.getCount() - 1];

        // 名称路径中的最后一个名称既可是节点名，也可以是属性名
        resultNode = node->findChildNode(strLastName);
        if (resultNode)
            result = resultNode->getDataString();
        else if (node->getProps()->propExists(strLastName))
            result = node->getProps()->valueOf(strLastName);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 根据名称路径取得配置字符串(以整型返回)
//-----------------------------------------------------------------------------
int XmlDocParser::getInteger(const std::string& namePath, int defaultVal)
{
    return strToInt(getString(namePath), defaultVal);
}

//-----------------------------------------------------------------------------
// 描述: 根据名称路径取得配置字符串(以浮点型返回)
//-----------------------------------------------------------------------------
double XmlDocParser::getFloat(const std::string& namePath, double defaultVal)
{
    return strToFloat(getString(namePath), defaultVal);
}

//-----------------------------------------------------------------------------
// 描述: 根据名称路径取得配置字符串(以布尔型返回)
//-----------------------------------------------------------------------------
bool XmlDocParser::getBoolean(const std::string& namePath, bool defaultVal)
{
    return strToBool(getString(namePath), defaultVal);
}

//-----------------------------------------------------------------------------
// 描述: 拆分名称路径
//-----------------------------------------------------------------------------
void XmlDocParser::splitNamePath(const std::string& namePath, StrList& result)
{
    const char NAME_PATH_SPLITTER = '.';
    splitString(namePath, NAME_PATH_SPLITTER, result);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace utils

} // namespace ise
