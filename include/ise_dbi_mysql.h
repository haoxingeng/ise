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
// ise_dbi_mysql.h
// Classes:
//   * CMySqlConnection       - 数据库连接类
//   * CMySqlField            - 字段数据类
//   * CMySqlDataSet          - 数据集类
//   * CMySqlQuery            - 数据查询器类
//   * CMySqlDatabase         - 数据库类
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_DBI_MYSQL_H_
#define _ISE_DBI_MYSQL_H_

#include "ise_database.h"

#include <errmsg.h>
#include <mysql.h>

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class CMySqlConnection;
class CMySqlField;
class CMySqlDataSet;
class CMySqlQuery;
class CMySqlDatabase;

///////////////////////////////////////////////////////////////////////////////
// class CMySqlConnection - 数据库连接类

class CMySqlConnection : public CDbConnection
{
private:
	MYSQL m_ConnObject;            // 连接对象

public:
	CMySqlConnection(CDatabase *pDatabase);
	virtual ~CMySqlConnection();

	// 建立连接 (若失败则抛出异常)
	virtual void DoConnect();
	// 断开连接
	virtual void DoDisconnect();

	// 取得MySQL连接对象
	MYSQL& GetConnObject() { return m_ConnObject; }
};

///////////////////////////////////////////////////////////////////////////////
// class CMySqlField - 字段数据类

class CMySqlField : public CDbField
{
private:
	char* m_pDataPtr;               // 指向字段数据
	int m_nDataSize;                // 字段数据的总字节数
public:
	CMySqlField();
	virtual ~CMySqlField() {}

	void SetData(void *pDataPtr, int nDataSize);
	virtual bool IsNull() const { return (m_pDataPtr == NULL); }
	virtual string AsString() const;
};

///////////////////////////////////////////////////////////////////////////////
// class CMySqlDataSet - 数据集类

class CMySqlDataSet : public CDbDataSet
{
private:
	MYSQL_RES* m_pRes;      // MySQL查询结果集
	MYSQL_ROW m_pRow;       // MySQL查询结果行

private:
	MYSQL& GetConnObject();
	void FreeDataSet();

protected:
	virtual void InitDataSet();
	virtual void InitFieldDefs();

public:
	CMySqlDataSet(CDbQuery* pDbQuery);
	virtual ~CMySqlDataSet();

	virtual bool Rewind();
	virtual bool Next();

	virtual UINT64 GetRecordCount();
	virtual bool IsEmpty();
};

///////////////////////////////////////////////////////////////////////////////
// class CMySqlQuery - 查询器类

class CMySqlQuery : public CDbQuery
{
private:
	MYSQL& GetConnObject();

protected:
	virtual void DoExecute(CDbDataSet *pResultDataSet);

public:
	CMySqlQuery(CDatabase *pDatabase);
	virtual ~CMySqlQuery();

	virtual string EscapeString(const string& str);
	virtual UINT GetAffectedRowCount();
	virtual UINT64 GetLastInsertId();
};

///////////////////////////////////////////////////////////////////////////////
// class CMySqlDatabase

class CMySqlDatabase : public CDatabase
{
public:
	virtual CDbConnection* CreateDbConnection() { return new CMySqlConnection(this); }
	virtual CDbDataSet* CreateDbDataSet(CDbQuery* pDbQuery) { return new CMySqlDataSet(pDbQuery); }
	virtual CDbQuery* CreateDbQuery() { return new CMySqlQuery(this); }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_DBI_MYSQL_H_
