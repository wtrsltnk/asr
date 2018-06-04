#include <iostream>
#include <string>
#include <memory>

#include "sqlite3/sqlite3.h"
#include "picojson.h"
#include <http/httplistener.h>
#include <http/httplistenerexception.h>
#include <http/httplistenerresponse.h>

class DataQuery
{
public:
    virtual DataQuery &find(std::string const &id);
    virtual DataQuery &top(int amount);
    virtual DataQuery &skip(int amount);
    virtual DataQuery &first();
    virtual DataQuery &filter(std::string const &filter);
    virtual DataQuery &where(std::string const &filter);
    virtual DataQuery &count();
    virtual DataQuery &orderBy(std::string const &field, std::string const &direction);
    virtual DataQuery &orderByDesc(std::string const &field, std::string const &direction);
};

class DataCollection : public DataQuery
{
public:
    void post(picojson::value const &obj);
    void patch(picojson::value const &obj);
    void put(picojson::value const &obj);
    void remove(picojson::value const &obj);

};

void showHelp(std::string const &exe, bool showOptions);

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        showHelp(argv[0], false);
    }

    std::string listenUrl = "http://localhost:8888/";

    for (int i = 0; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h")
        {
            showHelp(argv[0], true);
            return 0;
        }
        if (std::string(argv[i]) == "--listen-url" && ++i < argc)
        {
            listenUrl = argv[i];
        }
    }

    auto exe = std::string(argv[0]);
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
            auto context = std::unique_ptr<System::Net::Http::HttpListenerContext>(listener.GetContext());

            if (context->Request()->RawUrl() == "/quit")
            {
                context->Response()->WriteOutput("<html><body><h1>Ok, i quit</h1></body</html>");
                context->Response()->CloseOutput();
                break;
            }

            if (context->Request()->RawUrl().find(exe) != std::string::npos)
            {
                context->Response()->WriteOutput("<html><body><h1>admin page</h1><nav><a href=\"/quit\">Stop server</a></nav></body</html>");
                context->Response()->CloseOutput();
                continue;
            }

            auto odataVersion = context->Request()->Headers().find("OData-Version");
            if (odataVersion == context->Request()->Headers().end() || odataVersion->second != "4.0")
            {
                context->Response()->SetStatusCode(404);
                context->Response()->SetStatusDescription("Not Found");
                context->Response()->WriteOutput("<html><body><h1>Not Found</h1></body</html>");
                context->Response()->CloseOutput();
                continue;
            }

            if (context->Request()->HttpMethod() == "POST")
            {

            }

            context->Response()->WriteOutput("<html><body><h1>Hello world</h1></body</html>");
            context->Response()->CloseOutput();
        }

        listener.Stop();
    }
    catch (System::Net::Http::HttpListenerException const *ex)
    {
        std::cout << "Exception in http listener: " << ex->Message() << "\n";
    }

    return 0;
}

static const char zOptions[] =
        "   --listen-url URL     set the url for the http server to listen to (default http://localhost:8888/)\n"
        "   --odata-version N    set the version number of OData (default 4)\n"
        ;

void showHelp(std::string const &exe, bool showOptions)
{
    std::cout << "Usage: " << exe << " [OPTIONS] FILENAME [PORT]\n"
              << "FILENAME must be the name of an existing SQLite database.\n";

    if (showOptions)
    {
        std::cout << "OPTIONS include:\n%s" << zOptions;
    }
    else
    {
        std::cout << "Use the --help (or -h) option for additional information\n";
    }
}
