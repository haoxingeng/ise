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
// 文件名称: ise_database.cpp
// 功能描述: 数据库接口(DBI: ISE Database Interface)
///////////////////////////////////////////////////////////////////////////////

#include "ise_database.h"
#include "ise_sysutils.h"
#include "ise_errmsgs.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class CDbConnParams

CDbConnParams::CDbConnParams() :
	m_nPort(0)
{
	// nothing
}

CDbConnParams::CDbConnParams(const CDbConnParams& src)
{
	m_strHostName = src.m_strHostName;
	m_strUserName = src.m_strUserName;
	m_strPassword = src.m_strPassword;
	m_strDbName = src.m_strDbName;
	m_nPort = src.m_nPort;
}

CDbConnParams::CDbConnParams(const string& strHostName, const string& strUserName,
	const string& strPassword, const string& strDbName, int nPort)
{
	m_strHostName = strHostName;
	m_strUserName = strUserName;
	m_strPassword = strPassword;
	m_strDbName = strDbName;
	m_nPort = nPort;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbOptions

CDbOptions::CDbOptions()
{
	SetMaxDbConnections(DEF_MAX_DB_CONNECTIONS);
}

void CDbOptions::SetMaxDbConnections(int nValue)
{
	if (nValue < 1) nValue = 1;
	m_nMaxDbConnections = nValue;
}

void CDbOptions::SetInitialSqlCmd(const string& strValue)
{
	m_InitialSqlCmdList.Clear();
	m_InitialSqlCmdList.Add(strValue.c_str());
}

void CDbOptions::SetInitialCharSet(const string& strValue)
{
	m_strInitialCharSet = strValue;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbConnection

CDbConnection::CDbConnection(CDatabase *pDatabase)
{
	m_pDatabase = pDatabase;
	m_bConnected = false;
	m_bBusy = false;
}

CDbConnection::~CDbConnection()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 建立数据库连接并进行相关设置 (若失败则抛出异常)
//-----------------------------------------------------------------------------
void CDbConnection::Connect()
{
	if (!m_bConnected)
	{
		DoConnect();
		ExecCmdOnConnected();
		m_bConnected = true;
	}
}

//-----------------------------------------------------------------------------
// 描述: 断开数据库连接并进行相关设置
//-----------------------------------------------------------------------------
void CDbConnection::Disconnect()
{
	if (m_bConnected)
	{
		DoDisconnect();
		m_bConnected = false;
	}
}

//-----------------------------------------------------------------------------
// 描述: 刚建立连接时执行命令
//-----------------------------------------------------------------------------
void CDbConnection::ExecCmdOnConnected()
{
	try
	{
		CStrList& CmdList = m_pDatabase->GetDbOptions()->InitialSqlCmdList();
		if (!CmdList.IsEmpty())
		{
			CDbQueryWrapper Query(m_pDatabase->CreateDbQuery());
			Query->m_pDbConnection = this;

			for (int i = 0; i < CmdList.GetCount(); ++i)
			{
				try
				{
					Query->SetSql(CmdList[i]);
					Query->Execute();
				}
				catch (...)
				{}
			}

			Query->m_pDbConnection = NULL;
		}
	}
	catch (...)
	{}
}

//-----------------------------------------------------------------------------
// 描述: 借用连接 (由 ConnectionPool 调用)
//-----------------------------------------------------------------------------
bool CDbConnection::GetDbConnection()
{
	if (!m_bBusy)
	{
		ActivateConnection();
		m_bBusy = true;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// 描述: 归还连接 (由 ConnectionPool 调用)
//-----------------------------------------------------------------------------
void CDbConnection::ReturnDbConnection()
{
	m_bBusy = false;
}

//-----------------------------------------------------------------------------
// 描述: 返回连接是否被借用 (由 ConnectionPool 调用)
//-----------------------------------------------------------------------------
bool CDbConnection::IsBusy()
{
	return m_bBusy;
}

//-----------------------------------------------------------------------------
// 描述: 激活数据库连接
// 参数:
//   bForce - 是否强制激活
//-----------------------------------------------------------------------------
void CDbConnection::ActivateConnection(bool bForce)
{
	// 没有连接数据库则建立连接
	if (!m_bConnected || bForce)
	{
		Disconnect();
		Connect();
		return;
	}
}

///////////////////////////////////////////////////////////////////////////////
// class CDbConnectionPool

CDbConnectionPool::CDbConnectionPool(CDatabase *pDatabase) :
	m_pDatabase(pDatabase)
{
	// nothing
}

CDbConnectionPool::~CDbConnectionPool()
{
	ClearPool();
}

//-----------------------------------------------------------------------------
// 描述: 清空连接池
//-----------------------------------------------------------------------------
void CDbConnectionPool::ClearPool()
{
	CAutoLocker Locker(m_Lock);

	for (int i = 0; i < m_DbConnectionList.GetCount(); i++)
	{
		CDbConnection *pDbConnection;
		pDbConnection = (CDbConnection*)m_DbConnectionList[i];
		pDbConnection->DoDisconnect();
		delete pDbConnection;
	}

	m_DbConnectionList.Clear();
}

//-----------------------------------------------------------------------------
// 描述: 分配一个可用的空闲连接 (若失败则抛出异常)
// 返回: 连接对象指针
//-----------------------------------------------------------------------------
CDbConnection* CDbConnectionPool::GetConnection()
{
	CDbConnection *pDbConnection = NULL;
	bool bResult = false;

	{
		CAutoLocker Locker(m_Lock);

		// 检查现有的连接是否能用
		for (int i = 0; i < m_DbConnectionList.GetCount(); i++)
		{
			pDbConnection = (CDbConnection*)m_DbConnectionList[i];
			bResult = pDbConnection->GetDbConnection();  // 借出连接
			if (bResult) break;
		}

		// 如果借出失败，则增加新的数据库连接到连接池
		if (!bResult && (m_DbConnectionList.GetCount() < m_pDatabase->GetDbOptions()->GetMaxDbConnections()))
		{
			pDbConnection = m_pDatabase->CreateDbConnection();
			m_DbConnectionList.Add(pDbConnection);
			bResult = pDbConnection->GetDbConnection();
		}
	}

	if (!bResult)
		IseThrowDbException(SEM_GET_CONN_FROM_POOL_ERROR);

	return pDbConnection;
}

//-----------------------------------------------------------------------------
// 描述: 归还数据库连接
//-----------------------------------------------------------------------------
void CDbConnectionPool::ReturnConnection(CDbConnection *pDbConnection)
{
	CAutoLocker Locker(m_Lock);
	pDbConnection->ReturnDbConnection();
}

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldDef

CDbFieldDef::CDbFieldDef(const string& strName, int nType)
{
	m_strName = strName;
	m_nType = nType;
}

CDbFieldDef::CDbFieldDef(const CDbFieldDef& src)
{
	m_strName = src.m_strName;
	m_nType = src.m_nType;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldDefList

CDbFieldDefList::CDbFieldDefList()
{
	// nothing
}

CDbFieldDefList::~CDbFieldDefList()
{
	Clear();
}

//-----------------------------------------------------------------------------
// 描述: 添加一个字段定义对象
//-----------------------------------------------------------------------------
void CDbFieldDefList::Add(CDbFieldDef *pFieldDef)
{
	if (pFieldDef != NULL)
		m_Items.Add(pFieldDef);
}

//-----------------------------------------------------------------------------
// 描述: 释放并清空所有字段定义对象
//-----------------------------------------------------------------------------
void CDbFieldDefList::Clear()
{
	for (int i = 0; i < m_Items.GetCount(); i++)
		delete (CDbFieldDef*)m_Items[i];
	m_Items.Clear();
}

//-----------------------------------------------------------------------------
// 描述: 返回字段名对应的字段序号(0-based)，若未找到则返回-1.
//-----------------------------------------------------------------------------
int CDbFieldDefList::IndexOfName(const string& strName)
{
	int nIndex = -1;

	for (int i = 0; i < m_Items.GetCount(); i++)
	{
		if (SameText(((CDbFieldDef*)m_Items[i])->GetName(), strName))
		{
			nIndex = i;
			break;
		}
	}

	return nIndex;
}

//-----------------------------------------------------------------------------
// 描述: 返回全部字段名
//-----------------------------------------------------------------------------
void CDbFieldDefList::GetFieldNameList(CStrList& List)
{
	List.Clear();
	for (int i = 0; i < m_Items.GetCount(); i++)
		List.Add(((CDbFieldDef*)m_Items[i])->GetName().c_str());
}

//-----------------------------------------------------------------------------
// 描述: 根据下标号返回字段定义对象 (nIndex: 0-based)
//-----------------------------------------------------------------------------
CDbFieldDef* CDbFieldDefList::operator[] (int nIndex)
{
	if (nIndex >= 0 && nIndex < m_Items.GetCount())
		return (CDbFieldDef*)m_Items[nIndex];
	else
		return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbField

CDbField::CDbField()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 以整型返回字段值 (若转换失败则返回缺省值)
//-----------------------------------------------------------------------------
int CDbField::AsInteger(int nDefault) const
{
	return StrToInt(AsString(), nDefault);
}

//-----------------------------------------------------------------------------
// 描述: 以64位整型返回字段值 (若转换失败则返回缺省值)
//-----------------------------------------------------------------------------
INT64 CDbField::AsInt64(INT64 nDefault) const
{
	return StrToInt64(AsString(), nDefault);
}

//-----------------------------------------------------------------------------
// 描述: 以浮点型返回字段值 (若转换失败则返回缺省值)
//-----------------------------------------------------------------------------
double CDbField::AsFloat(double fDefault) const
{
	return StrToFloat(AsString(), fDefault);
}

//-----------------------------------------------------------------------------
// 描述: 以布尔型返回字段值 (若转换失败则返回缺省值)
//-----------------------------------------------------------------------------
bool CDbField::AsBoolean(bool bDefault) const
{
	return AsInteger(bDefault? 1 : 0) != 0;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldList

CDbFieldList::CDbFieldList()
{
	// nothing
}

CDbFieldList::~CDbFieldList()
{
	Clear();
}

//-----------------------------------------------------------------------------
// 描述: 添加一个字段数据对象
//-----------------------------------------------------------------------------
void CDbFieldList::Add(CDbField *pField)
{
	m_Items.Add(pField);
}

//-----------------------------------------------------------------------------
// 描述: 释放并清空所有字段数据对象
//-----------------------------------------------------------------------------
void CDbFieldList::Clear()
{
	for (int i = 0; i < m_Items.GetCount(); i++)
		delete (CDbField*)m_Items[i];
	m_Items.Clear();
}

//-----------------------------------------------------------------------------
// 描述: 根据下标号返回字段数据对象 (nIndex: 0-based)
//-----------------------------------------------------------------------------
CDbField* CDbFieldList::operator[] (int nIndex)
{
	if (nIndex >= 0 && nIndex < m_Items.GetCount())
		return (CDbField*)m_Items[nIndex];
	else
		return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbParamList

CDbParamList::CDbParamList(CDbQuery *pDbQuery) :
	m_pDbQuery(pDbQuery)
{
	// nothing
}

CDbParamList::~CDbParamList()
{
	Clear();
}

//-----------------------------------------------------------------------------
// 描述: 根据参数名称在列表中查找参数对象
//-----------------------------------------------------------------------------
CDbParam* CDbParamList::FindParam(const string& strName)
{
	CDbParam *pResult = NULL;

	for (int i = 0; i < m_Items.GetCount(); i++)
		if (SameText(((CDbParam*)m_Items[i])->m_strName, strName))
		{
			pResult = (CDbParam*)m_Items[i];
			break;
		}

	return pResult;
}

//-----------------------------------------------------------------------------
// 描述: 根据参数序号在列表中查找参数对象
//-----------------------------------------------------------------------------
CDbParam* CDbParamList::FindParam(int nNumber)
{
	CDbParam *pResult = NULL;

	for (int i = 0; i < m_Items.GetCount(); i++)
		if (((CDbParam*)m_Items[i])->m_nNumber == nNumber)
		{
			pResult = (CDbParam*)m_Items[i];
			break;
		}

	return pResult;
}

//-----------------------------------------------------------------------------
// 描述: 创建一个参数对象并返回
//-----------------------------------------------------------------------------
CDbParam* CDbParamList::CreateParam(const string& strName, int nNumber)
{
	CDbParam *pResult = m_pDbQuery->GetDatabase()->CreateDbParam();

	pResult->m_pDbQuery = m_pDbQuery;
	pResult->m_strName = strName;
	pResult->m_nNumber = nNumber;

	return pResult;
}

//-----------------------------------------------------------------------------
// 描述: 根据名称返回对应的参数对象，若无则返回NULL
//-----------------------------------------------------------------------------
CDbParam* CDbParamList::ParamByName(const string& strName)
{
	CDbParam *pResult = FindParam(strName);
	if (!pResult)
	{
		pResult = CreateParam(strName, 0);
		m_Items.Add(pResult);
	}

	return pResult;
}

//-----------------------------------------------------------------------------
// 描述: 根据序号(1-based)返回对应的参数对象，若无则返回NULL
//-----------------------------------------------------------------------------
CDbParam* CDbParamList::ParamByNumber(int nNumber)
{
	CDbParam *pResult = FindParam(nNumber);
	if (!pResult)
	{
		pResult = CreateParam("", nNumber);
		m_Items.Add(pResult);
	}

	return pResult;
}

//-----------------------------------------------------------------------------
// 描述: 释放并清空所有字段数据对象
//-----------------------------------------------------------------------------
void CDbParamList::Clear()
{
	for (int i = 0; i < m_Items.GetCount(); i++)
		delete (CDbParam*)m_Items[i];
	m_Items.Clear();
}

///////////////////////////////////////////////////////////////////////////////
// class CDbDataSet

CDbDataSet::CDbDataSet(CDbQuery *pDbQuery) :
	m_pDbQuery(pDbQuery)
{
	// nothing
}

CDbDataSet::~CDbDataSet()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 取得当前数据集中的记录总数
//-----------------------------------------------------------------------------
UINT64 CDbDataSet::GetRecordCount()
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return 0;
}

//-----------------------------------------------------------------------------
// 描述: 返回数据集是否为空
//-----------------------------------------------------------------------------
bool CDbDataSet::IsEmpty()
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return true;
}

//-----------------------------------------------------------------------------
// 描述: 取得当前记录中的字段总数
//-----------------------------------------------------------------------------
int CDbDataSet::GetFieldCount()
{
	return m_DbFieldDefList.GetCount();
}

//-----------------------------------------------------------------------------
// 描述: 取得当前记录中某个字段的定义 (nIndex: 0-based)
//-----------------------------------------------------------------------------
CDbFieldDef* CDbDataSet::GetFieldDefs(int nIndex)
{
	if (nIndex >= 0 && nIndex < m_DbFieldDefList.GetCount())
		return m_DbFieldDefList[nIndex];
	else
		IseThrowDbException(SEM_INDEX_ERROR);

	return NULL;
}

//-----------------------------------------------------------------------------
// 描述: 取得当前记录中某个字段的数据 (nIndex: 0-based)
//-----------------------------------------------------------------------------
CDbField* CDbDataSet::GetFields(int nIndex)
{
	if (nIndex >= 0 && nIndex < m_DbFieldList.GetCount())
		return m_DbFieldList[nIndex];
	else
		IseThrowDbException(SEM_INDEX_ERROR);

	return NULL;
}

//-----------------------------------------------------------------------------
// 描述: 取得当前记录中某个字段的数据
// 参数:
//   strName - 字段名
//-----------------------------------------------------------------------------
CDbField* CDbDataSet::GetFields(const string& strName)
{
	int nIndex = m_DbFieldDefList.IndexOfName(strName);

	if (nIndex >= 0)
		return GetFields(nIndex);
	else
	{
		CStrList FieldNames;
		m_DbFieldDefList.GetFieldNameList(FieldNames);
		string strFieldNameList = FieldNames.GetCommaText();

		string strErrMsg = FormatString(SEM_FIELD_NAME_ERROR, strName.c_str(), strFieldNameList.c_str());
		IseThrowDbException(strErrMsg.c_str());
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class CDbQuery

CDbQuery::CDbQuery(CDatabase *pDatabase) :
	m_pDatabase(pDatabase),
	m_pDbConnection(NULL),
	m_pDbParamList(NULL)
{
	m_pDbParamList = pDatabase->CreateDbParamList(this);
}

CDbQuery::~CDbQuery()
{
	delete m_pDbParamList;

	if (m_pDbConnection)
		m_pDatabase->GetDbConnectionPool()->ReturnConnection(m_pDbConnection);
}

void CDbQuery::EnsureConnected()
{
	if (!m_pDbConnection)
		m_pDbConnection = m_pDatabase->GetDbConnectionPool()->GetConnection();
}

//-----------------------------------------------------------------------------
// 描述: 设置SQL语句
//-----------------------------------------------------------------------------
void CDbQuery::SetSql(const string& strSql)
{
	m_strSql = strSql;
	m_pDbParamList->Clear();

	DoSetSql(strSql);
}

//-----------------------------------------------------------------------------
// 描述: 根据名称取得参数对象
// 备注:
//   缺省情况下此功能不可用，子类若要启用此功能，可调用：
//   return m_pDbParamList->ParamByName(strName);
//-----------------------------------------------------------------------------
CDbParam* CDbQuery::ParamByName(const string& strName)
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return NULL;
}

//-----------------------------------------------------------------------------
// 描述: 根据序号(1-based)取得参数对象
// 备注:
//   缺省情况下此功能不可用，子类若要启用此功能，可调用：
//   return m_pDbParamList->ParamByNumber(nNumber);
//-----------------------------------------------------------------------------
CDbParam* CDbQuery::ParamByNumber(int nNumber)
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return NULL;
}

//-----------------------------------------------------------------------------
// 描述: 执行SQL (无返回结果)
//-----------------------------------------------------------------------------
void CDbQuery::Execute()
{
	EnsureConnected();
	DoExecute(NULL);
}

//-----------------------------------------------------------------------------
// 描述: 执行SQL (返回数据集)
//-----------------------------------------------------------------------------
CDbDataSet* CDbQuery::Query()
{
	EnsureConnected();

	CDbDataSet *pDataSet = m_pDatabase->CreateDbDataSet(this);
	try
	{
		// 执行查询
		DoExecute(pDataSet);
		// 初始化数据集
		pDataSet->InitDataSet();
		// 初始化数据集各字段的定义
		pDataSet->InitFieldDefs();
	}
	catch (CException&)
	{
		delete pDataSet;
		pDataSet = NULL;
		throw;
	}

	return pDataSet;
}

//-----------------------------------------------------------------------------
// 描述: 转换字符串使之在SQL中合法 (str 中可含 '\0' 字符)
//-----------------------------------------------------------------------------
string CDbQuery::EscapeString(const string& str)
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return "";
}

//-----------------------------------------------------------------------------
// 描述: 取得执行SQL后受影响的行数
//-----------------------------------------------------------------------------
UINT CDbQuery::GetAffectedRowCount()
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return 0;
}

//-----------------------------------------------------------------------------
// 描述: 取得最后一条插入语句的自增ID的值
//-----------------------------------------------------------------------------
UINT64 CDbQuery::GetLastInsertId()
{
	IseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
	return 0;
}

//-----------------------------------------------------------------------------
// 描述: 取得查询器所用的数据库连接
//-----------------------------------------------------------------------------
CDbConnection* CDbQuery::GetDbConnection()
{
	EnsureConnected();
	return m_pDbConnection;
}

///////////////////////////////////////////////////////////////////////////////
// class CDatabase

CDatabase::CDatabase()
{
	m_pDbConnParams = NULL;
	m_pDbOptions = NULL;
	m_pDbConnectionPool = NULL;
}

CDatabase::~CDatabase()
{
	delete m_pDbConnParams;
	delete m_pDbOptions;
	delete m_pDbConnectionPool;
}

void CDatabase::EnsureInited()
{
	if (!m_pDbConnParams)
	{
		m_pDbConnParams = CreateDbConnParams();
		m_pDbOptions = CreateDbOptions();
		m_pDbConnectionPool = CreateDbConnectionPool();
	}
}

CDbConnParams* CDatabase::GetDbConnParams()
{
	EnsureInited();
	return m_pDbConnParams;
}

CDbOptions* CDatabase::GetDbOptions()
{
	EnsureInited();
	return m_pDbOptions;
}

CDbConnectionPool* CDatabase::GetDbConnectionPool()
{
	EnsureInited();
	return m_pDbConnectionPool;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
