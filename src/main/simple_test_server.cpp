//#define I_USE_MYSQL 1

#include <httplib.h>
#include <fstream>
#include <iostream>
#include <string>

#ifdef I_USE_MYSQL
#include <mysql.h>
#endif

#include"httplib.h"
#include"json.hpp"
#ifdef _WIN32
#include<windows.h>
#endif


using json = nlohmann::json;
using namespace httplib;


#define API_CALL(func) \
		std::string auth = get_auth(req); \
        json o_req = get_request(req); \
        json o_res;\
		try{o_res = func(o_req,auth); }catch(...){}\
        std::string str_res = get_response(o_res); \
        res.set_content(str_res, "application/json"); \
		common_response(req, res)

inline static void common_response(const Request& req, Response& res) {
	res.set_header("Access-Control-Allow-Origin", "*");
	res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
	res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

inline static std::string trim(std::string src, std::string tr=" ", uint8_t flag = 3)
{
	size_t start_pos, end_pos;
	start_pos = ((flag == 1 || flag == 3) ? src.find_first_not_of(tr) : 0);
	end_pos = ((flag == 2 || flag == 3) ? src.find_last_not_of(tr) : src.size());
	if (start_pos == -1 || end_pos == -1)return "";
	return src.substr(start_pos, end_pos - start_pos + 1);
}

inline static std::string get_auth(const Request& req)
{
	std::string ret = "error";
	try {
		if (!req.has_header("Authorization"))return ret;

		std::string val = req.get_header_value("Authorization");
		int idx = val.find("Bearer");
		if (idx == -1)return ret;

		std::string tmp = val.substr(idx + 6);
		tmp = trim(tmp);
		ret = tmp;
	}
	catch (...) {}
	return ret;
}

inline static json get_request(const Request& req)
{
	json ret;
	if (req.method == "GET")return ret;
    try {
        ret = json::parse(req.body);
    }
    catch (...) {}
	return ret;
}

inline static std::string get_response(json & res)
{
	std::string ret;
    try {
        ret = res.dump(4);
    }
    catch (...) {}
	return ret;
}


static json api_empty(json& req, std::string auth = "") {

    json res;
    return res;
}

static json api_hello(json& req, std::string auth = "") {

    json res;
    fprintf(stderr,"##### /api/hello from stderr\n");
    res = json::object();
    res["status"] = "success";
    res["hello"] = "hello from cpp";
    return res;
}


static json api_test(json& req, std::string auth = "") {

    json ret;
    fprintf(stderr,"##### /api/test from stderr\n");


    httplib::Client cli("127.0.0.1:8080");
    cli.set_keep_alive(false);
    auto res = cli.Get("/api/hello");
	printf("res=%s\n",res->body.c_str());

    ret = json::object();
    ret["status"] = "success";
    ret["hello"] = "hello from cpp";
    return ret;
}

static void api_image(const Request& req, Response& res) {
    std::string image_path = "images.jpg";
    std::ifstream file(image_path, std::ios::binary);
    if (!file) {
        res.status = 404;
        res.set_content("Image not found", "text/plain");
        return;
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    res.set_content(content, "image/jpeg");
    res.status = 200;
}


#ifdef I_USE_MYSQL
static json api_mysql(json& req, std::string auth = "") {

    json r;
    fprintf(stderr,"##### /api/mysql from stderr\n");

    const char* localhost = "rds.amazonaws.com";
    const char* your_user = "user";
    const char* your_password = "pass";
    const char* your_db = "dbname";
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    int ct = 0;

    conn = mysql_init(NULL);

    r = json::object();
    r["status"] = "fail";

    if (!mysql_real_connect(conn, localhost, your_user, your_password, 
                            your_db, 0, NULL, 0)) {
        fprintf(stderr, "接続失敗: %s\n", mysql_error(conn));
        return r;
    }

    if (mysql_query(conn, "SELECT id, user FROM user")) {
        fprintf(stderr, "クエリ失敗: %s\n", mysql_error(conn));
        return r;
    }

    res = mysql_store_result(conn);
    r["data"] = json::array();

    while ((row = mysql_fetch_row(res)) != NULL) {
        r["data"][ct++] = row[1];
        fprintf(stderr,"ID: %s, NAME: %s\n", row[0], row[1]);
    }

    mysql_free_result(res);
    mysql_close(conn);

    r["status"] = "success";
    return r;
}
#endif

int main()
{
	Server svr;
	int ret;
	int port = 8080;
	char* p;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

	if (!svr.is_valid()) {
		printf("server has an error...\n");
		return -1;
	}
	// file
	ret = svr.set_mount_point("/html", "html");


    svr.Get("/api/hello", [](const Request& req, Response& res) {
        API_CALL(api_hello);
        });
    svr.Options("/api/hello", [](const Request& req, Response& res) {
        API_CALL(api_empty);
        });

    svr.Get("/api/test", [](const Request& req, Response& res) {
        API_CALL(api_test);
        });
    svr.Options("/api/test", [](const Request& req, Response& res) {
        API_CALL(api_empty);
        });
    svr.Get("/api/image", [](const Request& req, Response& res) {
        api_image(req, res);
        });
    svr.Options("/api/image", [](const Request& req, Response& res) {
        API_CALL(api_empty);
        });

#ifdef I_USE_MYSQL
    svr.Get("/api/mysql", [](const Request& req, Response& res) {
        API_CALL(api_mysql);
        });
    svr.Options("/api/mysql", [](const Request& req, Response& res) {
        API_CALL(api_empty);
        });
#endif
    if (p = getenv("POST")) {
		port = std::atoi(p);
	}

	fprintf(stderr,"http server started port=%d\n",port);
	svr.listen("0.0.0.0", port);

	return 0;
}
