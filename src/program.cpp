#include "common/instrumentationtimer.h"
#include "common/templateutils.h"
#include <config.h>
#include <filesystem>
#include <fmt/format.h>
#include <htdocs.h>
#include <http/httplistener.h>
#include <http/httplistenerexception.h>
#include <http/httplistenerresponse.h>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <regex>
#include <sqlite3/sqlite3.h>
#include <sstream>
#include <string>
#include <string_view>

std::string exe;

class DataQuery
{
public:
    //    virtual DataQuery &find(
    //        std::string const &id);

    //    virtual DataQuery &top(
    //        int amount);

    //    virtual DataQuery &skip(
    //        int amount);

    //    virtual DataQuery &first();

    //    virtual DataQuery &filter(
    //        std::string const &filter);

    //    virtual DataQuery &where(
    //        std::string const &filter);

    //    virtual DataQuery &count();

    //    virtual DataQuery &orderBy(
    //        std::string const &field,
    //        std::string const &direction);

    //    virtual DataQuery &orderByDesc(
    //        std::string const &field,
    //        std::string const &direction);
};

enum class ColumnTypes
{
    Integer,
    Real,
    Text,
    Blob,
};

class DataTable
{
    std::string _rawName;
    std::string _name = "";
    int _version = 1;
    std::string _primaryKey;
    std::map<std::string, ColumnTypes> _columns;

public:
    DataTable();
    explicit DataTable(
        const std::string &name);

    inline std::string const &RawName() const { return _rawName; }
    inline std::string const &Name() const { return _name; }
    inline std::string const &PrimaryKey() const { return _primaryKey; }
    inline std::map<std::string, ColumnTypes> const &Columns() const { return _columns; }

    void PrimaryKey(
        char const *primaryKey);

    void ClearColumns();

    void AddColumn(
        char const *name,
        char const *type);
};

DataTable::DataTable()
{}

DataTable::DataTable(
    const std::string &name)
    : _rawName(name)
{
    auto versionRegex = std::regex(R"(([\w]+)_v([0-9]+))");

    std::smatch matches;

    if (std::regex_search(_rawName, matches, versionRegex))
    {
        _name = matches[1];
        _version = std::atoi(matches[2].str().c_str());
    }
}

void DataTable::PrimaryKey(
    char const *primaryKey)
{
    _primaryKey = primaryKey;
}

void DataTable::ClearColumns()
{
    _columns.clear();
}

void DataTable::AddColumn(
    char const *name,
    char const *type)
{
    if (std::string_view(type) == "INTEGER" || std::string_view(type).substr(0, 7) == "NUMERIC")
    {
        _columns.insert(std::make_pair(name, ColumnTypes::Integer));
    }
    else if (std::string_view(type) == "TEXT" || std::string_view(type).substr(0, 8) == "NVARCHAR")
    {
        _columns.insert(std::make_pair(name, ColumnTypes::Text));
    }
    else if (std::string_view(type) == "Blob")
    {
        _columns.insert(std::make_pair(name, ColumnTypes::Blob));
    }
    else if (std::string_view(type) == "REAL")
    {
        _columns.insert(std::make_pair(name, ColumnTypes::Real));
    }
}

class DataCollection : public DataQuery
{
    sqlite3 *_db;
    std::vector<DataTable> _tables;

public:
    explicit DataCollection(
        std::string const &db);
    ~DataCollection();

    inline std::vector<DataTable> const &Tables() const { return _tables; }

    nlohmann::json get(
        const DataTable &table) const;

    nlohmann::json get(
        const DataTable &table,
        const std::string &key) const;

    nlohmann::json post(
        const DataTable &table,
        nlohmann::json const &obj) const;

    void patch(
        const DataTable &table,
        nlohmann::json const &obj);

    void put(
        const DataTable &table,
        nlohmann::json const &obj);

    void remove(
        const DataTable &table,
        nlohmann::json const &obj);
};

std::vector<DataTable> ListTables(
    sqlite3 *db)
{
    std::vector<DataTable> tables;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT name FROM sqlite_master WHERE type='table';", -1, &stmt, NULL);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int i;
        int num_cols = sqlite3_column_count(stmt);

        for (i = 0; i < num_cols; i++)
        {
            switch (sqlite3_column_type(stmt, i))
            {
                case (SQLITE3_TEXT):
                {
                    auto table = std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, i)));

                    tables.push_back(DataTable(table));
                    break;
                }
            }
        }
    }

    sqlite3_finalize(stmt);

    return tables;
}

void UpdateTableWithColumns(
    sqlite3 *db,
    DataTable &table)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, fmt::format("PRAGMA table_info({0});", table.RawName()).c_str(), -1, &stmt, NULL);

    int num_cols = sqlite3_column_count(stmt);
    if (num_cols < 6)
    {
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        auto name = (const char *)sqlite3_column_text(stmt, 1);
        auto type = (const char *)sqlite3_column_text(stmt, 2);
        auto pk = sqlite3_column_int(stmt, 5);

        if (pk != 0)
        {
            table.PrimaryKey(name);
        }

        table.AddColumn(name, type);
    }

    sqlite3_finalize(stmt);
}

DataCollection::DataCollection(
    std::string const &db)
    : _db(nullptr)
{
    sqlite3_open(db.c_str(), &_db);

    if (_db == nullptr)
    {
        std::cout << db << " does not exist." << std::endl;
        return;
    }

    _tables = ListTables(_db);

    for (auto &table : _tables)
    {
        UpdateTableWithColumns(_db, table);
    }
}

DataCollection::~DataCollection()
{
    sqlite3_close(_db);
}

nlohmann::json getData(
    sqlite3_stmt *stmt)
{
    auto result = nlohmann::json::array();

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        nlohmann::json row;

        row["index"] = result.size();

        for (int i = 0; i < sqlite3_column_count(stmt); i++)
        {
            auto type = sqlite3_column_type(stmt, i);
            auto name = sqlite3_column_name(stmt, i);

            nlohmann::json cell;

            if (type == 3)
            {
                auto size = sqlite3_column_bytes(stmt, i);
                auto text = sqlite3_column_text(stmt, i);

                auto str = std::string(reinterpret_cast<const char *>(text), size);

                row[name] = str;
            }
            else if (type == 1)
            {
                auto value = sqlite3_column_int(stmt, i);

                row[name] = value;
            }
        }
        result.push_back(row);
    }

    return result;
}

nlohmann::json DataCollection::get(
    const DataTable &table,
    const std::string &key) const
{
    std::stringstream ss;

    ss << "select * from " << table.RawName() << " where " << table.PrimaryKey() << " = ?";

    auto sql = ss.str();

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(_db, sql.c_str(), int(sql.length()), &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, key.c_str(), int(key.length()), SQLITE_STATIC);

    auto result = getData(stmt);

    sqlite3_finalize(stmt);

    if (result.empty())
    {
        return nlohmann::json();
    }

    return result[0];
}

nlohmann::json DataCollection::get(
    const DataTable &table) const
{
    std::stringstream ss;

    ss << "select * from " << table.RawName() << ";";

    auto sql = ss.str();

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(_db, sql.c_str(), int(sql.length()), &stmt, nullptr);

    auto result = getData(stmt);

    sqlite3_finalize(stmt);

    return result;
}

nlohmann::json DataCollection::post(
    const DataTable &table,
    const nlohmann::json &obj) const
{
    (void)table;
    (void)obj;

    std::vector<std::string> keys;
    std::vector<nlohmann::json> values;

    for (auto &val : obj.items())
    {
        if (table.Columns().find(val.key()) == table.Columns().end())
        {
            continue;
        }

        if (val.key() == table.PrimaryKey())
        {
            continue;
        }

        keys.push_back(val.key());
        values.push_back(val.value());
    }

    if (keys.empty())
    {
        nlohmann::json error = {
            {"error", "object has no known properties"},
        };

        return error;
    }

    std::stringstream ss;

    ss << "insert into " << table.RawName() << " (";
    for (size_t i = 0; i < keys.size(); i++)
    {
        if (i > 0) ss << ",  ";
        ss << keys[i];
    }
    ss << ") VALUES (";
    for (size_t i = 0; i < values.size(); i++)
    {
        if (i > 0)
            ss << ", ?";
        else
            ss << "?";
    }
    ss << ");";

    auto sql = ss.str();

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(_db, sql.c_str(), int(sql.length()), &stmt, nullptr);

    for (size_t i = 0; i < values.size(); i++)
    {
        if (values[i].is_string())
        {
            auto var = values[i].get<std::string>();
            sqlite3_bind_text(stmt, 1 + int(i), var.c_str(), int(var.length()), SQLITE_TRANSIENT);
        }
        else if (values[i].is_number_integer())
        {
            auto var = values[i].get<int>();
            sqlite3_bind_int(stmt, 1 + int(i), var);
        }
    }

    auto stepResult = sqlite3_step(stmt);
    if (stepResult == SQLITE_DONE)
    {
        nlohmann::json v = {
            {table.PrimaryKey(), sqlite3_last_insert_rowid(this->_db)},
        };

        return v;
    }
    else if (stepResult == SQLITE_CONSTRAINT)
    {
        nlohmann::json error = {
            {"error", "Abort due to constraint violation"},
        };

        return error;
    }
    else if (stepResult == SQLITE_ERROR || stepResult == SQLITE_MISUSE)
    {
        nlohmann::json error = {
            {"error", sqlite3_errmsg(this->_db)},
        };

        return error;
    }

    sqlite3_finalize(stmt);

    nlohmann::json v = {
        {table.PrimaryKey(), -1},
    };

    return v;
}

void ParseQuery(
    System::Net::Http::HttpListenerRequest const *request,
    const DataQuery &query)
{
    (void)request;
    (void)query;
}

namespace fs = std::filesystem;

void RouteStatic(
    const std::string &content,
    const std::string &contentType,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches);

void RouteHelp(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches);

void RouteQuit(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches);

void RouteGetAllApi(
    const DataCollection &collection,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches);

void RouteGetByIdApi(
    const DataCollection &collection,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches);

void RoutePostApi(
    const DataCollection &collection,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches);

void RouteRoot(
    const char *dbFile,
    const DataCollection &collection,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches);

void BadRequest(
    std::string const &err,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response);

void InternalServerError(
    std::string const &err,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response);

void NotFoundError(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches);

std::string showHelp(
    std::string const &exe,
    bool showOptions);

typedef std::function<void(const System::Net::Http::HttpListenerRequest &request, System::Net::Http::HttpListenerResponse &response, const std::smatch &matches)> RouteHandler;
typedef std::vector<std::pair<std::regex, RouteHandler>> RouteCollection;

class Router
{
public:
    void Get(
        const std::string &pattern,
        RouteHandler handler);

    void Post(
        const std::string &pattern,
        RouteHandler handler);

    bool Route(
        const System::Net::Http::HttpListenerRequest &request,
        System::Net::Http::HttpListenerResponse &response);

private:
    RouteCollection _getRoutes;
    RouteCollection _postRoutes;
    RouteCollection _putRoutes;
};

void Router::Get(
    const std::string &pattern,
    RouteHandler handler)
{
    auto r = std::regex(pattern);

    _getRoutes.push_back(std::make_pair(std::move(r), handler));
}

void Router::Post(
    const std::string &pattern,
    RouteHandler handler)
{
    auto r = std::regex(pattern);

    _postRoutes.push_back(std::make_pair(std::move(r), handler));
}

bool Router::Route(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response)
{
    if (request.HttpMethod() == "GET")
    {
        for (auto &route : _getRoutes)
        {
            std::smatch matches;
            if (!std::regex_match(request.RawUrl(), matches, route.first))
            {
                continue;
            }

            route.second(request, response, matches);

            return true;
        }
    }

    else if (request.HttpMethod() == "POST")
    {
        for (auto &route : _postRoutes)
        {
            std::smatch matches;
            if (!std::regex_match(request.RawUrl(), matches, route.first))
            {
                continue;
            }

            route.second(request, response, matches);

            return true;
        }
    }

    return false;
}

bool keepServerRunning = true;

int main(
    int argc,
    char *argv[])
{
    if (argc == 1)
    {
        showHelp(argv[0], false);
    }

    std::string listenUrl = "http://localhost:8888/";
    const char *dbFile = nullptr;

    for (int i = 0; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h")
        {
            std::cout << showHelp(argv[0], true);
            return 0;
        }
        else if (std::string(argv[i]) == "--listen-url" && ++i < argc)
        {
            listenUrl = argv[i];
        }
        else
        {
            dbFile = argv[i];
        }
    }

    DataCollection collection(dbFile);

    exe = std::string(argv[0]);
    auto pos = exe.find_last_of('\\');
    if (pos != std::string::npos)
    {
        exe = exe.substr(pos + 1);
    }

    System::Net::Http::HttpListener listener;

    listener.Prefixes().push_back(listenUrl);

    try
    {
        listener.Start();

        Router router;

        router.Get("/quit", RouteQuit);
        router.Get("/asr.exe", RouteHelp);
        router.Get("/",
                   [&dbFile, &collection](
                       const System::Net::Http::HttpListenerRequest &request,
                       System::Net::Http::HttpListenerResponse &response,
                       const std::smatch &matches) {
                       RouteRoot(dbFile, collection, request, response, matches);
                   });
        router.Get(R"(/api/([\w\-]+)$)",
                   [&collection](
                       const System::Net::Http::HttpListenerRequest &request,
                       System::Net::Http::HttpListenerResponse &response,
                       const std::smatch &matches) {
                       RouteGetAllApi(collection, request, response, matches);
                   });
        router.Get(R"(/api/([\w\-]+)/([\w\-]+)$)",
                   [&collection](
                       const System::Net::Http::HttpListenerRequest &request,
                       System::Net::Http::HttpListenerResponse &response,
                       const std::smatch &matches) {
                       RouteGetByIdApi(collection, request, response, matches);
                   });
        router.Post(R"(/api/([\w\-]+)$)",
                    [&collection](
                        const System::Net::Http::HttpListenerRequest &request,
                        System::Net::Http::HttpListenerResponse &response,
                        const std::smatch &matches) {
                        RoutePostApi(collection, request, response, matches);
                    });

        router.Get("/styles.css",
                   [](
                       const System::Net::Http::HttpListenerRequest &request,
                       System::Net::Http::HttpListenerResponse &response,
                       const std::smatch &matches) {
                       RouteStatic(HTDOCS_STYLES, "text/css", request, response, matches);
                   });

        router.Get("/scripts.js",
                   [](
                       const System::Net::Http::HttpListenerRequest &request,
                       System::Net::Http::HttpListenerResponse &response,
                       const std::smatch &matches) {
                       RouteStatic(HTDOCS_SCRIPTS, "text/javascript", request, response, matches);
                   });

        while (keepServerRunning)
        {
            auto context = std::unique_ptr<System::Net::Http::HttpListenerContext>(
                listener.GetContext());

            InstrumentationTimer timer(context.get()->Request()->RawUrl().c_str());

            if (router.Route(*(context->Request()), *(context->Response())))
            {
                continue;
            }

            NotFoundError(*(context->Request()), *(context->Response()), std::smatch());
        }

        listener.Stop();
    }
    catch (System::Net::Http::HttpListenerException const *ex)
    {
        std::cout << "Exception in http listener: " << ex->Message() << "\n";
    }

    return 0;
}

std::string Header()
{
    std::stringstream ss;

    ss << "<!doctype html>"
       << "<html lang=\"en\">"
       << "<head>"
       << "<title>Auto Sqlite Rest</title>"
       << "<meta charset=\"utf-8\">"
       << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">"
       << "<script type=\"text/javascript\" src=\"scripts.js\" defer></script>"
       << "<link rel=\"stylesheet\" href=\"styles.css\" />"
       << "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">"
       << "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>"
       << "<link href=\"https://fonts.googleapis.com/css2?family=Prompt:wght@300&family=Ubuntu+Mono&display=swap\" rel=\"stylesheet\">"
       << "<link rel=\"icon\" type=\"image/svg+xml\" href=\"data:image/svg+xml;utf8,<svg xmlns=&quot;http://www.w3.org/2000/svg&quot; width=&quot;16&quot; height=&quot;16&quot; fill=&quot;currentColor&quot; class=&quot;bi bi-code-slash&quot; viewBox=&quot;0 0 16 16&quot;><path d=&quot;M10.478 1.647a.5.5 0 1 0-.956-.294l-4 13a.5.5 0 0 0 .956.294l4-13zM4.854 4.146a.5.5 0 0 1 0 .708L1.707 8l3.147 3.146a.5.5 0 0 1-.708.708l-3.5-3.5a.5.5 0 0 1 0-.708l3.5-3.5a.5.5 0 0 1 .708 0zm6.292 0a.5.5 0 0 0 0 .708L14.293 8l-3.147 3.146a.5.5 0 0 0 .708.708l3.5-3.5a.5.5 0 0 0 0-.708l-3.5-3.5a.5.5 0 0 0-.708 0z&quot;/></svg>\">"
       << "</head>"
       << "<body>"
       << "<a href=\"/" << exe << "\" class=\"header-button float-end\">"
       << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"24\" height=\"24\" fill=\"currentColor\" class=\"bi bi-question-circle-fill\" viewBox=\"0 0 16 16\"><path d=\"M16 8A8 8 0 1 1 0 8a8 8 0 0 1 16 0zM5.496 6.033h.825c.138 0 .248-.113.266-.25.09-.656.54-1.134 1.342-1.134.686 0 1.314.343 1.314 1.168 0 .635-.374.927-.965 1.371-.673.489-1.206 1.06-1.168 1.987l.003.217a.25.25 0 0 0 .25.246h.811a.25.25 0 0 0 .25-.25v-.105c0-.718.273-.927 1.01-1.486.609-.463 1.244-.977 1.244-2.056 0-1.511-1.276-2.241-2.673-2.241-1.267 0-2.655.59-2.75 2.286a.237.237 0 0 0 .241.247zm2.325 6.443c.61 0 1.029-.394 1.029-.927 0-.552-.42-.94-1.029-.94-.584 0-1.009.388-1.009.94 0 .533.425.927 1.01.927z\"/></svg>"
       << "</a>"
       << "<h1>"
       << "    <a href=\"/\">Auto Sqlite Rest <small>v" << ASR_VERSION << "</small></a>"
       << "</h1>";

    return ss.str();
}

std::string Footer()
{
    std::stringstream ss;

    ss << "</body>"
       << "</html>";

    return ss.str();
}

void Ok(
    std::string const &content,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response)
{
    (void)request;

    response.SetStatusCode(200);
    response.WriteOutput(content);
    response.CloseOutput();
}

void NoContent(
    System::Net::Http::HttpListenerResponse &response)
{
    response.SetStatusCode(204);
    response.CloseOutput();
}

void RouteStatic(
    const std::string &content,
    const std::string &contentType,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches)
{
    (void)request;
    (void)matches;

    response.Headers().insert(std::make_pair("Content-Type", contentType));

    Ok(content, request, response);
}

void RouteHelp(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches)
{
    (void)matches;

    std::stringstream ss;

    ss << Header()
       << "<h2>Help page</h1>"
       << "<div class=\"container\">"
       << "<ul class=\"nav justify-content-end\"><li class=\"nav-item\"><a class=\"nav-link\" aria-current=\"page\" href=\"/quit\">Stop server</a></li></ul>"
       << "<div class=\"block\"><pre>" << showHelp(exe, true) << "</pre></div>"
       << "</div>"
       << "</body>"
       << "</html>";

    Ok(ss.str(), request, response);
}

void RouteQuit(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches)
{
    (void)matches;

    std::stringstream ss;

    ss << "<!doctype html>"
       << "<html lang=\"en\">"
       << "<head>"
       << "<title>Auto Sqlite Rest</title>"
       << "<meta charset=\"utf-8\">"
       << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">"
       << "<link rel=\"icon\" type=\"image/svg+xml\" href=\"data:image/svg+xml;utf8,<svg xmlns=&quot;http://www.w3.org/2000/svg&quot; width=&quot;16&quot; height=&quot;16&quot; fill=&quot;currentColor&quot; class=&quot;bi bi-code-slash&quot; viewBox=&quot;0 0 16 16&quot;><path d=&quot;M10.478 1.647a.5.5 0 1 0-.956-.294l-4 13a.5.5 0 0 0 .956.294l4-13zM4.854 4.146a.5.5 0 0 1 0 .708L1.707 8l3.147 3.146a.5.5 0 0 1-.708.708l-3.5-3.5a.5.5 0 0 1 0-.708l3.5-3.5a.5.5 0 0 1 .708 0zm6.292 0a.5.5 0 0 0 0 .708L14.293 8l-3.147 3.146a.5.5 0 0 0 .708.708l3.5-3.5a.5.5 0 0 0 0-.708l-3.5-3.5a.5.5 0 0 0-.708 0z&quot;/></svg>\">"
       << "</head>"
       << "<body>"
       << "<h2>Ok, i quit</h2>"
       << "</body>"
       << "</html>";

    Ok(ss.str(), request, response);

    keepServerRunning = false;
}

void RouteGetAllApi(
    const DataCollection &collection,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches)
{
    (void)collection;
    (void)matches;

    auto found = std::find_if(
        collection.Tables().begin(),
        collection.Tables().end(),
        [&matches](const DataTable &table) {
            return table.Name() == matches[1];
        });

    if (found == collection.Tables().end())
    {
        NotFoundError(request, response, matches);
        return;
    }

    auto data = collection.get(*found);

    response.Headers().insert(std::make_pair("Content-Type", "application/json"));

    Ok(data.dump(4), request, response);
}

void RouteGetByIdApi(
    const DataCollection &collection,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches)
{
    (void)matches;

    auto found = std::find_if(
        collection.Tables().begin(),
        collection.Tables().end(),
        [&matches](const DataTable &table) {
            return table.Name() == matches[1];
        });

    if (found == collection.Tables().end())
    {
        NotFoundError(request, response, matches);
        return;
    }

    auto data = collection.get(*found, matches[2]);

    if (data.empty())
    {
        NotFoundError(request, response, matches);
        return;
    }

    response.Headers().insert(std::make_pair("Content-Type", "application/json"));

    Ok(data.dump(4), request, response);
}

void RoutePostApi(
    const DataCollection &collection,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches)
{
    auto found = std::find_if(
        collection.Tables().begin(),
        collection.Tables().end(),
        [&matches](const DataTable &table) {
            return table.Name() == matches[1];
        });

    if (found == collection.Tables().end())
    {
        NotFoundError(request, response, matches);
        return;
    }

    auto jsonData = nlohmann::json::parse(request._payload.begin(), request._payload.end());

    auto result = collection.post(*found, jsonData);

    std::cout << result.dump(4) << std::endl;
    if (result.count("error") > 0)
    {
        BadRequest(result["error"].get<std::string>(), request, response);
    }

    NoContent(response);
}

void RouteRoot(
    const char *dbFile,
    const DataCollection &collection,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches)
{
    (void)matches;

    auto db = fs::path(dbFile);

    std::stringstream ss;

    ss << Header()
       << fmt::format("<h2>{0}</h2>", db.filename().string());

    ss << "<div class=\"container\">"
       << "<ul class=\"nav justify-content-end\"><li class=\"nav-item\"><button type=\"button\" class=\"nav-link\" data-open-modal=\"AddContenModal\">Add content</buttons></li></ul>";

    for (auto &table : collection.Tables())
    {
        if (table.PrimaryKey().empty())
        {
            continue;
        }

        std::map<std::string, std::string> variables = {
            {"tableName", table.Name()},
            {"tablePrimaryKey", table.PrimaryKey()},
        };

        ss << TemplateUtils::ReplaceVariablesInString(HTDOCS_AGGREGATEROOTHTML, variables);
    }

    ss << HTDOCS_POSTMODALHTML << Footer();

    Ok(ss.str(), request, response);
}

void BadRequest(
    std::string const &err,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response)
{
    (void)request;

    response.SetStatusCode(400);
    response.WriteOutput(err);
    response.CloseOutput();
}

void InternalServerError(
    std::string const &err,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response)
{
    (void)request;

    std::stringstream ss;

    ss << "<html>"
       << "<head>"
       << "<title>Auto Sqlite Rest</title>"
       << "<meta charset=\"utf-8\">"
       << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">"
       << "<link rel=\"icon\" type=\"image/svg+xml\" href=\"data:image/svg+xml;utf8,<svg xmlns=&quot;http://www.w3.org/2000/svg&quot; width=&quot;16&quot; height=&quot;16&quot; fill=&quot;currentColor&quot; class=&quot;bi bi-code-slash&quot; viewBox=&quot;0 0 16 16&quot;><path d=&quot;M10.478 1.647a.5.5 0 1 0-.956-.294l-4 13a.5.5 0 0 0 .956.294l4-13zM4.854 4.146a.5.5 0 0 1 0 .708L1.707 8l3.147 3.146a.5.5 0 0 1-.708.708l-3.5-3.5a.5.5 0 0 1 0-.708l3.5-3.5a.5.5 0 0 1 .708 0zm6.292 0a.5.5 0 0 0 0 .708L14.293 8l-3.147 3.146a.5.5 0 0 0 .708.708l3.5-3.5a.5.5 0 0 0 0-.708l-3.5-3.5a.5.5 0 0 0-.708 0z&quot;/></svg>\">"
       << "</head>"
       << "<body>"
       << "<h1 class=\"display-4 text-center my-4\">Internal Server Error</h1>"
       << fmt::format("<p>{0}</p>", err)
       << "</body"
       << "</html>";

    response.SetStatusCode(500);
    response.WriteOutput(ss.str());
    response.CloseOutput();
}

void NotFoundError(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response,
    const std::smatch &matches)
{
    (void)request;
    (void)matches;

    std::stringstream ss;

    ss << "<html>"
       << "<head>"
       << "<title>Auto Sqlite Rest</title>"
       << "<meta charset=\"utf-8\">"
       << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">"
       << "<link rel=\"icon\" type=\"image/svg+xml\" href=\"data:image/svg+xml;utf8,<svg xmlns=&quot;http://www.w3.org/2000/svg&quot; width=&quot;16&quot; height=&quot;16&quot; fill=&quot;currentColor&quot; class=&quot;bi bi-code-slash&quot; viewBox=&quot;0 0 16 16&quot;><path d=&quot;M10.478 1.647a.5.5 0 1 0-.956-.294l-4 13a.5.5 0 0 0 .956.294l4-13zM4.854 4.146a.5.5 0 0 1 0 .708L1.707 8l3.147 3.146a.5.5 0 0 1-.708.708l-3.5-3.5a.5.5 0 0 1 0-.708l3.5-3.5a.5.5 0 0 1 .708 0zm6.292 0a.5.5 0 0 0 0 .708L14.293 8l-3.147 3.146a.5.5 0 0 0 .708.708l3.5-3.5a.5.5 0 0 0 0-.708l-3.5-3.5a.5.5 0 0 0-.708 0z&quot;/></svg>\">"
       << "</head>"
       << "<body>"
       << "<h1 class=\"display-4 text-center my-4\">Not found Error</h1>"
       << "</body"
       << "</html>";

    response.SetStatusCode(404);
    response.WriteOutput(ss.str());
    response.CloseOutput();
}

static const char zOptions[] =
    "   --listen-url URL     set the url for the http server to listen to\n"
    "                        (default http://localhost:8888/)\n";

std::string showHelp(
    std::string const &exe,
    bool showOptions)
{
    std::stringstream ss;

    ss << "Usage: " << exe << " [OPTIONS] FILENAME [PORT]\n"
       << "FILENAME must be the name of an existing SQLite database.\n";

    if (showOptions)
    {
        ss << "OPTIONS include:\n"
           << zOptions;
    }
    else
    {
        ss << "Use the --help (or -h) option for additional information\n";
    }

    return ss.str();
}
