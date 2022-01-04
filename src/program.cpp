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
    std::string _name;
    std::string _primaryKey;
    std::map<std::string, ColumnTypes> _columns;

public:
    DataTable();
    explicit DataTable(
        DataTable const &other);
    explicit DataTable(
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
    explicit DataCollection(
        std::string const &db);
    ~DataCollection();

    nlohmann::json get();

    nlohmann::json post(
        nlohmann::json const &obj);

    void patch(
        nlohmann::json const &obj);

    void put(
        nlohmann::json const &obj);

    void remove(
        nlohmann::json const &obj);

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

nlohmann::json DataCollection::get()
{
    nlohmann::json v;

    for (auto &table : _tables)
    {
        nlohmann::json data = {
            {"name", table.Name()},
            {"primary-key", table.PrimaryKey()},
        };

        v.push_back(data);
    }

    return v;
}

nlohmann::json DataCollection::post(
    const nlohmann::json &obj)
{
    (void)obj;

    nlohmann::json v = {
        {"id", 1},
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
    System::Net::Http::HttpListenerResponse &response);

void RouteHelp(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response);

void RouteQuit(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response);

void RouteRoot(
    const char *dbFile,
    DataCollection &collection,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response);

void InternalServerError(
    std::string const &err,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response);

std::string showHelp(
    std::string const &exe,
    bool showOptions);

using FloatingPointMicroseconds = std::chrono::duration<double, std::micro>;

class InstrumentationTimer
{
public:
    explicit InstrumentationTimer(
        const char *name)
        : m_Name(name),
          m_StartTimepoint(std::chrono::steady_clock::now()),
          m_Stopped(false)
    {}

    ~InstrumentationTimer()
    {
        if (!m_Stopped)
            Stop();
    }

    void Stop()
    {
        auto endTimepoint = std::chrono::steady_clock::now();
        auto elapsedTime = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch() - std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch();

        std::cout << "Rendered " << m_Name << " in " << elapsedTime.count() / 1000.f << "ms" << std::endl;

        m_Stopped = true;
    }

private:
    const char *m_Name;
    std::chrono::time_point<std::chrono::steady_clock> m_StartTimepoint;
    bool m_Stopped;
};

typedef std::vector<std::pair<std::regex, std::function<void(const System::Net::Http::HttpListenerRequest &request, System::Net::Http::HttpListenerResponse &response)>>> RouteCollection;

class Router
{
public:
    void Get(
        const std::string &pattern,
        std::function<void(const System::Net::Http::HttpListenerRequest &request, System::Net::Http::HttpListenerResponse &response)> handler);

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
    std::function<void(const System::Net::Http::HttpListenerRequest &request, System::Net::Http::HttpListenerResponse &response)> handler)
{
    auto r = std::regex(pattern);

    _getRoutes.push_back(std::make_pair(std::move(r), handler));
}

bool Router::Route(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response)
{
    if (request.HttpMethod() == "GET")
    {
        for (auto &route : _getRoutes)
        {
            if (!std::regex_match(request.RawUrl(), route.first))
            {
                continue;
            }

            route.second(request, response);
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
        router.Get("/", [&dbFile, &collection](const System::Net::Http::HttpListenerRequest &request, System::Net::Http::HttpListenerResponse &response) {
            RouteRoot(dbFile, collection, request, response);
        });

        router.Get("/styles.css", [](const System::Net::Http::HttpListenerRequest &request, System::Net::Http::HttpListenerResponse &response) {
            RouteStatic(HTDOCS_STYLES, "text/css", request, response);
        });

        router.Get("/scripts.js", [](const System::Net::Http::HttpListenerRequest &request, System::Net::Http::HttpListenerResponse &response) {
            RouteStatic(HTDOCS_SCRIPTS, "text/javascript", request, response);
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

            ParseQuery(context->Request(), collection);

            nlohmann::json result;
            if (context->Request()->HttpMethod() == "GET")
            {
                result = nlohmann::json(collection.get());
            }
            else if (context->Request()->HttpMethod() == "POST")
            {
                nlohmann::json v = nlohmann::json::parse(context->Request()->_payload);
                if (v.empty())
                {
                    InternalServerError("error parsing json", *(context->Request()), *(context->Response()));
                    continue;
                }

                result = collection.post(v);
            }

            context->Response()
                ->SetStatusCode(200);
            context->Response()
                ->SetContentType("application/json");
            context->Response()
                ->WriteOutput(result.dump());
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

void RouteStatic(
    const std::string &content,
    const std::string &contentType,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response)
{
    (void)request;

    response.SetContentType(contentType);

    Ok(content, request, response);
}

void RouteHelp(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response)
{
    std::stringstream ss;

    ss << Header()
       << "<h2>Help page</h1>"
       << "<div class=\"container\">"
       << "<ul class=\"nav justify-content-end\"><li class=\"nav-item\"><a class=\"nav-link\" aria-current=\"page\" href=\"/quit\">Stop server</a></li></ul>"
       << "<div class=\"card p-3\"><pre>" << showHelp(exe, true) << "</pre></div>"
       << "</div>"
       << "</body>"
       << "</html>";

    Ok(ss.str(), request, response);
}

void RouteQuit(
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response)
{
    std::stringstream ss;

    ss << "<!doctype html>"
       << "<html lang=\"en\">"
       << "<head>"
       << "<title>Auto Sqlite Rest</title>"
       << "<meta charset=\"utf-8\">"
       << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">"
       << "</head>"
       << "<body>"
       << "<h2>Ok, i quit</h2>"
       << "</body>"
       << "</html>";

    Ok(ss.str(), request, response);

    keepServerRunning = false;
}

void RouteRoot(
    const char *dbFile,
    DataCollection &collection,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response)
{
    auto db = fs::path(dbFile);

    std::stringstream ss;

    ss << Header()
       << fmt::format("<h2>{0}</h2>", db.filename().string());

    ss << "<div class=\"container\">";

    for (auto &table : collection.Tables())
    {
        if (table.PrimaryKey().empty())
        {
            continue;
        }

        ss << "<h3>" << table.Name() << "</h3>";

        ss << "<div class=\"accordion\">";

        ss << "        <button type=\"button\" class=\"endpoint collapsible\">"
           << "            <span class=\"method get\">GET</span>"
           << "            <span>/" << table.Name() << "</span>"
           << "        </button>"
           << "        <div class=\"content\">"
           << "            <div class=\"content-margin\">"
           << "                GetById"
           << "            </div>"
           << "        </div>";

        ss << "        <button type=\"button\" class=\"endpoint collapsible\">"
           << "            <span class=\"method get\">GET</span>"
           << "            <span>/" << table.Name() << "/{" << table.PrimaryKey() << "}</span>"
           << "        </button>"
           << "        <div class=\"content\">"
           << "            <div class=\"content-margin\">"
           << "                GetAll"
           << "            </div>"
           << "        </div>";

        ss << "        <button type=\"button\" class=\"endpoint collapsible\">"
           << "            <span class=\"method post\">POST</span>"
           << "            <span>/" << table.Name() << "</span>"
           << "        </button>"
           << "        <div class=\"content\" id=\"" << table.Name() << "_Post\">"
           << "            <div class=\"content-margin\">"
           << "                Post"
           << "            </div>"
           << "        </div>";

        ss << "        <button type=\"button\" class=\"endpoint collapsible\">"
           << "            <span class=\"method put\">PUT</span>"
           << "            <span>/" << table.Name() << "/{" << table.PrimaryKey() << "}</span>"
           << "        </button>"
           << "        <div class=\"content\" id=\"" << table.Name() << "_Put\">"
           << "            <div class=\"content-margin\">"
           << "                Put"
           << "            </div>"
           << "        </div>";

        ss << "        <button type=\"button\" class=\"endpoint collapsible\">"
           << "            <span class=\"method delete\">DELETE</span>"
           << "            <span>/" << table.Name() << "/{" << table.PrimaryKey() << "}</span>"
           << "        </button>"
           << "        <div class=\"content\" id=\"" << table.Name() << "_Delete\">"
           << "            <div class=\"content-margin\">"
           << "                Delete"
           << "            </div>"
           << "        </div>";

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
        ss << "</div>";
    }

    ss << Footer();

    Ok(ss.str(), request, response);
}

void InternalServerError(
    std::string const &err,
    const System::Net::Http::HttpListenerRequest &request,
    System::Net::Http::HttpListenerResponse &response)
{
    (void)request;

    std::stringstream ss;

    ss << "<html>"
       << "<body>"
       << "<h1 class=\"display-4 text-center my-4\">Internal Server Error</h1>"
       << fmt::format("<p>{0}</p>", err)
       << "</body"
       << "</html>";

    response.SetStatusCode(500);
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
