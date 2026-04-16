#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <thread>
#include <map>

// 必要なヘッダーオンリーライブラリ
#include <httplib.h>
#include <json.hpp>
#include "base64.hpp" // https://github.com/ReneNyffenegger/cpp-base64 等を利用

using json = nlohmann::json;

int main() {
    // 1. 環境変数の取得と初期設定
    const char* api_host_env = std::getenv("AWS_LAMBDA_RUNTIME_API");
    if (!api_host_env) {
        std::cerr << "Error: AWS_LAMBDA_RUNTIME_API is not set." << std::endl;
        return 1;
    }
    std::string api_host = api_host_env;

    const char* target_port_env = std::getenv("APP_PORT");
    std::string target_port = target_port_env ? target_port_env : "8080";

    // Lambda Runtime API用クライアント
    httplib::Client runtime_cli(api_host.c_str());
    
    // 背後のWebアプリ用クライアント (localhost)
    httplib::Client app_cli("127.0.0.1", std::stoi(target_port));
    app_cli.set_read_timeout(29, 0); // API Gatewayのタイムアウト(29秒)に合わせる

    std::cout << "Custom C++ Lambda Web Adapter started. Forwarding to port: " << target_port << std::endl;

    // 2. メインのイベントループ
    while (true) {
        // 次のイベントを取得するまでブロックして待機
        auto next = runtime_cli.Get("/2018-06-01/runtime/invocation/next");
        if (!next || next->status != 200) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::string request_id = next->get_header_value("Lambda-Runtime-Aws-Request-Id");
        
        json event;
        try {
            event = json::parse(next->body);
        } catch (...) {
            std::cerr << "Failed to parse JSON event." << std::endl;
            continue;
        }

        // --- リクエストのパースと再構築 ---

        // メソッドの抽出 (API Gateway v1/v2, EventBridge両対応)
        std::string method = event.value("httpMethod", "POST"); // デフォルトはPOST
        if (event.contains("requestContext") && event["requestContext"].contains("http")) {
            method = event["requestContext"]["http"].value("method", method);
        }

        // パスとクエリ文字列の抽出
        std::string path = event.value("path", "/");
        if (event.contains("rawPath")) {
            path = event.value("rawPath", "/");
        }
        std::string query_string = event.value("rawQueryString", "");
        if (!query_string.empty()) {
            path += "?" + query_string;
        }

        // ヘッダーと Content-Type の抽出 (大文字小文字を区別せず処理)
        httplib::Headers headers;
        std::string content_type = "application/json"; // デフォルト
        
        if (event.contains("headers")) {
            for (auto& el : event["headers"].items()) {
                std::string key = el.key();
                // 数値などが来た場合の安全対策として文字列化
                std::string val = el.value().is_string() ? el.value().get<std::string>() : el.value().dump();
                headers.emplace(key, val);

                std::string lower_key = key;
                std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
                if (lower_key == "content-type") {
                    content_type = val;
                }
            }
        }

        // ボディの抽出と Base64 デコード
        std::string body = event.value("body", "");
        if (event.value("isBase64Encoded", false)) {
            body = base64_decode(body);
        }


        // --- 背後のWebアプリへ転送 ---

        httplib::Result res;
        
        // 【重要】EventBridge起動直後のネットワーク準備遅延への対策
        // 接続に失敗した場合は、最大3回（少し待機して）リトライする
        int max_retries = 3;
        for (int i = 0; i < max_retries; ++i) {
            if (method == "GET") { res = app_cli.Get(path.c_str(), headers); }
            else if (method == "POST") { res = app_cli.Post(path.c_str(), headers, body, content_type.c_str()); }
            else if (method == "PUT") { res = app_cli.Put(path.c_str(), headers, body, content_type.c_str()); }
            else if (method == "DELETE") { res = app_cli.Delete(path.c_str(), headers, body, content_type.c_str()); }
            else if (method == "PATCH") { res = app_cli.Patch(path.c_str(), headers, body, content_type.c_str()); }
            else if (method == "OPTIONS") { res = app_cli.Options(path.c_str(), headers); }
            else { res = app_cli.Post(path.c_str(), headers, body, content_type.c_str()); } // Fallback

            // 成功（またはアプリが404等の正当なHTTPエラーを返した）ならループを抜ける
            if (res) break; 
            
            // 接続自体に失敗した場合は少し待ってリトライ
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }


        // --- レスポンスの構築とLambdaへの返却 ---

        json response_json;
        if (res) {
            response_json["statusCode"] = res->status;
            
            // 全てのレスポンスをBase64化して安全に運ぶ
            response_json["body"] = base64_encode(res->body);
            response_json["isBase64Encoded"] = true;
            
            for (const auto& h : res->headers) {
                response_json["headers"][h.first] = h.second;
            }
        } else {
            // リトライしてもダメだった場合 (Webサーバが起動していない等)
            std::cerr << "Failed to connect to local app after retries." << std::endl;
            response_json["statusCode"] = 502;
            response_json["body"] = base64_encode("Bad Gateway (Local server unreachable)");
            response_json["isBase64Encoded"] = true;
        }

        // Lambda Runtime API に結果をPOSTして1イベント完了
        std::string response_url = "/2018-06-01/runtime/invocation/" + request_id + "/response";
        runtime_cli.Post(response_url.c_str(), response_json.dump(), "application/json");
    }

    return 0;
}

