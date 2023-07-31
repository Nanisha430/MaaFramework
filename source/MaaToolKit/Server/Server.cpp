#include "Server.h"
#include "ApiDispatcher.h"
#include "Framework/Init.hpp"

#include <meojson/json.hpp>

using tcp = boost::asio::ip::tcp;

MAA_TOOLKIT_NS_BEGIN

boost::beast::http::message_generator handle_request(
    boost::beast::http::request<boost::beast::http::string_body>&& request)
{
    using namespace boost::beast;

    auto respOk = [&request](string_view json) {
        http::response<http::string_body> res(http::status::ok, request.version());
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "application/json");
        res.content_length(json.size());
        res.keep_alive(request.keep_alive());
        res.body() = json;
        res.prepare_payload();
        return res;
    };

    return respOk(SingletonHolder<ApiDispatcher>::get_instance().handle_route(std::move(request)).to_string());
}

void handle_session(tcp::socket& socket)
{
    using namespace boost::beast;

    error_code ec;
    flat_buffer buf;

    while (true) {
        http::request<http::string_body> request;
        http::read(socket, buf, request, ec);
        if (ec == http::error::end_of_stream) {
            break;
        }
        if (ec) {
            // ERROR
            break;
        }
        http::message_generator msg = handle_request(std::move(request));
        auto keep_alive = msg.keep_alive();
        write(socket, std::move(msg), ec);
        if (ec) {
            // ERROR
            break;
        }
        if (!keep_alive) {
            break;
        }
    }

    socket.shutdown(tcp::socket::shutdown_send, ec);
}

bool HttpServer::start(std::string_view ip, uint16_t port)
{
    if (acceptor) {
        return false;
    }
    auto address = boost::asio::ip::make_address(ip);

    // TODO: 不知道为啥make_shared直接传参匹配不到
    tcp::acceptor acc { ctx, { address, port } };
    acceptor = std::make_shared<tcp::acceptor>(std::move(acc));

    init_maa_framework(SingletonHolder<ApiDispatcher>::get_instance());

    stopping = false;
    dispatcher = std::make_shared<std::thread>([this]() {
        boost::system::error_code ec;
        while (!stopping) {
            tcp::socket socket(ctx);
            acceptor->accept(socket, ec);
            if (ec) {
                break;
            }
            std::thread([](tcp::socket&& sock) { handle_session(sock); }, std::move(socket)).detach();
        }
    });

    return true;
}

bool HttpServer::stop()
{
    stopping = true;
    acceptor->close();
    dispatcher->join();
    acceptor = nullptr;
    dispatcher = nullptr;
    return true;
}

MAA_TOOLKIT_NS_END