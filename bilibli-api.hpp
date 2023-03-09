#include <nlohmann/json.hpp>
#include <httplib/httplib.h>
using namespace httplib;
using namespace nlohmann;
namespace bilibili
{
    typedef uint32_t uid_t;
}

namespace bilibili
{
    const Headers headers{{"User-Agent", "有事请联系xukela@qq.com，我会在看到邮件后处理。"}};

    json getUserInfo(bilibili::uid_t uid)
    {
        const char *host = "api.bilibili.com";
        const char *path = "/x/space/acc/info";
        const Params params{{std::string("mid"), std::to_string(uid)}};
        Client cli(host);
        Result res = cli.Get(path, params, headers);
        res.value().body;
        return json::parse(res.value().body);
    }
}