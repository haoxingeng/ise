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
// ise_xml.h
// Classes:
//   * XmlNode
//   * XmlNodeProps
//   * XmlDocument
//   * XmlReader
//   * XmlWriter
//   * XmlDocParser
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_EXT_UTILS_XML_H_
#define _ISE_EXT_UTILS_XML_H_

#include "ise/main/ise_options.h"
#include "ise/main/ise_global_defs.h"
#include "ise/main/ise_err_msgs.h"
#include "ise/main/ise_classes.h"
#include "ise/main/ise_sys_utils.h"

namespace ise
{

namespace utils
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class XmlNode;
class XmlNodeProps;

///////////////////////////////////////////////////////////////////////////////
// 类型定义

// XML标签类别
enum XML_TAG_TYPE
{
    XTT_START_TAG   = 0x01,      // 起始标签
    XTT_END_TAG     = 0x02,      // 结束标签
};

typedef UINT XML_TAG_TYPES;

///////////////////////////////////////////////////////////////////////////////
// 常量定义

const int DEF_XML_INDENT_SPACES = 4;
const char* const S_DEF_XML_DOC_VER = "1.0";

///////////////////////////////////////////////////////////////////////////////
// 杂项函数

string strToXml(const string& str);
string xmlToStr(const string& str);

///////////////////////////////////////////////////////////////////////////////
// class XmlNode

class XmlNode
{
public:
    XmlNode();
    XmlNode(const XmlNode& src);
    virtual ~XmlNode();

    XmlNode& operator = (const XmlNode& rhs);

    void addNode(XmlNode *node);
    XmlNode* addNode();
    XmlNode* addNode(const string& name, const string& dataString);
    void insertNode(int index, XmlNode *node);

    XmlNode* findChildNode(const string& name);
    int indexOf(const string& name);
    void clear();

    XmlNode* getRootNode() const;
    XmlNode* getParentNode() const { return parentNode_; }
    int getChildCount() const;
    XmlNode* getChildNodes(int index) const;
    XmlNodeProps* getProps() const { return props_; }
    string getName() const { return name_; }
    string getDataString() const { return dataString_; }

    void setParentNode(XmlNode *node);
    void setName(const string& value) { name_ = value; }
    void setDataString(const string& value);

private:
    void initObject();
    void assignNode(XmlNode& src, XmlNode& dest);

private:
    XmlNode *parentNode_;       // 父节点
    PointerList *childNodes_;   // 子节点列表 (XmlNode*)[]
    XmlNodeProps *props_;       // 属性值列表
    string name_;          // 节点名称
    string dataString_;    // 节点数据(xmlToStr之后的数据)
};

///////////////////////////////////////////////////////////////////////////////
// class XmlNodeProps

struct XmlNodePropItem
{
    string name;
    string value;
};

class XmlNodeProps
{
public:
    XmlNodeProps();
    XmlNodeProps(const XmlNodeProps& src);
    virtual ~XmlNodeProps();

    XmlNodeProps& operator = (const XmlNodeProps& rhs);
    string& operator[](const string& name) { return valueOf(name); }

    bool add(const string& name, const string& value);
    void remove(const string& name);
    void clear();
    int indexOf(const string& name);
    bool propExists(const string& name);
    string& valueOf(const string& name);

    int getCount() const { return items_.getCount(); }
    XmlNodePropItem getItems(int index) const;
    string getPropString() const;
    void setPropString(const string& propString);

private:
    XmlNodePropItem* getItemPtr(int index) { return (XmlNodePropItem*)items_[index]; }
    void parsePropString(const string& propStr);

private:
    PointerList items_;      // (XmlNodePropItem*)[]
};

///////////////////////////////////////////////////////////////////////////////
// class XmlDocument

class XmlDocument
{
public:
    XmlDocument();
    XmlDocument(const XmlDocument& src);
    virtual ~XmlDocument();

    XmlDocument& operator = (const XmlDocument& rhs);

    bool saveToStream(Stream& stream);
    bool loadFromStream(Stream& stream);
    bool saveToString(string& str);
    bool loadFromString(const string& str);
    bool saveToFile(const string& fileName);
    bool loadFromFile(const string& fileName);
    void clear();

    bool getAutoIndent() const { return autoIndent_; }
    int getIndentSpaces() const { return indentSpaces_; }
    string getEncoding() const { return encoding_; }
    XmlNode* getRootNode() { return &rootNode_; }

    void setAutoIndent(bool value) { autoIndent_ = value; }
    void setIndentSpaces(int value) { indentSpaces_ = value; }
    void setEncoding(const string& value) { encoding_ = value; }

private:
    bool autoIndent_;       // 是否自动缩进
    int indentSpaces_;      // 缩进空格数
    XmlNode rootNode_;      // 根节点
    string encoding_;  // XML编码
};

///////////////////////////////////////////////////////////////////////////////
// class XmlReader

class XmlReader : boost::noncopyable
{
public:
    XmlReader(XmlDocument *owner, Stream *stream);
    virtual ~XmlReader();

    void readHeader(string& version, string& encoding);
    void readRootNode(XmlNode *node);

private:
    XML_TAG_TYPES readXmlData(string& name, string& prop, string& data);
    XML_TAG_TYPES readNode(XmlNode *node);

private:
    XmlDocument *owner_;
    Stream *stream_;
    Buffer buffer_;
    int position_;
};

///////////////////////////////////////////////////////////////////////////////
// class XmlWriter

class XmlWriter : boost::noncopyable
{
public:
    XmlWriter(XmlDocument *owner, Stream *stream);
    virtual ~XmlWriter();

    void writeHeader(const string& version = S_DEF_XML_DOC_VER, const string& encoding = "");
    void writeRootNode(XmlNode *node);

private:
    void flushBuffer();
    void writeLn(const string& str);
    void writeNode(XmlNode *node, int indent);

private:
    XmlDocument *owner_;
    Stream *stream_;
    string buffer_;
};

///////////////////////////////////////////////////////////////////////////////
// class XmlDocParser - XML文档解读器
//
// 名词解释:
//   NamePath: XML值的名称路径。它由一系列XML节点名组成，各名称之间用点号(.)分隔，最后
//   一个名称可以是节点的属性名。XML的根节点名称在名称路径中被省略。名称路径不区分大小写。
//   须注意的是，由于名称之间用点号分隔，所以各名称内部不应含有点号，以免混淆。
//   示例: "Database.MainDb.HostName"

class XmlDocParser : boost::noncopyable
{
public:
    XmlDocParser();
    virtual ~XmlDocParser();

    bool loadFromFile(const string& fileName);

    string getString(const string& namePath);
    int getInteger(const string& namePath, int defaultVal = 0);
    double getFloat(const string& namePath, double defaultVal = 0);
    bool getBoolean(const string& namePath, bool defaultVal = 0);

    XmlDocument& getXmlDoc() { return xmlDoc_; }

private:
    void splitNamePath(const string& namePath, StrList& result);
private:
    XmlDocument xmlDoc_;
};

///////////////////////////////////////////////////////////////////////////////

} // namespace utils

} // namespace ise

#endif // _ISE_EXT_UTILS_XML_H_
