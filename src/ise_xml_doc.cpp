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
// 文件名称: ise_xml_doc.cpp
// 功能描述: XML文档支持
///////////////////////////////////////////////////////////////////////////////

#include "ise_xml_doc.h"
#include "ise_exceptions.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 杂项函数

//-----------------------------------------------------------------------------
// 描述: 任意字符串 -> 合法的XML字符串
//-----------------------------------------------------------------------------
string StrToXml(const string& str)
{
	string s = str;

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
			string strRep;
			strRep = string("#") + ise::IntToStr(s[i]) + ";";
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
string XmlToStr(const string& str)
{
	string s = str;
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
					h = StrToInt(s.substr(i + 2, j - i - 2).c_str());
					s.erase(i, j - i);
					s[i] = h;
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
int FindChars(const string& s, const string& chars)
{
	int nResult = -1;
	int nLen = (int)s.length();

	for (int i = 0; i < nLen; i++)
		if (chars.find(s[i], 0) != string::npos)
		{
			nResult = i;
			break;
		}

	return nResult;
}

///////////////////////////////////////////////////////////////////////////////
// CXmlNode

CXmlNode::CXmlNode() :
	m_pParentNode(NULL),
	m_pChildNodes(NULL),
	m_pProps(NULL)
{
	InitObject();
}

CXmlNode::CXmlNode(const CXmlNode& src)
{
	InitObject();
	*this = src;
}

CXmlNode::~CXmlNode()
{
	Clear();
	delete m_pProps;
	if (m_pParentNode && m_pParentNode->m_pChildNodes)
		m_pParentNode->m_pChildNodes->Remove(this);
}

void CXmlNode::InitObject()
{
	m_pProps = new CXmlNodeProps();
}

void CXmlNode::AssignNode(CXmlNode& Src, CXmlNode& Dest)
{
	Dest.m_strName = Src.m_strName;
	Dest.m_strDataString = Src.m_strDataString;
	*(Dest.m_pProps) = *(Src.m_pProps);
	for (int i = 0; i < Src.GetChildCount(); i++)
		AssignNode(*(Src.GetChildNodes(i)), *(Dest.AddNode()));
}

CXmlNode& CXmlNode::operator = (const CXmlNode& rhs)
{
	if (this == &rhs) return *this;

	Clear();
	AssignNode((CXmlNode&)rhs, *this);
	return *this;
}

//-----------------------------------------------------------------------------
// 描述: 在当前节点下添加子节点(Node)
//-----------------------------------------------------------------------------
void CXmlNode::AddNode(CXmlNode *pNode)
{
	if (!m_pChildNodes)
		m_pChildNodes = new CList();

	m_pChildNodes->Add(pNode);
	if (pNode->m_pParentNode != NULL)
		pNode->m_pParentNode->m_pChildNodes->Remove(pNode);
	pNode->m_pParentNode = this;
}

//-----------------------------------------------------------------------------
// 描述: 在当前节点下添加一个新的子节点
//-----------------------------------------------------------------------------
CXmlNode* CXmlNode::AddNode()
{
	CXmlNode *pNode = new CXmlNode();
	AddNode(pNode);
	return pNode;
}

//-----------------------------------------------------------------------------
// 描述: 在当前节点下添加一个新的子节点
//-----------------------------------------------------------------------------
CXmlNode* CXmlNode::AddNode(const string& strName, const string& strDataString)
{
	CXmlNode *pNode = AddNode();
	pNode->SetName(strName);
	pNode->SetDataString(strDataString);
	return pNode;
}

//-----------------------------------------------------------------------------
// 描述: 在当前节点下插入子节点
//-----------------------------------------------------------------------------
void CXmlNode::InsertNode(int nIndex, CXmlNode *pNode)
{
	AddNode(pNode);
	m_pChildNodes->Delete(m_pChildNodes->GetCount() - 1);
	m_pChildNodes->Insert(nIndex, pNode);
}

//-----------------------------------------------------------------------------
// 描述: 根据子节点名称，查找子节点。若未找到则返回 NULL。
//-----------------------------------------------------------------------------
CXmlNode* CXmlNode::FindChildNode(const string& strName)
{
	CXmlNode *pResult;
	int i;

	i = IndexOf(strName);
	if (i >= 0)
		pResult = GetChildNodes(i);
	else
		pResult = NULL;

	return pResult;
}

//-----------------------------------------------------------------------------
// 描述: 在当前节点下查找名称为 strName 的节点，返回其序号(0-based)。若未找到则返回-1。
//-----------------------------------------------------------------------------
int CXmlNode::IndexOf(const string& strName)
{
	int nResult = -1;

	for (int i = 0; i < GetChildCount(); i++)
		if (SameText(strName, GetChildNodes(i)->GetName()))
		{
			nResult = i;
			break;
		}

	return nResult;
}

//-----------------------------------------------------------------------------
// 描述: 清空当前节点及其所有子节点的数据
//-----------------------------------------------------------------------------
void CXmlNode::Clear()
{
	if (m_pChildNodes)
	{
		while (m_pChildNodes->GetCount() > 0)
			delete (CXmlNode*)(*m_pChildNodes)[0];
		delete m_pChildNodes;
		m_pChildNodes = NULL;
	}

	m_strName.clear();
	m_strDataString.clear();
	m_pProps->Clear();
}

CXmlNode* CXmlNode::GetRootNode() const
{
	CXmlNode *pResult = (CXmlNode*)this;

	while (pResult->GetParentNode() != NULL)
		pResult = pResult->GetParentNode();

	return pResult;
}

int CXmlNode::GetChildCount() const
{
	if (!m_pChildNodes)
		return 0;
	else
		return m_pChildNodes->GetCount();
}

CXmlNode* CXmlNode::GetChildNodes(int nIndex) const
{
	if (!m_pChildNodes)
		return NULL;
	else
		return (CXmlNode*)(*m_pChildNodes)[nIndex];
}

void CXmlNode::SetParentNode(CXmlNode *pNode)
{
	if (m_pParentNode && m_pParentNode->m_pChildNodes)
		m_pParentNode->m_pChildNodes->Remove(this);
	m_pParentNode = pNode;
}

void CXmlNode::SetDataString(const string& strValue)
{
	m_strDataString = XmlToStr(strValue);
}

///////////////////////////////////////////////////////////////////////////////
// CXmlNodeProps

CXmlNodeProps::CXmlNodeProps()
{
	// nothing
}

CXmlNodeProps::CXmlNodeProps(const CXmlNodeProps& src)
{
	*this = src;
}

CXmlNodeProps::~CXmlNodeProps()
{
	Clear();
}

void CXmlNodeProps::ParsePropString(const string& strPropStr)
{
	string s, strName, strValue;

	Clear();
	s = strPropStr;
	while (true)
	{
		string::size_type i = s.find('=', 0);
		if (i != string::npos)
		{
			strName = TrimString(s.substr(0, i));
			s.erase(0, i + 1);
			s = TrimString(s);
			if (s.size() > 0 && s[0] == '\"')
			{
				s.erase(0, 1);
				i = s.find('\"', 0);
				if (i != string::npos)
				{
					strValue = XmlToStr(s.substr(0, i));
					s.erase(0, i + 1);
				}
				else
					IseThrowException(SEM_INVALID_XML_FILE_FORMAT);
			}
			else
				IseThrowException(SEM_INVALID_XML_FILE_FORMAT);

			Add(strName, strValue);
		}
		else
		{
			if (TrimString(s).size() > 0)
				IseThrowException(SEM_INVALID_XML_FILE_FORMAT);
			else
				break;
		}
	}
}

CXmlNodeProps& CXmlNodeProps::operator = (const CXmlNodeProps& rhs)
{
	if (this == &rhs) return *this;

	Clear();
	for (int i = 0; i < rhs.GetCount(); i++)
	{
		CXmlNodePropItem Item = rhs.GetItems(i);
		Add(Item.strName, Item.strValue);
	}

	return *this;
}

bool CXmlNodeProps::Add(const string& strName, const string& strValue)
{
	bool bResult;

	bResult = (IndexOf(strName) < 0);
	if (bResult)
	{
		CXmlNodePropItem *pItem = new CXmlNodePropItem();
		pItem->strName = strName;
		pItem->strValue = strValue;
		m_Items.Add(pItem);
	}

	return bResult;
}

void CXmlNodeProps::Remove(const string& strName)
{
	int i;

	i = IndexOf(strName);
	if (i >= 0)
	{
		delete GetItemPtr(i);
		m_Items.Delete(i);
	}
}

void CXmlNodeProps::Clear()
{
	for (int i = 0; i < m_Items.GetCount(); i++)
		delete GetItemPtr(i);
	m_Items.Clear();
}

int CXmlNodeProps::IndexOf(const string& strName)
{
	int nResult = -1;

	for (int i = 0; i < m_Items.GetCount(); i++)
		if (SameText(strName, GetItemPtr(i)->strName))
		{
			nResult = i;
			break;
		}

	return nResult;
}

bool CXmlNodeProps::PropExists(const string& strName)
{
	return (IndexOf(strName) >= 0);
}

string& CXmlNodeProps::ValueOf(const string& strName)
{
	int i;

	i = IndexOf(strName);
	if (i >= 0)
		return GetItemPtr(i)->strValue;
	else
	{
		Add(strName, "");
		return GetItemPtr(GetCount()-1)->strValue;
	}
}

CXmlNodePropItem CXmlNodeProps::GetItems(int nIndex) const
{
	CXmlNodePropItem Result;

	if (nIndex >= 0 && nIndex < m_Items.GetCount())
		Result = *(((CXmlNodeProps*)this)->GetItemPtr(nIndex));

	return Result;
}

string CXmlNodeProps::GetPropString() const
{
	string strResult;
	CXmlNodePropItem *pItem;

	for (int i = 0; i < m_Items.GetCount(); i++)
	{
		pItem = ((CXmlNodeProps*)this)->GetItemPtr(i);
		if (i > 0) strResult = strResult + " ";
		strResult = strResult + FormatString("%s=\"%s\"", pItem->strName.c_str(), pItem->strValue.c_str());
	}

	return strResult;
}

void CXmlNodeProps::SetPropString(const string& strPropString)
{
	ParsePropString(strPropString);
}

///////////////////////////////////////////////////////////////////////////////
// CXmlDocument

CXmlDocument::CXmlDocument() :
	m_bAutoIndent(true),
	m_nIndentSpaces(DEF_XML_INDENT_SPACES)
{
	// nothing
}

CXmlDocument::CXmlDocument(const CXmlDocument& src)
{
	*this = src;
}

CXmlDocument::~CXmlDocument()
{
	// nothing
}

CXmlDocument& CXmlDocument::operator = (const CXmlDocument& rhs)
{
	if (this == &rhs) return *this;

	m_bAutoIndent = rhs.m_bAutoIndent;
	m_nIndentSpaces = rhs.m_nIndentSpaces;
	m_RootNode = rhs.m_RootNode;

	return *this;
}

bool CXmlDocument::SaveToStream(CStream& Stream)
{
	try
	{
		CXmlWriter Writer(this, &Stream);
		Writer.WriteHeader(S_DEF_XML_DOC_VER, m_strEncoding);
		Writer.WriteRootNode(&m_RootNode);
		return true;
	}
	catch (CException&)
	{
		return false;
	}
}

bool CXmlDocument::LoadFromStream(CStream& Stream)
{
	try
	{
		string strVersion, strEncoding;
		CXmlReader Reader(this, &Stream);

		m_RootNode.Clear();
		Reader.ReadHeader(strVersion, strEncoding);
		Reader.ReadRootNode(&m_RootNode);

		m_strEncoding = strEncoding;
		return true;
	}
	catch (CException&)
	{
		return false;
	}
}

bool CXmlDocument::SaveToString(string& str)
{
	CMemoryStream Stream;
	bool bResult = SaveToStream(Stream);
	str.assign(Stream.GetMemory(), (int)Stream.GetSize());
	return bResult;
}

bool CXmlDocument::LoadFromString(const string& str)
{
	CMemoryStream Stream;
	Stream.Write(str.c_str(), (int)str.size());
	Stream.SetPosition(0);
	return LoadFromStream(Stream);
}

bool CXmlDocument::SaveToFile(const string& strFileName)
{
	CFileStream fs;
	bool bResult = fs.Open(strFileName, FM_CREATE | FM_SHARE_DENY_WRITE);
	if (bResult)
		bResult = SaveToStream(fs);
	return bResult;
}

bool CXmlDocument::LoadFromFile(const string& strFileName)
{
	CFileStream fs;
	bool bResult = fs.Open(strFileName, FM_OPEN_READ | FM_SHARE_DENY_NONE);
	if (bResult)
		bResult = LoadFromStream(fs);
	return bResult;
}

void CXmlDocument::Clear()
{
	m_RootNode.Clear();
}

///////////////////////////////////////////////////////////////////////////////
// CXmlReader

CXmlReader::CXmlReader(CXmlDocument *pOwner, CStream *pStream)
{
	m_pOwner = pOwner;
	m_pStream = pStream;

	m_Buffer.SetSize((int)pStream->GetSize());
	pStream->SetPosition(0);
	pStream->Read(m_Buffer.Data(), m_Buffer.GetSize());

	m_nPosition = 0;
}

CXmlReader::~CXmlReader()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 读取 XML 标签
//-----------------------------------------------------------------------------
XML_TAG_TYPES CXmlReader::ReadXmlData(string& strName, string& strProp, string& strData)
{
	XML_TAG_TYPES nResult = XTT_START_TAG;
	enum { FIND_LEFT, FIND_RIGHT, FIND_DATA, FIND_COMMENT, DONE } nState;
	char c;
	int i, n, nBufferSize, nComment;

	strName = "";
	strProp = "";
	strData = "";
	nComment = 0;
	nBufferSize = m_Buffer.GetSize();
	nState = FIND_LEFT;

	while (m_nPosition < nBufferSize && nState != DONE)
	{
		c = m_Buffer[m_nPosition];
		m_nPosition++;

		switch (nState)
		{
		case FIND_LEFT:
			if (c == '<') nState = FIND_RIGHT;
			break;

		case FIND_RIGHT:
			if (c == '>')
				nState = FIND_DATA;
			else if (c == '<')
				IseThrowException(SEM_INVALID_XML_FILE_FORMAT);
			else
			{
				strName += c;
				if (strName == "!--")
				{
					nState = FIND_COMMENT;
					nComment = 0;
					strName.clear();
				}
			}
			break;

		case FIND_DATA:
			if (c == '<')
			{
				m_nPosition--;
				nState = DONE;
				break;
			}
			else
				strData += c;
			break;

		case FIND_COMMENT:
			if (nComment == 2)
			{
				if (c == '>') nState = FIND_LEFT;
			}
			else if (c == '-')
				nComment++;
			else
				nComment = 0;
			break;

		default:
			break;
		}
	}

	if (nState == FIND_RIGHT)
		IseThrowException(SEM_INVALID_XML_FILE_FORMAT);

	strName = TrimString(strName);
	n = (int)strName.size();
	if (n > 0 && strName[n - 1] == '/')
	{
		strName.resize(n - 1);
		n--;
		nResult |= XTT_END_TAG;
	}
	if (n > 0 && strName[0] == '/')
	{
		strName.erase(0, 1);
		n--;
		nResult = XTT_END_TAG;
	}
	if (n <= 0)
		nResult = 0;

	if (nResult != XTT_START_TAG)
		strData.clear();

	if ((nResult & XTT_START_TAG) != 0)
	{
		i = FindChars(strName, " \t");
		if (i >= 0)
		{
			strProp = TrimString(strName.substr(i + 1));
			if (!strProp.empty() && strProp[strProp.size()-1] == '?')
				strProp.erase(strProp.size() - 1);

			strName.erase(i);
			strName = TrimString(strName);
			if (!strName.empty() && strName[0] == '?')
				strName.erase(0, 1);
		}
		strData = TrimString(strData);
	}

	return nResult;
}

XML_TAG_TYPES CXmlReader::ReadNode(CXmlNode *pNode)
{
	XML_TAG_TYPES nResult, TagTypes;
	CXmlNode *pChildNode;
	string strName, strProp, strData;
	string strEndName;

	nResult = ReadXmlData(strName, strProp, strData);

	pNode->SetName(strName);
	if ((nResult & XTT_START_TAG) != 0)
	{
		pNode->GetProps()->SetPropString(strProp);
		pNode->SetDataString(strData);
	}
	else
		return nResult;

	if ((nResult & XTT_END_TAG) != 0) return nResult;

	do
	{
		pChildNode = new CXmlNode();
		try
		{
			TagTypes = 0;
			TagTypes = ReadNode(pChildNode);
		}
		catch (CException&)
		{
			delete pChildNode;
			throw;
		}

		if ((TagTypes & XTT_START_TAG) != 0)
			pNode->AddNode(pChildNode);
		else
		{
			strEndName = pChildNode->GetName();
			delete pChildNode;
		}
	}
	while ((TagTypes & XTT_START_TAG) != 0);

	if (!SameText(strName, strEndName))
		IseThrowException(SEM_INVALID_XML_FILE_FORMAT);

	return nResult;
}

void CXmlReader::ReadHeader(string& strVersion, string& strEncoding)
{
	string s1, s2, s3;

	ReadXmlData(s1, s2, s3);
	if (s1.find("xml", 0) != 0)
		IseThrowException(SEM_INVALID_XML_FILE_FORMAT);

	CXmlNode Node;
	const char* STR_VERSION = "version";
	const char* STR_ENCODING = "encoding";

	Node.GetProps()->SetPropString(s2);
	if (Node.GetProps()->PropExists(STR_VERSION))
		strVersion = Node.GetProps()->ValueOf(STR_VERSION);
	if (Node.GetProps()->PropExists(STR_ENCODING))
		strEncoding = Node.GetProps()->ValueOf(STR_ENCODING);
}

void CXmlReader::ReadRootNode(CXmlNode *pNode)
{
	ReadNode(pNode);
}

///////////////////////////////////////////////////////////////////////////////
// CXmlWriter

CXmlWriter::CXmlWriter(CXmlDocument *pOwner, CStream *pStream)
{
	m_pOwner = pOwner;
	m_pStream = pStream;
}

CXmlWriter::~CXmlWriter()
{
	// nothing
}

void CXmlWriter::FlushBuffer()
{
	if (!m_strBuffer.empty())
		m_pStream->WriteBuffer(m_strBuffer.c_str(), (int)m_strBuffer.size());
	m_strBuffer.clear();
}

void CXmlWriter::WriteLn(const string& str)
{
	string s = str;

	m_strBuffer += s;
	if (m_pOwner->GetAutoIndent())
		m_strBuffer += S_CRLF;
	FlushBuffer();
}

void CXmlWriter::WriteNode(CXmlNode *pNode, int nIndent)
{
	string s;

	if (!m_pOwner->GetAutoIndent())
		nIndent = 0;

	if (pNode->GetProps()->GetCount() > 0)
	{
		s = pNode->GetProps()->GetPropString();
		if (s.empty() || s[0] != ' ')
			s = string(" ") + s;
	}

	if (pNode->GetDataString().size() > 0 && pNode->GetChildCount() == 0)
		s = s + ">" + StrToXml(pNode->GetDataString()) + "</" + pNode->GetName() + ">";
	else if (pNode->GetChildCount() == 0)
		s = s + "/>";
	else
		s = s + ">";

	s = string(nIndent, ' ') + "<" + pNode->GetName() + s;
	WriteLn(s);

	for (int i = 0; i < pNode->GetChildCount(); i++)
		WriteNode(pNode->GetChildNodes(i), nIndent + m_pOwner->GetIndentSpaces());

	if (pNode->GetChildCount() > 0)
	{
		s = string(nIndent, ' ') + "</" + pNode->GetName() + ">";
		WriteLn(s);
	}
}

void CXmlWriter::WriteHeader(const string& strVersion, const string& strEncoding)
{
	string strHeader, strVer, strEnc;

	strVer = FormatString("version=\"%s\"", strVersion.c_str());
	if (!strEncoding.empty())
		strEnc = FormatString("encoding=\"%s\"", strEncoding.c_str());

	strHeader = "<?xml";
	if (!strVer.empty()) strHeader += " ";
	strHeader += strVer;
	if (!strEnc.empty()) strHeader += " ";
	strHeader += strEnc;
	strHeader += "?>";

	WriteLn(strHeader);
}

void CXmlWriter::WriteRootNode(CXmlNode *pNode)
{
	WriteNode(pNode, 0);
	FlushBuffer();
}

///////////////////////////////////////////////////////////////////////////////
// class CXmlDocParser

CXmlDocParser::CXmlDocParser()
{
	// nothing
}

CXmlDocParser::~CXmlDocParser()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 拆分名称路径
//-----------------------------------------------------------------------------
void CXmlDocParser::SplitNamePath(const string& strNamePath, CStrList& Result)
{
	const char NAME_PATH_SPLITTER = '.';
	SplitString(strNamePath, NAME_PATH_SPLITTER, Result);
}

//-----------------------------------------------------------------------------
// 描述: 从外部配置文件载入配置信息
// 参数:
//   strFileName - 配置文件名(含路径)
//-----------------------------------------------------------------------------
bool CXmlDocParser::LoadFromFile(const string& strFileName)
{
	CXmlDocument XmlDoc;
	bool bResult = XmlDoc.LoadFromFile(strFileName);
	if (bResult)
		m_XmlDoc = XmlDoc;
	return bResult;
}

//-----------------------------------------------------------------------------
// 描述: 根据名称路径取得配置字符串
//-----------------------------------------------------------------------------
string CXmlDocParser::GetString(const string& strNamePath)
{
	string strResult;
	CStrList NameList;
	CXmlNode *pNode, *pResultNode;

	SplitNamePath(strNamePath, NameList);
	if (NameList.GetCount() == 0) return strResult;

	pNode = m_XmlDoc.GetRootNode();
	for (int i = 0; (i < NameList.GetCount() - 1) && pNode; i++)
		pNode = pNode->FindChildNode(NameList[i]);

	if (pNode)
	{
		string strLastName = NameList[NameList.GetCount() - 1];

		// 名称路径中的最后一个名称既可是节点名，也可以是属性名
		pResultNode = pNode->FindChildNode(strLastName);
		if (pResultNode)
			strResult = pResultNode->GetDataString();
		else if (pNode->GetProps()->PropExists(strLastName))
			strResult = pNode->GetProps()->ValueOf(strLastName);
	}

	return strResult;
}

//-----------------------------------------------------------------------------
// 描述: 根据名称路径取得配置字符串(以整型返回)
//-----------------------------------------------------------------------------
int CXmlDocParser::GetInteger(const string& strNamePath, int nDefault)
{
	return StrToInt(GetString(strNamePath), nDefault);
}

//-----------------------------------------------------------------------------
// 描述: 根据名称路径取得配置字符串(以浮点型返回)
//-----------------------------------------------------------------------------
double CXmlDocParser::GetFloat(const string& strNamePath, double fDefault)
{
	return StrToFloat(GetString(strNamePath), fDefault);
}

//-----------------------------------------------------------------------------
// 描述: 根据名称路径取得配置字符串(以布尔型返回)
//-----------------------------------------------------------------------------
bool CXmlDocParser::GetBoolean(const string& strNamePath, bool bDefault)
{
	return StrToBool(GetString(strNamePath), bDefault);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
