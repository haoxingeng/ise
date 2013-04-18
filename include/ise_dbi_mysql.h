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
//   * MySqlConnection       - 数据库连接类
//   * MySqlField            - 字段数据类
//   * MySqlDataSet          - 数据集类
//   * MySqlQuery            - 数据查询器类
//   * MySqlDatabase         - 数据库类
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

class MySqlConnection;
class MySqlField;
class MySqlDataSet;
class MySqlQuery;
class MySqlDatabase;

///////////////////////////////////////////////////////////////////////////////
// class MySqlConnection - 数据库连接类

class MySqlConnection : public DbConnection
{
public:
    MySqlConnection(Database *database);
    virtual ~MySqlConnection();

    // 建立连接 (若失败则抛出异常)
    virtual void doConnect();
    // 断开连接
    virtual void doDisconnect();

    // 取得MySQL连接对象
    MYSQL& getConnObject() { return connObject_; }

private:
    MYSQL connObject_;            // 连接对象
};

///////////////////////////////////////////////////////////////////////////////
// class MySqlField - 字段数据类

class MySqlField : public DbField
{
public:
    MySqlField();
    virtual ~MySqlField() {}

    void setData(void *dataPtr, int dataSize);
    virtual bool isNull() const { return (dataPtr_ == NULL); }
    virtual string asString() const;

private:
    char* dataPtr_;               // 指向字段数据
    int dataSize_;                // 字段数据的总字节数
};

///////////////////////////////////////////////////////////////////////////////
// class MySqlDataSet - 数据集类

class MySqlDataSet : public DbDataSet
{
public:
    MySqlDataSet(DbQuery* dbQuery);
    virtual ~MySqlDataSet();

    virtual bool rewind();
    virtual bool next();

    virtual UINT64 getRecordCount();
    virtual bool isEmpty();

protected:
    virtual void initDataSet();
    virtual void initFieldDefs();

private:
    MYSQL& getConnObject();
    void freeDataSet();

private:
    MYSQL_RES* res_;      // MySQL查询结果集
    MYSQL_ROW row_;       // MySQL查询结果行
};

///////////////////////////////////////////////////////////////////////////////
// class MySqlQuery - 查询器类

class MySqlQuery : public DbQuery
{
public:
    MySqlQuery(Database *database);
    virtual ~MySqlQuery();

    virtual string escapeString(const string& str);
    virtual UINT getAffectedRowCount();
    virtual UINT64 getLastInsertId();

protected:
    virtual void doExecute(DbDataSet *resultDataSet);

private:
    MYSQL& getConnObject();
};

///////////////////////////////////////////////////////////////////////////////
// class MySqlDatabase

class MySqlDatabase : public Database
{
public:
    virtual DbConnection* createDbConnection() { return new MySqlConnection(this); }
    virtual DbDataSet* createDbDataSet(DbQuery* dbQuery) { return new MySqlDataSet(dbQuery); }
    virtual DbQuery* createDbQuery() { return new MySqlQuery(this); }
};

///////////////////////////////////////////////////////////////////////////////

} // namespace ise

#endif // _ISE_DBI_MYSQL_H_
