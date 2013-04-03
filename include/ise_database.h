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
// ise_database.h
// Classes:
//   * CDbConnParams          - 数据库连接参数类
//   * CDbOptions             - 数据库配置参数类
//   * CDbConnection          - 数据库连接基类
//   * CDbConnectionPool      - 数据库连接池基类
//   * CDbFieldDef            - 字段定义类
//   * CDbFieldDefList        - 字段定义列表类
//   * CDbField               - 字段数据类
//   * CDbFieldList           - 字段数据列表类
//   * CDbParam               - SQL参数类
//   * CDbParamList           - SQL参数列表类
//   * CDbDataSet             - 数据集类
//   * CDbQuery               - 数据查询器类
//   * CDbQueryWrapper        - 数据查询器包装类
//   * CDbDataSetWrapper      - 数据集包装类
//   * CDatabase              - 数据库类
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_DATABASE_H_
#define _ISE_DATABASE_H_

#include "ise_options.h"
#include "ise_global_defs.h"
#include "ise_classes.h"
#include "ise_socket.h"
#include "ise_exceptions.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class CDbConnParams;
class CDbOptions;
class CDbConnection;
class CDbConnectionPool;
class CDbFieldDef;
class CDbFieldDefList;
class CDbField;
class CDbFieldList;
class CDbParam;
class CDbParamList;
class CDbDataSet;
class CDbQuery;
class CDbQueryWrapper;
class CDbDataSetWrapper;
class CDatabase;

///////////////////////////////////////////////////////////////////////////////
// class CDbConnParams - 数据库连接参数类

class CDbConnParams
{
private:
	string m_strHostName;           // 主机地址
	string m_strUserName;           // 用户名
	string m_strPassword;           // 用户口令
	string m_strDbName;             // 数据库名
	int m_nPort;                    // 连接端口号

public:
	CDbConnParams();
	CDbConnParams(const CDbConnParams& src);
	CDbConnParams(const string& strHostName, const string& strUserName,
		const string& strPassword, const string& strDbName, const int nPort);

	string GetHostName() const { return m_strHostName; }
	string GetUserName() const { return m_strUserName; }
	string GetPassword() const { return m_strPassword; }
	string GetDbName() const { return m_strDbName; }
	int GetPort() const { return m_nPort; }

	void SetHostName(const string& strValue) { m_strHostName = strValue; }
	void SetUserName(const string& strValue) { m_strUserName = strValue; }
	void SetPassword(const string& strValue) { m_strPassword = strValue; }
	void SetDbName(const string& strValue) { m_strDbName = strValue; }
	void SetPort(const int nValue) { m_nPort = nValue; }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbOptions - 数据库配置参数类

class CDbOptions
{
public:
	enum { DEF_MAX_DB_CONNECTIONS = 100 };      // 连接池最大连接数缺省值

private:
	int m_nMaxDbConnections;                    // 连接池所允许的最大连接数
	CStrList m_InitialSqlCmdList;               // 数据库刚建立连接时要执行的命令
	string m_strInitialCharSet;                 // 数据库刚建立连接时要设置的字符集

public:
	CDbOptions();

	int GetMaxDbConnections() const { return m_nMaxDbConnections; }
	CStrList& InitialSqlCmdList() { return m_InitialSqlCmdList; }
	string GetInitialCharSet() { return m_strInitialCharSet; }

	void SetMaxDbConnections(int nValue);
	void SetInitialSqlCmd(const string& strValue);
	void SetInitialCharSet(const string& strValue);
};

///////////////////////////////////////////////////////////////////////////////
// class CDbConnection - 数据库连接基类

class CDbConnection
{
public:
	friend class CDbConnectionPool;

protected:
	CDatabase *m_pDatabase;         // CDatabase 对象的引用
	bool m_bConnected;              // 是否已建立连接
	bool m_bBusy;                   // 此连接当前是否正被占用

protected:
	// 建立数据库连接并进行相关设置 (若失败则抛出异常)
	void Connect();
	// 断开数据库连接并进行相关设置
	void Disconnect();
	// 刚建立连接时执行命令
	void ExecCmdOnConnected();

	// ConnectionPool 将调用下列函数，控制连接的使用情况
	bool GetDbConnection();         // 借用连接
	void ReturnDbConnection();      // 归还连接
	bool IsBusy();                  // 连接是否被借用

protected:
	// 和数据库建立连接(若失败则抛出异常)
	virtual void DoConnect() {}
	// 和数据库断开连接
	virtual void DoDisconnect() {}

public:
	CDbConnection(CDatabase *pDatabase);
	virtual ~CDbConnection();

	// 激活数据库连接
	void ActivateConnection(bool bForce = false);
};

///////////////////////////////////////////////////////////////////////////////
// class CDbConnectionPool - 数据库连接池基类
//
// 工作原理:
// 1. 类中维护一个连接列表，管理当前的所有连接(空闲连接、忙连接)，初始为空，且有一个数量上限。
// 2. 分配连接:
//    首先尝试从连接列表中找出一个空闲连接，若找到则分配成功，同时将连接置为忙状态；若没找到
//    则: (1)若连接数未达到上限，则创建一个新的空闲连接；(2)若连接数已达到上限，则分配失败。
//    若分配失败(连接数已满、无法建立新连接等)，则抛出异常。
// 3. 归还连接:
//    只需将连接置为空闲状态即可，无需(也不可)断开数据库连接。

class CDbConnectionPool
{
protected:
	CDatabase *m_pDatabase;         // 所属 CDatabase 引用
	CList m_DbConnectionList;       // 当前连接列表 (CDbConnection*[])，包含空闲连接和忙连接
	CCriticalSection m_Lock;        // 互斥锁

protected:
	void ClearPool();

public:
	CDbConnectionPool(CDatabase *pDatabase);
	virtual ~CDbConnectionPool();

	// 分配一个可用的空闲连接 (若失败则抛出异常)
	virtual CDbConnection* GetConnection();
	// 归还数据库连接
	virtual void ReturnConnection(CDbConnection *pDbConnection);
};

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldDef - 字段定义类

class CDbFieldDef
{
protected:
	string m_strName;               // 字段名称
	int m_nType;                    // 字段类型(含义由子类定义)

public:
	CDbFieldDef() {}
	CDbFieldDef(const string& strName, int nType);
	CDbFieldDef(const CDbFieldDef& src);
	virtual ~CDbFieldDef() {}

	void SetData(char *sName, int nType) { m_strName = sName; m_nType = nType; }

	string GetName() const { return m_strName; }
	int GetType() const { return m_nType; }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldDefList - 字段定义列表类

class CDbFieldDefList
{
private:
	CList m_Items;                  // (CDbFieldDef* [])

public:
	CDbFieldDefList();
	virtual ~CDbFieldDefList();

	// 添加一个字段定义对象
	void Add(CDbFieldDef *pFieldDef);
	// 释放并清空所有字段定义对象
	void Clear();
	// 返回字段名对应的字段序号(0-based)
	int IndexOfName(const string& strName);
	// 返回全部字段名
	void GetFieldNameList(CStrList& List);

	CDbFieldDef* operator[] (int nIndex);
	int GetCount() const { return m_Items.GetCount(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbField - 字段数据类

class CDbField
{
public:
	CDbField();
	virtual ~CDbField() {}

	virtual bool IsNull() const { return false; }
	virtual int AsInteger(int nDefault = 0) const;
	virtual INT64 AsInt64(INT64 nDefault = 0) const;
	virtual double AsFloat(double fDefault = 0) const;
	virtual bool AsBoolean(bool bDefault = false) const;
	virtual string AsString() const { return ""; };
};

///////////////////////////////////////////////////////////////////////////////
// class CDbFieldList - 字段数据列表类

class CDbFieldList
{
private:
	CList m_Items;       // (CDbField* [])

public:
	CDbFieldList();
	virtual ~CDbFieldList();

	// 添加一个字段数据对象
	void Add(CDbField *pField);
	// 释放并清空所有字段数据对象
	void Clear();

	CDbField* operator[] (int nIndex);
	int GetCount() const { return m_Items.GetCount(); }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbParam - SQL参数类

class CDbParam
{
public:
	friend class CDbParamList;

protected:
	CDbQuery *m_pDbQuery;   // CDbQuery 对象引用
	string m_strName;       // 参数名称
	int m_nNumber;          // 参数序号(1-based)

public:
	CDbParam() : m_pDbQuery(NULL), m_nNumber(0) {}
	virtual ~CDbParam() {}

	virtual void SetInt(int nValue) {}
	virtual void SetFloat(double fValue) {}
	virtual void SetString(const string& strValue) {}
};

///////////////////////////////////////////////////////////////////////////////
// class CDbParamList - SQL参数列表类

class CDbParamList
{
protected:
	CDbQuery *m_pDbQuery;   // CDbQuery 对象引用
	CList m_Items;          // (CDbParam* [])

protected:
	virtual CDbParam* FindParam(const string& strName);
	virtual CDbParam* FindParam(int nNumber);
	virtual CDbParam* CreateParam(const string& strName, int nNumber);

public:
	CDbParamList(CDbQuery *pDbQuery);
	virtual ~CDbParamList();

	virtual CDbParam* ParamByName(const string& strName);
	virtual CDbParam* ParamByNumber(int nNumber);

	void Clear();
};

///////////////////////////////////////////////////////////////////////////////
// class CDbDataSet - 数据集类
//
// 说明:
// 1. 此类只提供单向遍历数据集的功能。
// 2. 数据集初始化(InitDataSet)后，游标指向第一条记录之前，需调用 Next() 才指向第一条记录。
//    典型的数据集遍历方法为: while(DataSet.Next()) { ... }
//
// 约定:
// 1. CDbQuery.Query() 创建一个新的 CDbDataSet 对象，之后必须先销毁 CDbDataSet 对象，
//    才能销毁 CDbQuery 对象。
// 2. CDbQuery 在执行查询A后创建了一个数据集A，之后在执行查询B前应关闭数据集A。

class CDbDataSet
{
public:
	friend class CDbQuery;

protected:
	CDbQuery *m_pDbQuery;             // CDbQuery 对象引用
	CDbFieldDefList m_DbFieldDefList; // 字段定义对象列表
	CDbFieldList m_DbFieldList;       // 字段数据对象列表

protected:
	// 初始化数据集 (若失败则抛出异常)
	virtual void InitDataSet() = 0;
	// 初始化数据集各字段的定义
	virtual void InitFieldDefs() = 0;

public:
	CDbDataSet(CDbQuery *pDbQuery);
	virtual ~CDbDataSet();

	// 将游标指向起始位置(第一条记录之前)
	virtual bool Rewind() = 0;
	// 将游标指向下一条记录
	virtual bool Next() = 0;

	// 取得当前数据集中的记录总数
	virtual UINT64 GetRecordCount();
	// 返回数据集是否为空
	virtual bool IsEmpty();

	// 取得当前记录中的字段总数
	int GetFieldCount();
	// 取得当前记录中某个字段的定义 (nIndex: 0-based)
	CDbFieldDef* GetFieldDefs(int nIndex);
	// 取得当前记录中某个字段的数据 (nIndex: 0-based)
	CDbField* GetFields(int nIndex);
	CDbField* GetFields(const string& strName);
};

///////////////////////////////////////////////////////////////////////////////
// class CDbQuery - 数据查询器类
//
// 工作原理:
// 1. 执行SQL: 从连接池取得一个空闲连接，然后利用此连接执行SQL，最后归还连接。

class CDbQuery
{
public:
	friend class CDbConnection;

protected:
	CDatabase *m_pDatabase;         // CDatabase 对象引用
	CDbConnection *m_pDbConnection; // CDbConnection 对象引用
	CDbParamList *m_pDbParamList;   // SQL 参数列表
	string m_strSql;                // 待执行的SQL语句

protected:
	void EnsureConnected();

protected:
	// 设置SQL语句
	virtual void DoSetSql(const string& strSql) {}
	// 执行SQL (若 pResultDataSet 为 NULL，则表示无数据集返回。若失败则抛出异常)
	virtual void DoExecute(CDbDataSet *pResultDataSet) {}

public:
	CDbQuery(CDatabase *pDatabase);
	virtual ~CDbQuery();

	// 设置SQL语句
	void SetSql(const string& strSql);

	// 根据名称取得参数对象
	virtual CDbParam* ParamByName(const string& strName);
	// 根据序号(1-based)取得参数对象
	virtual CDbParam* ParamByNumber(int nNumber);

	// 执行SQL (无返回结果, 若失败则抛出异常)
	void Execute();
	// 执行SQL (返回数据集, 若失败则抛出异常)
	CDbDataSet* Query();

	// 转换字符串使之在SQL中合法 (str 中可含 '\0' 字符)
	virtual string EscapeString(const string& str);
	// 取得执行SQL后受影响的行数
	virtual UINT GetAffectedRowCount();
	// 取得最后一条插入语句的自增ID的值
	virtual UINT64 GetLastInsertId();

	// 取得查询器所用的数据库连接
	CDbConnection* GetDbConnection();
	// 取得 CDatabase 对象
	CDatabase* GetDatabase() { return m_pDatabase; }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbQueryWrapper - 查询器包装类
//
// 说明: 此类用于包装 CDbQuery 对象，自动释放被包装的对象，防止资源泄漏。
// 示例:
//      int main()
//      {
//          CDbQueryWrapper qry( MyDatabase.CreateDbQuery() );
//          qry->SetSql("select * from users");
//          /* ... */
//          // 栈对象 qry 会自动销毁，与此同时被包装的堆对象也自动释放。
//      }

class CDbQueryWrapper
{
private:
	CDbQuery *m_pDbQuery;

public:
	CDbQueryWrapper(CDbQuery *pDbQuery) : m_pDbQuery(pDbQuery) {}
	virtual ~CDbQueryWrapper() { delete m_pDbQuery; }

	CDbQuery* operator -> () { return m_pDbQuery; }
};

///////////////////////////////////////////////////////////////////////////////
// class CDbDataSetWrapper - 数据集包装类
//
// 说明:
// 1. 此类用于包装 CDbDataSet 对象，自动释放被包装的对象，防止资源泄漏。
// 2. 每次给包装器赋值(Wrapper = DataSet)，上次被包装的对象自动释放。
// 示例:
//      int main()
//      {
//          CDbQueryWrapper qry( MyDatabase.CreateDbQuery() );
//          CDbDataSetWrapper ds;
//
//          qry->SetSql("select * from users");
//          ds = qry->Query();
//          /* ... */
//
//          // 栈对象 qry 和 ds 会自动销毁，与此同时被包装的堆对象也自动释放。
//      }

class CDbDataSetWrapper
{
private:
	CDbDataSet *m_pDbDataSet;

public:
	CDbDataSetWrapper() : m_pDbDataSet(NULL) {}
	virtual ~CDbDataSetWrapper() { delete m_pDbDataSet; }

	CDbDataSetWrapper& operator = (CDbDataSet *pDataSet)
	{
		if (m_pDbDataSet) delete m_pDbDataSet;
		m_pDbDataSet = pDataSet;
		return *this;
	}

	CDbDataSet* operator -> () { return m_pDbDataSet; }
};

///////////////////////////////////////////////////////////////////////////////
// class CDatabase - 数据库类

class CDatabase
{
protected:
	CDbConnParams *m_pDbConnParams;             // 数据库连接参数
	CDbOptions *m_pDbOptions;                   // 数据库配置参数
	CDbConnectionPool *m_pDbConnectionPool;     // 数据库连接池
private:
	void EnsureInited();
public:
	CDatabase();
	virtual ~CDatabase();

	// 类工厂方法:
	virtual CDbConnParams* CreateDbConnParams() { return new CDbConnParams(); }
	virtual CDbOptions* CreateDbOptions() { return new CDbOptions(); }
	virtual CDbConnection* CreateDbConnection() = 0;
	virtual CDbConnectionPool* CreateDbConnectionPool() { return new CDbConnectionPool(this); }
	virtual CDbParam* CreateDbParam() { return new CDbParam(); }
	virtual CDbParamList* CreateDbParamList(CDbQuery* pDbQuery) { return new CDbParamList(pDbQuery); }
	virtual CDbDataSet* CreateDbDataSet(CDbQuery* pDbQuery) = 0;
	virtual CDbQuery* CreateDbQuery() = 0;

	CDbConnParams* GetDbConnParams();
	CDbOptions* GetDbOptions();
	CDbConnectionPool* GetDbConnectionPool();
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_DATABASE_H_
