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
// ise_xml_doc.h
// Classes:
//   * CXmlNode
//   * CXmlNodeProps
//   * CXmlDocument
//   * CXmlReader
//   * CXmlWriter
//   * CXmlDocParser
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_XML_DOC_H_
#define _ISE_XML_DOC_H_

#include "ise_options.h"
#include "ise_global_defs.h"
#include "ise_errmsgs.h"
#include "ise_classes.h"
#include "ise_sysutils.h"

using std::string;

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class CXmlNode;
class CXmlNodeProps;

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

string StrToXml(const string& str);
string XmlToStr(const string& str);

///////////////////////////////////////////////////////////////////////////////
// class CXmlNode

class CXmlNode
{
private:
	CXmlNode *m_pParentNode;    // 父节点
	CList *m_pChildNodes;       // 子节点列表 (CXmlNode*)[]
	CXmlNodeProps *m_pProps;    // 属性值列表
	string m_strName;           // 节点名称
	string m_strDataString;     // 节点数据(XmlToStr之后的数据)
private:
	void InitObject();
	void AssignNode(CXmlNode& Src, CXmlNode& Dest);
public:
	CXmlNode();
	CXmlNode(const CXmlNode& src);
	virtual ~CXmlNode();

	CXmlNode& operator = (const CXmlNode& rhs);

	void AddNode(CXmlNode *pNode);
	CXmlNode* AddNode();
	CXmlNode* AddNode(const string& strName, const string& strDataString);
	void InsertNode(int nIndex, CXmlNode *pNode);

	CXmlNode* FindChildNode(const string& strName);
	int IndexOf(const string& strName);
	void Clear();

	CXmlNode* GetRootNode() const;
	CXmlNode* GetParentNode() const { return m_pParentNode; }
	int GetChildCount() const;
	CXmlNode* GetChildNodes(int nIndex) const;
	CXmlNodeProps* GetProps() const { return m_pProps; }
	string GetName() const { return m_strName; }
	string GetDataString() const { return m_strDataString; }

	void SetParentNode(CXmlNode *pNode);
	void SetName(const string& strValue) { m_strName = strValue; }
	void SetDataString(const string& strValue);
};

///////////////////////////////////////////////////////////////////////////////
// class CXmlNodeProps

struct CXmlNodePropItem
{
	string strName;
	string strValue;
};

class CXmlNodeProps
{
private:
	CList m_Items;      // (CXmlNodePropItem*)[]
private:
	CXmlNodePropItem* GetItemPtr(int nIndex)
		{ return (CXmlNodePropItem*)m_Items[nIndex]; }
	void ParsePropString(const string& strPropStr);
public:
	CXmlNodeProps();
	CXmlNodeProps(const CXmlNodeProps& src);
	virtual ~CXmlNodeProps();

	CXmlNodeProps& operator = (const CXmlNodeProps& rhs);
	string& operator[](const string& strName) { return ValueOf(strName); }

	bool Add(const string& strName, const string& strValue);
	void Remove(const string& strName);
	void Clear();
	int IndexOf(const string& strName);
	bool PropExists(const string& strName);
	string& ValueOf(const string& strName);

	int GetCount() const { return m_Items.GetCount(); }
	CXmlNodePropItem GetItems(int nIndex) const;
	string GetPropString() const;
	void SetPropString(const string& strPropString);
};

///////////////////////////////////////////////////////////////////////////////
// class CXmlDocument

class CXmlDocument
{
private:
	bool m_bAutoIndent;     // 是否自动缩进
	int m_nIndentSpaces;    // 缩进空格数
	CXmlNode m_RootNode;    // 根节点
	string m_strEncoding;   // XML编码
public:
	CXmlDocument();
	CXmlDocument(const CXmlDocument& src);
	virtual ~CXmlDocument();

	CXmlDocument& operator = (const CXmlDocument& rhs);

	bool SaveToStream(CStream& Stream);
	bool LoadFromStream(CStream& Stream);
	bool SaveToString(string& str);
	bool LoadFromString(const string& str);
	bool SaveToFile(const string& strFileName);
	bool LoadFromFile(const string& strFileName);
	void Clear();

	bool GetAutoIndent() const { return m_bAutoIndent; }
	int GetIndentSpaces() const { return m_nIndentSpaces; }
	string GetEncoding() const { return m_strEncoding; }
	CXmlNode* GetRootNode() { return &m_RootNode; }

	void SetAutoIndent(bool bValue) { m_bAutoIndent = bValue; }
	void SetIndentSpaces(int nValue) { m_nIndentSpaces = nValue; }
	void SetEncoding(const string& strValue) { m_strEncoding = strValue; }
};

///////////////////////////////////////////////////////////////////////////////
// class CXmlReader

class CXmlReader
{
private:
	CXmlDocument *m_pOwner;
	CStream *m_pStream;
	CBuffer m_Buffer;
	int m_nPosition;
private:
	XML_TAG_TYPES ReadXmlData(string& strName, string& strProp, string& strData);
	XML_TAG_TYPES ReadNode(CXmlNode *pNode);
public:
	CXmlReader(CXmlDocument *pOwner, CStream *pStream);
	virtual ~CXmlReader();

	void ReadHeader(string& strVersion, string& strEncoding);
	void ReadRootNode(CXmlNode *pNode);
};

///////////////////////////////////////////////////////////////////////////////
// class CXmlWriter

class CXmlWriter
{
private:
	CXmlDocument *m_pOwner;
	CStream *m_pStream;
	string m_strBuffer;
private:
	void FlushBuffer();
	void WriteLn(const string& str);
	void WriteNode(CXmlNode *pNode, int nIndent);
public:
	CXmlWriter(CXmlDocument *pOwner, CStream *pStream);
	virtual ~CXmlWriter();

	void WriteHeader(const string& strVersion = S_DEF_XML_DOC_VER, const string& strEncoding = "");
	void WriteRootNode(CXmlNode *pNode);
};

///////////////////////////////////////////////////////////////////////////////
// class CXmlDocParser - XML文档解读器
//
// 名词解释:
//   NamePath: XML值的名称路径。它由一系列XML节点名组成，各名称之间用点号(.)分隔，最后
//   一个名称可以是节点的属性名。XML的根节点名称在名称路径中被省略。名称路径不区分大小写。
//   须注意的是，由于名称之间用点号分隔，所以各名称内部不应含有点号，以免混淆。
//   示例: "Database.MainDb.HostName"

class CXmlDocParser
{
private:
	CXmlDocument m_XmlDoc;
private:
	void SplitNamePath(const string& strNamePath, CStrList& Result);
public:
	CXmlDocParser();
	virtual ~CXmlDocParser();

	bool LoadFromFile(const string& strFileName);

	string GetString(const string& strNamePath);
	int GetInteger(const string& strNamePath, int nDefault = 0);
	double GetFloat(const string& strNamePath, double fDefault = 0);
	bool GetBoolean(const string& strNamePath, bool bDefault = 0);

	CXmlDocument& GetXmlDoc() { return m_XmlDoc; }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_XML_DOC_H_
