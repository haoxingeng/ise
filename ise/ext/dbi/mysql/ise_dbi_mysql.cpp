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

#include "ise/ext/dbi/mysql/ise_dbi_mysql.h"
#include "ise/main/ise_sys_utils.h"

namespace ise
{

///////////////////////////////////////////////////////////////////////////////
// 错误信息 (ISE Error Message)

const char* const SEM_MYSQL_INIT_ERROR            = "mysql init error.";
const char* const SEM_MYSQL_NUM_FIELDS_ERROR      = "mysql_num_fields error.";
const char* const SEM_MYSQL_REAL_CONNECT_ERROR    = "mysql_real_connect failed. Error: %s.";
const char* const SEM_MYSQL_STORE_RESULT_ERROR    = "mysql_store_result failed. Error: %s.";
const char* const SEM_MYSQL_CONNECTING            = "Try to connect MySQL server... (%p)";
const char* const SEM_MYSQL_CONNECTED             = "Connected to MySQL server. (%p)";
const char* const SEM_MYSQL_LOST_CONNNECTION      = "Lost connection to MySQL server.";

///////////////////////////////////////////////////////////////////////////////
// class MySqlConnection

MySqlConnection::MySqlConnection(Database* database) :
    DbConnection(database)
{
    memset(&connObject_, 0, sizeof(connObject_));
}

MySqlConnection::~MySqlConnection()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 建立连接 (若失败则抛出异常)
//-----------------------------------------------------------------------------
void MySqlConnection::doConnect()
{
    static Mutex s_mutex;
    AutoLocker locker(s_mutex);

    if (mysql_init(&connObject_) == NULL)
        iseThrowDbException(SEM_MYSQL_INIT_ERROR);

    logger().writeFmt(SEM_MYSQL_CONNECTING, &connObject_);

    int value = 0;
    mysql_options(&connObject_, MYSQL_OPT_RECONNECT, (char*)&value);

    if (mysql_real_connect(&connObject_,
        database_->getDbConnParams()->getHostName().c_str(),
        database_->getDbConnParams()->getUserName().c_str(),
        database_->getDbConnParams()->getPassword().c_str(),
        database_->getDbConnParams()->getDbName().c_str(),
        database_->getDbConnParams()->getPort(), NULL, 0) == NULL)
    {
        mysql_close(&connObject_);
        iseThrowDbException(formatString(SEM_MYSQL_REAL_CONNECT_ERROR, mysql_error(&connObject_)).c_str());
    }

    // for MYSQL 5.0.7 or higher
    string strInitialCharSet = database_->getDbOptions()->getInitialCharSet();
    if (!strInitialCharSet.empty())
        mysql_set_character_set(&connObject_, strInitialCharSet.c_str());

    logger().writeFmt(SEM_MYSQL_CONNECTED, &connObject_);
}

//-----------------------------------------------------------------------------
// 描述: 断开连接
//-----------------------------------------------------------------------------
void MySqlConnection::doDisconnect()
{
    mysql_close(&connObject_);
}

///////////////////////////////////////////////////////////////////////////////
// class MySqlField

MySqlField::MySqlField()
{
    dataPtr_ = NULL;
    dataSize_ = 0;
}

void MySqlField::setData(void *dataPtr, int dataSize)
{
    dataPtr_ = (char*)dataPtr;
    dataSize_ = dataSize;
}

//-----------------------------------------------------------------------------
// 描述: 以字符串型返回字段值
//-----------------------------------------------------------------------------
string MySqlField::asString() const
{
    string result;

    if (dataPtr_ && dataSize_ > 0)
        result.assign(dataPtr_, dataSize_);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class MySqlDataSet

MySqlDataSet::MySqlDataSet(DbQuery* dbQuery) :
    DbDataSet(dbQuery),
    res_(NULL),
    row_(NULL)
{
    // nothing
}

MySqlDataSet::~MySqlDataSet()
{
    freeDataSet();
}

//-----------------------------------------------------------------------------
// 描述: 将游标指向起始位置(第一条记录之前)
//-----------------------------------------------------------------------------
bool MySqlDataSet::rewind()
{
    if (getRecordCount() > 0)
    {
        mysql_data_seek(res_, 0);
        return true;
    }
    else
        return false;
}

//-----------------------------------------------------------------------------
// 描述: 将游标指向下一条记录
//-----------------------------------------------------------------------------
bool MySqlDataSet::next()
{
    row_ = mysql_fetch_row(res_);
    if (row_)
    {
        MySqlField* field;
        int fieldCount;
        unsigned long *lengths;

        fieldCount = mysql_num_fields(res_);
        lengths = (unsigned long*)mysql_fetch_lengths(res_);

        for (int i = 0; i < fieldCount; i++)
        {
            if (i < dbFieldList_.getCount())
            {
                field = (MySqlField*)dbFieldList_[i];
            }
            else
            {
                field = new MySqlField();
                dbFieldList_.add(field);
            }

            field->setData(row_[i], lengths[i]);
        }
    }

    return (row_ != NULL);
}

//-----------------------------------------------------------------------------
// 描述: 取得记录总数
// 备注: mysql_num_rows 实际上只是直接返回 m_pRes->row_count，所以效率很高。
//-----------------------------------------------------------------------------
UINT64 MySqlDataSet::getRecordCount()
{
    if (res_)
        return (UINT64)mysql_num_rows(res_);
    else
        return 0;
}

//-----------------------------------------------------------------------------
// 描述: 返回数据集是否为空
//-----------------------------------------------------------------------------
bool MySqlDataSet::isEmpty()
{
    return (getRecordCount() == 0);
}

//-----------------------------------------------------------------------------
// 描述: 初始化数据集 (若初始化失败则抛出异常)
//-----------------------------------------------------------------------------
void MySqlDataSet::initDataSet()
{
    // 从MySQL服务器一次性获取所有行
    res_ = mysql_store_result(&getConnObject());

    // 如果获取失败
    if (!res_)
    {
        iseThrowDbException(formatString(SEM_MYSQL_STORE_RESULT_ERROR,
            mysql_error(&getConnObject())).c_str());
    }
}

//-----------------------------------------------------------------------------
// 描述: 初始化数据集各字段的定义
//-----------------------------------------------------------------------------
void MySqlDataSet::initFieldDefs()
{
    MYSQL_FIELD *mySqlFields;
    DbFieldDef* fieldDef;
    int fieldCount;

    dbFieldDefList_.clear();
    fieldCount = mysql_num_fields(res_);
    mySqlFields = mysql_fetch_fields(res_);

    if (fieldCount <= 0)
        iseThrowDbException(SEM_MYSQL_NUM_FIELDS_ERROR);

    for (int i = 0; i < fieldCount; i++)
    {
        fieldDef = new DbFieldDef();
        fieldDef->setData(mySqlFields[i].name, mySqlFields[i].type);
        dbFieldDefList_.add(fieldDef);
    }
}

//-----------------------------------------------------------------------------

MYSQL& MySqlDataSet::getConnObject()
{
    return ((MySqlConnection*)dbQuery_->getDbConnection())->getConnObject();
}

//-----------------------------------------------------------------------------
// 描述: 释放数据集
//-----------------------------------------------------------------------------
void MySqlDataSet::freeDataSet()
{
    if (res_)
        mysql_free_result(res_);
    res_ = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class MySqlQuery

MySqlQuery::MySqlQuery(Database* database) :
    DbQuery(database)
{
    // nothing
}

MySqlQuery::~MySqlQuery()
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 转换字符串使之在SQL中合法
//-----------------------------------------------------------------------------
string MySqlQuery::escapeString(const string& str)
{
    if (str.empty()) return "";

    int srcLen = (int)str.size();
    Buffer buffer(srcLen * 2 + 1);
    char *end;

    ensureConnected();

    end = (char*)buffer.data();
    end += mysql_real_escape_string(&getConnObject(), end, str.c_str(), srcLen);
    *end = '\0';

    return (char*)buffer.data();
}

//-----------------------------------------------------------------------------
// 描述: 获取执行SQL后受影响的行数
//-----------------------------------------------------------------------------
UINT MySqlQuery::getAffectedRowCount()
{
    UINT result = 0;

    if (dbConnection_)
        result = (UINT)mysql_affected_rows(&getConnObject());

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 获取最后一条插入语句的自增ID的值
//-----------------------------------------------------------------------------
UINT64 MySqlQuery::getLastInsertId()
{
    UINT64 result = 0;

    if (dbConnection_)
        result = (UINT64)mysql_insert_id(&getConnObject());

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 执行SQL (若 resultDataSet 为 NULL，则表示无数据集返回。若失败则抛出异常)
//-----------------------------------------------------------------------------
void MySqlQuery::doExecute(DbDataSet *resultDataSet)
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

    for (int times = 0; times < 2; times++)
    {
        int r = mysql_real_query(&getConnObject(), sql_.c_str(), (int)sql_.length());

        // 如果执行SQL失败
        if (r != 0)
        {
            // 如果是首次，并且错误类型为连接丢失，则重试连接
            if (times == 0)
            {
                int errNum = mysql_errno(&getConnObject());
                if (errNum == CR_SERVER_GONE_ERROR || errNum == CR_SERVER_LOST)
                {
                    logger().writeStr(SEM_MYSQL_LOST_CONNNECTION);

                    // 强制重新连接
                    getDbConnection()->activateConnection(true);
                    continue;
                }
            }

            // 否则抛出异常
            string sql(sql_);
            if (sql.length() > 1024*2)
            {
                sql.resize(100);
                sql += "...";
            }

            string errMsg = formatString("%s; Error: %s", sql.c_str(), mysql_error(&getConnObject()));
            iseThrowDbException(errMsg.c_str());
        }
        else break;
    }
}

//-----------------------------------------------------------------------------

MYSQL& MySqlQuery::getConnObject()
{
    return ((MySqlConnection*)dbConnection_)->getConnObject();
}

///////////////////////////////////////////////////////////////////////////////

} // namespace ise
