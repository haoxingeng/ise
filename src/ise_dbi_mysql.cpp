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
// 文件名称: ise_dbi_mysql.cpp
// 功能描述: MySQL数据库访问
///////////////////////////////////////////////////////////////////////////////

#include "ise_dbi_mysql.h"
#include "ise_sysutils.h"
#include "ise_errmsgs.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class CMySqlConnection

CMySqlConnection::CMySqlConnection(CDatabase* pDatabase) :
	CDbConnection(pDatabase)
{
	memset(&m_ConnObject, 0, sizeof(m_ConnObject));
}

CMySqlConnection::~CMySqlConnection()
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 建立连接 (若失败则抛出异常)
//-----------------------------------------------------------------------------
void CMySqlConnection::DoConnect()
{
	static CCriticalSection s_Lock;
	CAutoLocker Locker(s_Lock);

	if (mysql_init(&m_ConnObject) == NULL)
		IseThrowDbException(SEM_MYSQL_INIT_ERROR);

	Logger().WriteFmt(SEM_MYSQL_CONNECTING, &m_ConnObject);

	int nValue = 0;
	mysql_options(&m_ConnObject, MYSQL_OPT_RECONNECT, (char*)&nValue);

	if (mysql_real_connect(&m_ConnObject,
		m_pDatabase->GetDbConnParams()->GetHostName().c_str(),
		m_pDatabase->GetDbConnParams()->GetUserName().c_str(),
		m_pDatabase->GetDbConnParams()->GetPassword().c_str(),
		m_pDatabase->GetDbConnParams()->GetDbName().c_str(),
		m_pDatabase->GetDbConnParams()->GetPort(), NULL, 0) == NULL)
	{
		mysql_close(&m_ConnObject);
		IseThrowDbException(FormatString(SEM_MYSQL_REAL_CONNECT_ERROR, mysql_error(&m_ConnObject)).c_str());
	}

	// for MYSQL 5.0.7 or higher
	string strInitialCharSet = m_pDatabase->GetDbOptions()->GetInitialCharSet();
	if (!strInitialCharSet.empty())
		mysql_set_character_set(&m_ConnObject, strInitialCharSet.c_str());

	Logger().WriteFmt(SEM_MYSQL_CONNECTED, &m_ConnObject);
}

//-----------------------------------------------------------------------------
// 描述: 断开连接
//-----------------------------------------------------------------------------
void CMySqlConnection::DoDisconnect()
{
	mysql_close(&m_ConnObject);
}

///////////////////////////////////////////////////////////////////////////////
// class CMySqlField

CMySqlField::CMySqlField()
{
	m_pDataPtr = NULL;
	m_nDataSize = 0;
}

void CMySqlField::SetData(void *pDataPtr, int nDataSize)
{
	m_pDataPtr = (char*)pDataPtr;
	m_nDataSize = nDataSize;
}

//-----------------------------------------------------------------------------
// 描述: 以字符串型返回字段值
//-----------------------------------------------------------------------------
string CMySqlField::AsString() const
{
	string strResult;

	if (m_pDataPtr && m_nDataSize > 0)
		strResult.assign(m_pDataPtr, m_nDataSize);

	return strResult;
}

///////////////////////////////////////////////////////////////////////////////
// class CMySqlDataSet

CMySqlDataSet::CMySqlDataSet(CDbQuery* pDbQuery) :
	CDbDataSet(pDbQuery),
	m_pRes(NULL),
	m_pRow(NULL)
{
	// nothing
}

CMySqlDataSet::~CMySqlDataSet()
{
	FreeDataSet();
}

MYSQL& CMySqlDataSet::GetConnObject()
{
	return ((CMySqlConnection*)m_pDbQuery->GetDbConnection())->GetConnObject();
}

//-----------------------------------------------------------------------------
// 描述: 释放数据集
//-----------------------------------------------------------------------------
void CMySqlDataSet::FreeDataSet()
{
	if (m_pRes)
		mysql_free_result(m_pRes);
	m_pRes = NULL;
}

//-----------------------------------------------------------------------------
// 描述: 初始化数据集 (若初始化失败则抛出异常)
//-----------------------------------------------------------------------------
void CMySqlDataSet::InitDataSet()
{
	// 从MySQL服务器一次性获取所有行
	m_pRes = mysql_store_result(&GetConnObject());

	// 如果获取失败
	if (!m_pRes)
	{
		IseThrowDbException(FormatString(SEM_MYSQL_STORE_RESULT_ERROR,
			mysql_error(&GetConnObject())).c_str());
	}
}

//-----------------------------------------------------------------------------
// 描述: 初始化数据集各字段的定义
//-----------------------------------------------------------------------------
void CMySqlDataSet::InitFieldDefs()
{
	MYSQL_FIELD *pMySqlFields;
	CDbFieldDef* pFieldDef;
	int nFieldCount;

	m_DbFieldDefList.Clear();
	nFieldCount = mysql_num_fields(m_pRes);
	pMySqlFields = mysql_fetch_fields(m_pRes);

	if (nFieldCount <= 0)
		IseThrowDbException(SEM_MYSQL_NUM_FIELDS_ERROR);

	for (int i = 0; i < nFieldCount; i++)
	{
		pFieldDef = new CDbFieldDef();
		pFieldDef->SetData(pMySqlFields[i].name, pMySqlFields[i].type);
		m_DbFieldDefList.Add(pFieldDef);
	}
}

//-----------------------------------------------------------------------------
// 描述: 将游标指向起始位置(第一条记录之前)
//-----------------------------------------------------------------------------
bool CMySqlDataSet::Rewind()
{
	if (GetRecordCount() > 0)
	{
		mysql_data_seek(m_pRes, 0);
		return true;
	}
	else
		return false;
}

//-----------------------------------------------------------------------------
// 描述: 将游标指向下一条记录
//-----------------------------------------------------------------------------
bool CMySqlDataSet::Next()
{
	m_pRow = mysql_fetch_row(m_pRes);
	if (m_pRow)
	{
		CMySqlField* pField;
		int nFieldCount;
		unsigned long* pLengths;

		nFieldCount = mysql_num_fields(m_pRes);
		pLengths = (unsigned long*)mysql_fetch_lengths(m_pRes);

		for (int i = 0; i < nFieldCount; i++)
		{
			if (i < m_DbFieldList.GetCount())
			{
				pField = (CMySqlField*)m_DbFieldList[i];
			}
			else
			{
				pField = new CMySqlField();
				m_DbFieldList.Add(pField);
			}

			pField->SetData(m_pRow[i], pLengths[i]);
		}
	}

	return (m_pRow != NULL);
}

//-----------------------------------------------------------------------------
// 描述: 取得记录总数
// 备注: mysql_num_rows 实际上只是直接返回 m_pRes->row_count，所以效率很高。
//-----------------------------------------------------------------------------
UINT64 CMySqlDataSet::GetRecordCount()
{
	if (m_pRes)
		return (UINT64)mysql_num_rows(m_pRes);
	else
		return 0;
}

//-----------------------------------------------------------------------------
// 描述: 返回数据集是否为空
//-----------------------------------------------------------------------------
bool CMySqlDataSet::IsEmpty()
{
	return (GetRecordCount() == 0);
}

///////////////////////////////////////////////////////////////////////////////
// class CMySqlQuery

CMySqlQuery::CMySqlQuery(CDatabase* pDatabase) :
	CDbQuery(pDatabase)
{
	// nothing
}

CMySqlQuery::~CMySqlQuery()
{
	// nothing
}

MYSQL& CMySqlQuery::GetConnObject()
{
	return ((CMySqlConnection*)m_pDbConnection)->GetConnObject();
}

//-----------------------------------------------------------------------------
// 描述: 执行SQL (若 pResultDataSet 为 NULL，则表示无数据集返回。若失败则抛出异常)
//-----------------------------------------------------------------------------
void CMySqlQuery::DoExecute(CDbDataSet *pResultDataSet)
{
	/*
	摘自MYSQL官方手册:
	Upon connection, mysql_real_connect() sets the reconnect flag (part of the
	MYSQL structure) to a value of 1 in versions of the API older than 5.0.3,
	or 0 in newer versions. A value of 1 for this flag indicates that if a
	statement cannot be performed because of a lost connection, to try reconnecting
	to the server before giving up. You can use the MYSQL_OPT_RECONNECT option
	to mysql_options() to control reconnection behavior.

	即：只要用 mysql_real_connect() 连接到数据库(而不是 mysql_connect())，那么
	在 mysql_real_query() 调用时即使连接丢失(比如TCP连接断开)，该调用也会尝试
	去重新连接数据库。该特性经测试被证明属实。

	注:
	1. 对于 <= 5.0.3 的版本，MySQL默认会重连；此后的版本需调用 mysql_options()
	   明确指定 MYSQL_OPT_RECONNECT 为 true，才会重连。
	2. 为了在连接丢失并重连后，能执行“数据库刚建立连接时要执行的命令”，程序明确
	   指定不让 mysql_real_query() 自动重连，而是由程序显式重连。
	*/

	for (int nTimes = 0; nTimes < 2; nTimes++)
	{
		int r = mysql_real_query(&GetConnObject(), m_strSql.c_str(), (int)m_strSql.length());

		// 如果执行SQL失败
		if (r != 0)
		{
			// 如果是首次，并且错误类型为连接丢失，则重试连接
			if (nTimes == 0)
			{
				int nErrNo = mysql_errno(&GetConnObject());
				if (nErrNo == CR_SERVER_GONE_ERROR || nErrNo == CR_SERVER_LOST)
				{
					Logger().WriteStr(SEM_MYSQL_LOST_CONNNECTION);

					// 强制重新连接
					GetDbConnection()->ActivateConnection(true);
					continue;
				}
			}

			// 否则抛出异常
			string strSql(m_strSql);
			if (strSql.length() > 1024*2)
			{
				strSql.resize(100);
				strSql += "...";
			}

			string strErrMsg = FormatString("%s; Error: %s", strSql.c_str(), mysql_error(&GetConnObject()));
			IseThrowDbException(strErrMsg.c_str());
		}
		else break;
	}
}

//-----------------------------------------------------------------------------
// 描述: 转换字符串使之在SQL中合法
//-----------------------------------------------------------------------------
string CMySqlQuery::EscapeString(const string& str)
{
	if (str.empty()) return "";

	int nSrcLen = (int)str.size();
	CBuffer Buffer(nSrcLen * 2 + 1);
	char *pEnd;

	EnsureConnected();

	pEnd = (char*)Buffer.Data();
	pEnd += mysql_real_escape_string(&GetConnObject(), pEnd, str.c_str(), nSrcLen);
	*pEnd = '\0';

	return (char*)Buffer.Data();
}

//-----------------------------------------------------------------------------
// 描述: 获取执行SQL后受影响的行数
//-----------------------------------------------------------------------------
UINT CMySqlQuery::GetAffectedRowCount()
{
	UINT nResult = 0;

	if (m_pDbConnection)
		nResult = (UINT)mysql_affected_rows(&GetConnObject());

	return nResult;
}

//-----------------------------------------------------------------------------
// 描述: 获取最后一条插入语句的自增ID的值
//-----------------------------------------------------------------------------
UINT64 CMySqlQuery::GetLastInsertId()
{
	UINT64 nResult = 0;

	if (m_pDbConnection)
		nResult = (UINT64)mysql_insert_id(&GetConnObject());

	return nResult;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
