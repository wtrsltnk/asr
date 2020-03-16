#include "picojson.h"
#include "sqlite3/sqlite3.h"
#include <filesystem>
#include <fmt/format.h>
#include <http/httplistener.h>
#include <http/httplistenerexception.h>
#include <http/httplistenerresponse.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

std::string exe;

std::string Header()
{
    std::stringstream ss;

    ss << "<!doctype html>"
       << "<html lang=\"en\">"
       << "<head>"
       << "<title>Auto Sqlie Rest</title>"
       << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">"
       << "<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css\" integrity=\"sha384-Vkoo8x4CGsO3+Hhxv8T/Q5PaXtkKtu6ug5TOeNV6gBiFeWPGFN9MuhOf23Q9Ifjh\" crossorigin=\"anonymous\">"
       << "<style type=\"text/css\">"
       << "    body{background-color:#ddd;}"
       << "</style>"
       << "</head>"
       << "<body>"
       << "<nav class=\"navbar navbar-expand-lg navbar-dark bg-dark\">"
       << "    <div class=\"container\">"
       << "        <a class=\"navbar-brand\" href=\"/\">ASR</a>"
       << "        <ul class=\"navbar-nav mr-auto\">"
       << "            <li class=\"nav-item\">"
       << "                <a class=\"nav-link\" href=\"/\">Root</a>"
       << "            </li>"
       << "            <li class=\"nav-item\">"
       << fmt::format("                <a class=\"nav-link\" href=\"/{0}\">Admin</a>", exe)
       << "            </li>"
       << "        </ul>"
       << "    </div>"
       << "</nav>"
       << "<div class=\"container\">";

    return ss.str();
}

std::string Footer()
{
    std::stringstream ss;

    ss << "</div>"
       << "<script src=\"https://code.jquery.com/jquery-3.4.1.slim.min.js\" integrity=\"sha384-J6qa4849blE2+poT4WnyKhv5vZF5SrPo0iEjwBvKU7imGFAV0wwj1yYfoRSJoZ+n\" crossorigin=\"anonymous\"></script>"
       << "<script src=\"https://cdn.jsdelivr.net/npm/popper.js@1.16.0/dist/umd/popper.min.js\" integrity=\"sha384-Q6E9RHvbIyZFJoft+2mJbHaEWldlvI9IOYy5n3zV9zzTtmI3UksdQRVvoxMfooAo\" crossorigin=\"anonymous\"></script>"
       << "<script src=\"https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/js/bootstrap.min.js\" integrity=\"sha384-wfSDF2E50Y2D1uUdj0O3uMBJnjuUD4Ih7YwaYd1iqfktj0Uod8GCExl3Og8ifwB6\" crossorigin=\"anonymous\"></script>"
       << "</body>"
       << "</html>";

    return ss.str();
}

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
    std::string _name;
    std::string _primaryKey;
    std::map<std::string, ColumnTypes> _columns;

public:
    DataTable();
    DataTable(
        DataTable const &other);
    DataTable(
        const char *name);

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
    DataTable const &other)
    : _name(other._name),
      _primaryKey(other._primaryKey),
      _columns(other._columns)
{}

DataTable::DataTable(
    const char *name)
    : _name(name)
{}

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
    DataCollection(
        std::string const &db);
    ~DataCollection();

    picojson::value::array get();

    picojson::value post(
        picojson::value const &obj);

    void patch(
        picojson::value const &obj);

    void put(
        picojson::value const &obj);

    void remove(
        picojson::value const &obj);

    inline std::vector<DataTable> const &Tables() const { return _tables; }
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
                    auto table = (const char *)sqlite3_column_text(stmt, i);
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
    sqlite3_prepare_v2(db, fmt::format("PRAGMA table_info({0});", table.Name()).c_str(), -1, &stmt, NULL);

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

picojson::value::array DataCollection::get()
{
    picojson::value::array v;

    for (auto table : _tables)
    {
        picojson::value data;
        picojson::object &o = data.get<picojson::object>();

        o["name"] = picojson::value(table.Name());
        o["primary-key"] = picojson::value(table.PrimaryKey());

        v.push_back(data);
    }

    return v;
}

picojson::value DataCollection::post(
    const picojson::value &obj)
{
    picojson::value v;

    picojson::parse(v, "{\"id\":1}");

    return v;
}

void ParseQuery(
    System::Net::Http::HttpListenerRequest const *request,
    DataQuery &query)
{
}

namespace fs = std::filesystem;

void RouteAdmin(
    std::string const &exe,
    System::Net::Http::HttpListenerContext *context);

void RouteQuit(
    System::Net::Http::HttpListenerContext *context);

void RouteRoot(
    const char *dbFile,
    DataCollection &collection,
    System::Net::Http::HttpListenerContext *context);

void InternalServerError(
    std::string const &err,
    System::Net::Http::HttpListenerContext *context);

std::string showHelp(
    std::string const &exe,
    bool showOptions);

using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

class InstrumentationTimer
{
public:
    InstrumentationTimer(
        const char *name)
        : m_Name(name), m_Stopped(false)
    {
        m_StartTimepoint = std::chrono::steady_clock::now();
    }

    ~InstrumentationTimer()
    {
        if (!m_Stopped)
            Stop();
    }

    void Stop()
    {
        auto endTimepoint = std::chrono::steady_clock::now();
        auto highResStart = FloatingPointMicroseconds{m_StartTimepoint.time_since_epoch()};
        auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

        std::cout << "Rendered " << m_Name << " in " << elapsedTime.count() / 1000.f << "ms" << std::endl;

        m_Stopped = true;
    }

private:
    const char *m_Name;
    std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
    bool m_Stopped;
};

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

        while (true)
        {
            auto context = std::unique_ptr<System::Net::Http::HttpListenerContext>(
                listener.GetContext());

            InstrumentationTimer timer(context.get()->Request()->RawUrl().c_str());

            if (context->Request()->RawUrl() == "/quit")
            {
                RouteQuit(context.get());
                break;
            }

            if (context->Request()->RawUrl().find(exe) != std::string::npos)
            {
                RouteAdmin(exe, context.get());
                continue;
            }

            DataCollection collection(dbFile);

            if (context->Request()->RawUrl() == "/")
            {
                RouteRoot(dbFile, collection, context.get());
                continue;
            }

            ParseQuery(context->Request(), collection);

            picojson::value result;
            if (context->Request()->HttpMethod() == "GET")
            {
                result = picojson::value(collection.get());
            }
            else if (context->Request()->HttpMethod() == "POST")
            {
                picojson::value v;
                std::string err = picojson::parse(v, context->Request()->_payload);
                if (!err.empty())
                {
                    InternalServerError(err, context.get());
                    continue;
                }

                result = collection.post(v);
            }

            context->Response()
                ->SetStatusCode(200);
            context->Response()
                ->SetContentType("application/json");
            context->Response()
                ->WriteOutput(result.serialize(true));
            context->Response()
                ->CloseOutput();
        }

        listener.Stop();
    }
    catch (System::Net::Http::HttpListenerException const *ex)
    {
        std::cout << "Exception in http listener: " << ex->Message() << "\n";
    }

    return 0;
}

void Ok(
    std::string const &content,
    System::Net::Http::HttpListenerContext *context)
{
    context->Response()
        ->SetStatusCode(200);
    context->Response()
        ->WriteOutput(content);
    context->Response()
        ->CloseOutput();
}

void RouteAdmin(
    std::string const &exe,
    System::Net::Http::HttpListenerContext *context)
{
    std::stringstream ss;

    ss << Header()
       << "<h1>admin page</h1>"
       << "<nav><a href=\"/quit\">Stop server</a></nav>"
       << "<div class=\"card p-3\"><pre>" << showHelp(exe, true) << "</pre></div>"
       << "</body>"
       << "</html>";

    Ok(ss.str(), context);
}

void RouteQuit(
    System::Net::Http::HttpListenerContext *context)
{
    std::stringstream ss;

    ss << Header()
       << "<h1>Ok, i quit</h1>"
       << "</body>"
       << "</html>";

    Ok(ss.str(), context);
}

void RouteRoot(
    const char *dbFile,
    DataCollection &collection,
    System::Net::Http::HttpListenerContext *context)
{
    auto db = fs::path(dbFile);

    std::stringstream ss;

    ss << Header()
       << fmt::format("<h1>{0}</h1>", db.filename().string());

    for (auto &table : collection.Tables())
    {
        if (table.PrimaryKey().empty())
        {
            continue;
        }

        ss << "<div class=\"card p-3 mb-3\">";

        ss << "<h3>" << table.Name() << "</h3>";

        ss << "<div class=\"accordion\">";

        ss << "    <div class=\"card\">"
           << "        <button class=\"text-left card-header alert alert-info p-2\" data-toggle=\"collapse\" data-target=\"#" << table.Name() << "_GetById\" aria-expanded=\"true\" aria-controls=\"collapseOne\">"
           << "            <span class=\"badge badge-primary font-weight-bold p-1\" style=\"min-width:60px;\">GET</span>"
           << "            <span class=\"p-1 mx-3 text-monospace\">/" << table.Name() << "/{" << table.PrimaryKey() << "}</span>"
           << "        </button>"
           << "        <div class=\"collapse card-body\" id=\"" << table.Name() << "_GetById\">GetById</div>"
           << "    </div>";

        ss << "    <div class=\"card\">"
           << "        <button class=\"text-left card-header alert alert-success p-2\" data-toggle=\"collapse\" data-target=\"#" << table.Name() << "_Post\" aria-expanded=\"true\" aria-controls=\"collapseOne\">"
           << "            <span class=\"badge badge-success font-weight-bold p-1\" style=\"min-width:60px;\">POST</span>"
           << "            <span class=\"p-1 mx-3 text-monospace\">/" << table.Name() << "</span>"
           << "        </button>"
           << "        <div class=\"collapse card-body\" id=\"" << table.Name() << "_Post\">Post</div>"
           << "    </div>";

        ss << "    <div class=\"card\">"
           << "        <button class=\"text-left card-header alert alert-warning p-2\" data-toggle=\"collapse\" data-target=\"#" << table.Name() << "_Put\" aria-expanded=\"true\" aria-controls=\"collapseOne\">"
           << "            <span class=\"badge badge-warning font-weight-bold p-1\" style=\"min-width:60px;\">PUT</span>"
           << "            <span class=\"p-1 mx-3 text-monospace\">/" << table.Name() << "/{" << table.PrimaryKey() << "}</span>"
           << "        </button>"
           << "        <div class=\"collapse card-body\" id=\"" << table.Name() << "_Put\">Put</div>"
           << "    </div>";

        ss << "    <div class=\"card\">"
           << "        <button class=\"text-left card-header alert alert-danger p-2\" data-toggle=\"collapse\" data-target=\"#" << table.Name() << "_Delete\" aria-expanded=\"true\" aria-controls=\"collapseOne\">"
           << "            <span class=\"badge badge-danger font-weight-bold p-1\" style=\"min-width:60px;\">DELETE</span>"
           << "            <span class=\"p-1 mx-3 text-monospace\">/" << table.Name() << "/{" << table.PrimaryKey() << "}</span>"
           << "        </button>"
           << "        <div class=\"collapse card-body\" id=\"" << table.Name() << "_Delete\">Delete</div>"
           << "    </div>";

        ss << "</div>";

        ss << "</div>";
/*
        for (auto &col : table.Columns())
        {
            ss << col.first;
            if (col.first == table.PrimaryKey())
            {
                ss << " (pk)";
            }
            ss << "<br/>\n";
        }
*/
    }

    ss << Footer();

    Ok(ss.str(), context);
}

void InternalServerError(
    std::string const &err,
    System::Net::Http::HttpListenerContext *context)
{
    std::stringstream ss;

    ss << "<html>"
       << "<body>"
       << "<h1>Internal Server Error</h1>"
       << fmt::format("<p>{0}</p>", err)
       << "</body"
       << "</html>";

    context->Response()
        ->SetStatusCode(500);
    context->Response()
        ->WriteOutput(ss.str());
    context->Response()
        ->CloseOutput();
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
