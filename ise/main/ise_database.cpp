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

#include "ise/main/ise_database.h"
#include "ise/main/ise_sys_utils.h"
#include "ise/main/ise_err_msgs.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// class DbConnParams

DbConnParams::DbConnParams() :
    port_(0)
{
    // nothing
}

DbConnParams::DbConnParams(const DbConnParams& src)
{
    hostName_ = src.hostName_;
    userName_ = src.userName_;
    password_ = src.password_;
    dbName_ = src.dbName_;
    port_ = src.port_;
}

DbConnParams::DbConnParams(const string& hostName, const string& userName,
    const string& password, const string& dbName, int port)
{
    hostName_ = hostName;
    userName_ = userName;
    password_ = password;
    dbName_ = dbName;
    port_ = port;
}

///////////////////////////////////////////////////////////////////////////////
// class DbOptions

DbOptions::DbOptions()
{
    setMaxDbConnections(DEF_MAX_DB_CONNECTIONS);
}

void DbOptions::setMaxDbConnections(int value)
{
    if (value < 1) value = 1;
    maxDbConnections_ = value;
}

void DbOptions::setInitialSqlCmd(const string& value)
{
    initialSqlCmdList_.clear();
    initialSqlCmdList_.add(value.c_str());
}

void DbOptions::setInitialCharSet(const string& value)
{
    initialCharSet_ = value;
}

///////////////////////////////////////////////////////////////////////////////
// class DbConnection

DbConnection::DbConnection(Database *database)
{
    database_ = database;
    isConnected_ = false;
    isBusy_ = false;
}

DbConnection::~DbConnection()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 激活数据库连接
// 参数:
//   force - 是否强制激活
//-----------------------------------------------------------------------------
void DbConnection::activateConnection(bool force)
{
    // 没有连接数据库则建立连接
    if (!isConnected_ || force)
    {
        disconnect();
        connect();
        return;
    }
}

//-----------------------------------------------------------------------------
// 描述: 建立数据库连接并进行相关设置 (若失败则抛出异常)
//-----------------------------------------------------------------------------
void DbConnection::connect()
{
    if (!isConnected_)
    {
        doConnect();
        execCmdOnConnected();
        isConnected_ = true;
    }
}

//-----------------------------------------------------------------------------
// 描述: 断开数据库连接并进行相关设置
//-----------------------------------------------------------------------------
void DbConnection::disconnect()
{
    if (isConnected_)
    {
        doDisconnect();
        isConnected_ = false;
    }
}

//-----------------------------------------------------------------------------
// 描述: 刚建立连接时执行命令
//-----------------------------------------------------------------------------
void DbConnection::execCmdOnConnected()
{
    try
    {
        StrList& cmdList = database_->getDbOptions()->initialSqlCmdList();
        if (!cmdList.isEmpty())
        {
            DbQueryWrapper query(database_->createDbQuery());
            query->dbConnection_ = this;

            for (int i = 0; i < cmdList.getCount(); ++i)
            {
                try
                {
                    query->setSql(cmdList[i]);
                    query->execute();
                }
                catch (...)
                {}
            }

            query->dbConnection_ = NULL;
        }
    }
    catch (...)
    {}
}

//-----------------------------------------------------------------------------
// 描述: 借用连接 (由 ConnectionPool 调用)
//-----------------------------------------------------------------------------
bool DbConnection::getDbConnection()
{
    if (!isBusy_)
    {
        activateConnection();
        isBusy_ = true;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// 描述: 归还连接 (由 ConnectionPool 调用)
//-----------------------------------------------------------------------------
void DbConnection::returnDbConnection()
{
    isBusy_ = false;
}

//-----------------------------------------------------------------------------
// 描述: 返回连接是否被借用 (由 ConnectionPool 调用)
//-----------------------------------------------------------------------------
bool DbConnection::isBusy()
{
    return isBusy_;
}

///////////////////////////////////////////////////////////////////////////////
// class DbConnectionPool

DbConnectionPool::DbConnectionPool(Database *database) :
    database_(database)
{
    // nothing
}

DbConnectionPool::~DbConnectionPool()
{
    clearPool();
}

//-----------------------------------------------------------------------------
// 描述: 分配一个可用的空闲连接 (若失败则抛出异常)
// 返回: 连接对象指针
//-----------------------------------------------------------------------------
DbConnection* DbConnectionPool::getConnection()
{
    DbConnection *dbConnection = NULL;
    bool result = false;

    {
        AutoLocker locker(lock_);

        // 检查现有的连接是否能用
        for (int i = 0; i < dbConnectionList_.getCount(); i++)
        {
            dbConnection = (DbConnection*)dbConnectionList_[i];
            result = dbConnection->getDbConnection();  // 借出连接
            if (result) break;
        }

        // 如果借出失败，则增加新的数据库连接到连接池
        if (!result && (dbConnectionList_.getCount() < database_->getDbOptions()->getMaxDbConnections()))
        {
            dbConnection = database_->createDbConnection();
            dbConnectionList_.add(dbConnection);
            result = dbConnection->getDbConnection();
        }
    }

    if (!result)
        iseThrowDbException(SEM_GET_CONN_FROM_POOL_ERROR);

    return dbConnection;
}

//-----------------------------------------------------------------------------
// 描述: 归还数据库连接
//-----------------------------------------------------------------------------
void DbConnectionPool::returnConnection(DbConnection *dbConnection)
{
    AutoLocker locker(lock_);
    dbConnection->returnDbConnection();
}

//-----------------------------------------------------------------------------
// 描述: 清空连接池
//-----------------------------------------------------------------------------
void DbConnectionPool::clearPool()
{
    AutoLocker locker(lock_);

    for (int i = 0; i < dbConnectionList_.getCount(); i++)
    {
        DbConnection *dbConnection;
        dbConnection = (DbConnection*)dbConnectionList_[i];
        dbConnection->doDisconnect();
        delete dbConnection;
    }

    dbConnectionList_.clear();
}

///////////////////////////////////////////////////////////////////////////////
// class DbFieldDef

DbFieldDef::DbFieldDef(const string& name, int type)
{
    name_ = name;
    type_ = type;
}

DbFieldDef::DbFieldDef(const DbFieldDef& src)
{
    name_ = src.name_;
    type_ = src.type_;
}

///////////////////////////////////////////////////////////////////////////////
// class DbFieldDefList

DbFieldDefList::DbFieldDefList()
{
    // nothing
}

DbFieldDefList::~DbFieldDefList()
{
    clear();
}

//-----------------------------------------------------------------------------
// 描述: 添加一个字段定义对象
//-----------------------------------------------------------------------------
void DbFieldDefList::add(DbFieldDef *fieldDef)
{
    if (fieldDef != NULL)
        items_.add(fieldDef);
}

//-----------------------------------------------------------------------------
// 描述: 释放并清空所有字段定义对象
//-----------------------------------------------------------------------------
void DbFieldDefList::clear()
{
    for (int i = 0; i < items_.getCount(); i++)
        delete (DbFieldDef*)items_[i];
    items_.clear();
}

//-----------------------------------------------------------------------------
// 描述: 返回字段名对应的字段序号(0-based)，若未找到则返回-1.
//-----------------------------------------------------------------------------
int DbFieldDefList::indexOfName(const string& name)
{
    int index = -1;

    for (int i = 0; i < items_.getCount(); i++)
    {
        if (sameText(((DbFieldDef*)items_[i])->getName(), name))
        {
            index = i;
            break;
        }
    }

    return index;
}

//-----------------------------------------------------------------------------
// 描述: 返回全部字段名
//-----------------------------------------------------------------------------
void DbFieldDefList::getFieldNameList(StrList& list)
{
    list.clear();
    for (int i = 0; i < items_.getCount(); i++)
        list.add(((DbFieldDef*)items_[i])->getName().c_str());
}

//-----------------------------------------------------------------------------
// 描述: 根据下标号返回字段定义对象 (index: 0-based)
//-----------------------------------------------------------------------------
DbFieldDef* DbFieldDefList::operator[] (int index)
{
    if (index >= 0 && index < items_.getCount())
        return (DbFieldDef*)items_[index];
    else
        return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class DbField

DbField::DbField()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 以整型返回字段值 (若转换失败则返回缺省值)
//-----------------------------------------------------------------------------
int DbField::asInteger(int defaultVal) const
{
    return strToInt(asString(), defaultVal);
}

//-----------------------------------------------------------------------------
// 描述: 以64位整型返回字段值 (若转换失败则返回缺省值)
//-----------------------------------------------------------------------------
INT64 DbField::asInt64(INT64 defaultVal) const
{
    return strToInt64(asString(), defaultVal);
}

//-----------------------------------------------------------------------------
// 描述: 以浮点型返回字段值 (若转换失败则返回缺省值)
//-----------------------------------------------------------------------------
double DbField::asFloat(double defaultVal) const
{
    return strToFloat(asString(), defaultVal);
}

//-----------------------------------------------------------------------------
// 描述: 以布尔型返回字段值 (若转换失败则返回缺省值)
//-----------------------------------------------------------------------------
bool DbField::asBoolean(bool defaultVal) const
{
    return asInteger(defaultVal? 1 : 0) != 0;
}

///////////////////////////////////////////////////////////////////////////////
// class DbFieldList

DbFieldList::DbFieldList()
{
    // nothing
}

DbFieldList::~DbFieldList()
{
    clear();
}

//-----------------------------------------------------------------------------
// 描述: 添加一个字段数据对象
//-----------------------------------------------------------------------------
void DbFieldList::add(DbField *field)
{
    items_.add(field);
}

//-----------------------------------------------------------------------------
// 描述: 释放并清空所有字段数据对象
//-----------------------------------------------------------------------------
void DbFieldList::clear()
{
    for (int i = 0; i < items_.getCount(); i++)
        delete (DbField*)items_[i];
    items_.clear();
}

//-----------------------------------------------------------------------------
// 描述: 根据下标号返回字段数据对象 (index: 0-based)
//-----------------------------------------------------------------------------
DbField* DbFieldList::operator[] (int index)
{
    if (index >= 0 && index < items_.getCount())
        return (DbField*)items_[index];
    else
        return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class DbParamList

DbParamList::DbParamList(DbQuery *dbQuery) :
    dbQuery_(dbQuery)
{
    // nothing
}

DbParamList::~DbParamList()
{
    clear();
}

//-----------------------------------------------------------------------------
// 描述: 根据名称返回对应的参数对象，若无则返回NULL
//-----------------------------------------------------------------------------
DbParam* DbParamList::paramByName(const string& name)
{
    DbParam *result = findParam(name);
    if (!result)
    {
        result = createParam(name, 0);
        items_.add(result);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 根据序号(1-based)返回对应的参数对象，若无则返回NULL
//-----------------------------------------------------------------------------
DbParam* DbParamList::paramByNumber(int number)
{
    DbParam *result = findParam(number);
    if (!result)
    {
        result = createParam("", number);
        items_.add(result);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 释放并清空所有字段数据对象
//-----------------------------------------------------------------------------
void DbParamList::clear()
{
    for (int i = 0; i < items_.getCount(); i++)
        delete (DbParam*)items_[i];
    items_.clear();
}

//-----------------------------------------------------------------------------
// 描述: 根据参数名称在列表中查找参数对象
//-----------------------------------------------------------------------------
DbParam* DbParamList::findParam(const string& name)
{
    DbParam *result = NULL;

    for (int i = 0; i < items_.getCount(); i++)
    {
        if (sameText(((DbParam*)items_[i])->name_, name))
        {
            result = (DbParam*)items_[i];
            break;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 根据参数序号在列表中查找参数对象
//-----------------------------------------------------------------------------
DbParam* DbParamList::findParam(int number)
{
    DbParam *result = NULL;

    for (int i = 0; i < items_.getCount(); i++)
    {
        if (((DbParam*)items_[i])->number_ == number)
        {
            result = (DbParam*)items_[i];
            break;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 创建一个参数对象并返回
//-----------------------------------------------------------------------------
DbParam* DbParamList::createParam(const string& name, int number)
{
    DbParam *result = dbQuery_->getDatabase()->createDbParam();

    result->dbQuery_ = dbQuery_;
    result->name_ = name;
    result->number_ = number;

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class DbDataSet

DbDataSet::DbDataSet(DbQuery *dbQuery) :
    dbQuery_(dbQuery)
{
    // nothing
}

DbDataSet::~DbDataSet()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 取得当前数据集中的记录总数
//-----------------------------------------------------------------------------
UINT64 DbDataSet::getRecordCount()
{
    iseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return 0;
}

//-----------------------------------------------------------------------------
// 描述: 返回数据集是否为空
//-----------------------------------------------------------------------------
bool DbDataSet::isEmpty()
{
    iseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return true;
}

//-----------------------------------------------------------------------------
// 描述: 取得当前记录中的字段总数
//-----------------------------------------------------------------------------
int DbDataSet::getFieldCount()
{
    return dbFieldDefList_.getCount();
}

//-----------------------------------------------------------------------------
// 描述: 取得当前记录中某个字段的定义 (index: 0-based)
//-----------------------------------------------------------------------------
DbFieldDef* DbDataSet::getFieldDefs(int index)
{
    if (index >= 0 && index < dbFieldDefList_.getCount())
        return dbFieldDefList_[index];
    else
        iseThrowDbException(SEM_INDEX_ERROR);

    return NULL;
}

//-----------------------------------------------------------------------------
// 描述: 取得当前记录中某个字段的数据 (index: 0-based)
//-----------------------------------------------------------------------------
DbField* DbDataSet::getFields(int index)
{
    if (index >= 0 && index < dbFieldList_.getCount())
        return dbFieldList_[index];
    else
        iseThrowDbException(SEM_INDEX_ERROR);

    return NULL;
}

//-----------------------------------------------------------------------------
// 描述: 取得当前记录中某个字段的数据
// 参数:
//   name - 字段名
//-----------------------------------------------------------------------------
DbField* DbDataSet::getFields(const string& name)
{
    int index = dbFieldDefList_.indexOfName(name);

    if (index >= 0)
        return getFields(index);
    else
    {
        StrList fieldNames;
        dbFieldDefList_.getFieldNameList(fieldNames);
        string fieldNameList = fieldNames.getCommaText();

        string errMsg = formatString(SEM_FIELD_NAME_ERROR, name.c_str(), fieldNameList.c_str());
        iseThrowDbException(errMsg.c_str());
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class DbQuery

DbQuery::DbQuery(Database *database) :
    database_(database),
    dbConnection_(NULL),
    dbParamList_(NULL)
{
    dbParamList_ = database->createDbParamList(this);
}

DbQuery::~DbQuery()
{
    delete dbParamList_;

    if (dbConnection_)
        database_->getDbConnectionPool()->returnConnection(dbConnection_);
}

//-----------------------------------------------------------------------------
// 描述: 设置SQL语句
//-----------------------------------------------------------------------------
void DbQuery::setSql(const string& sql)
{
    sql_ = sql;
    dbParamList_->clear();

    doSetSql(sql);
}

//-----------------------------------------------------------------------------
// 描述: 根据名称取得参数对象
// 备注:
//   缺省情况下此功能不可用，子类若要启用此功能，可调用：
//   return dbParamList_->ParamByName(name);
//-----------------------------------------------------------------------------
DbParam* DbQuery::paramByName(const string& name)
{
    iseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return NULL;
}

//-----------------------------------------------------------------------------
// 描述: 根据序号(1-based)取得参数对象
// 备注:
//   缺省情况下此功能不可用，子类若要启用此功能，可调用：
//   return dbParamList_->ParamByNumber(number);
//-----------------------------------------------------------------------------
DbParam* DbQuery::paramByNumber(int number)
{
    iseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return NULL;
}

//-----------------------------------------------------------------------------
// 描述: 执行SQL (无返回结果)
//-----------------------------------------------------------------------------
void DbQuery::execute()
{
    ensureConnected();
    doExecute(NULL);
}

//-----------------------------------------------------------------------------
// 描述: 执行SQL (返回数据集)
//-----------------------------------------------------------------------------
DbDataSet* DbQuery::query()
{
    ensureConnected();

    DbDataSet *dataSet = database_->createDbDataSet(this);
    try
    {
        // 执行查询
        doExecute(dataSet);
        // 初始化数据集
        dataSet->initDataSet();
        // 初始化数据集各字段的定义
        dataSet->initFieldDefs();
    }
    catch (Exception&)
    {
        delete dataSet;
        dataSet = NULL;
        throw;
    }

    return dataSet;
}

//-----------------------------------------------------------------------------
// 描述: 转换字符串使之在SQL中合法 (str 中可含 '\0' 字符)
//-----------------------------------------------------------------------------
string DbQuery::escapeString(const string& str)
{
    iseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return "";
}

//-----------------------------------------------------------------------------
// 描述: 取得执行SQL后受影响的行数
//-----------------------------------------------------------------------------
UINT DbQuery::getAffectedRowCount()
{
    iseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return 0;
}

//-----------------------------------------------------------------------------
// 描述: 取得最后一条插入语句的自增ID的值
//-----------------------------------------------------------------------------
UINT64 DbQuery::getLastInsertId()
{
    iseThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return 0;
}

//-----------------------------------------------------------------------------
// 描述: 取得查询器所用的数据库连接
//-----------------------------------------------------------------------------
DbConnection* DbQuery::getDbConnection()
{
    ensureConnected();
    return dbConnection_;
}

//-----------------------------------------------------------------------------

void DbQuery::ensureConnected()
{
    if (!dbConnection_)
        dbConnection_ = database_->getDbConnectionPool()->getConnection();
}

///////////////////////////////////////////////////////////////////////////////
// class Database

Database::Database()
{
    dbConnParams_ = NULL;
    dbOptions_ = NULL;
    dbConnectionPool_ = NULL;
}

Database::~Database()
{
    delete dbConnParams_;
    delete dbOptions_;
    delete dbConnectionPool_;
}

DbConnParams* Database::getDbConnParams()
{
    ensureInited();
    return dbConnParams_;
}

DbOptions* Database::getDbOptions()
{
    ensureInited();
    return dbOptions_;
}

DbConnectionPool* Database::getDbConnectionPool()
{
    ensureInited();
    return dbConnectionPool_;
}

void Database::ensureInited()
{
    if (!dbConnParams_)
    {
        dbConnParams_ = createDbConnParams();
        dbOptions_ = createDbOptions();
        dbConnectionPool_ = createDbConnectionPool();
    }
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
