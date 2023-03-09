#include <fstream>
#include <mutex>
#include "bilibli-api.hpp"
#include <argparse/argparse.hpp>
using namespace std;

mutex m;                  // 写文件保护，当手动结束进程时先写完文件再退出
bool spider_exit = false; // 当此值为true时，start()退出
thread *spider;
string out_dir;
string user_name = "user.json";
string err_name = "err.txt";
string log_name = "log.txt";
uint32_t delay;
bilibili::uid_t suid; // 起始uid
bilibili::uid_t euid; // 结束uid,包括在内
ofstream *fuser;
ofstream *ferr;
ofstream *flog;

void start();
void destory();
string getCurrTime();
void signal_handle(int sign);
void addLog(const string &content);
void init(const util::argparser &args);
void addErr(bilibili::uid_t uid, const json &data);
void saveFile(bilibili::uid_t uid, const json &data);

int main(int argc, char const *argv[])
{
    auto args = util::argparser("哔哩哔哩爬虫程序");
    args.set_program_name("哔哩哔哩爬虫")
        .add_help_option()
        .add_argument<int64_t>("uid", "起始UID")
        .add_option<int64_t>("-e", "--end", "结束UID（包括此UID），默认：0", 0)
        .add_option("-d", "--delay", "爬虫延迟，默认：5秒", 5)
        .add_option<string>("-o", "--out-dir", "输出目录，默认：out", "out")
        .parse(argc, argv);
    init(args);
    cout
        << "爬虫开始\n起始UID：" << suid << endl
        << "结束UID：" << euid << endl
        << "输出目录：" << out_dir << endl
        << "延迟时间：" << delay << " s\n\n"
        << endl;
    spider = new thread(start);
    spider->join();
    destory();
    exit(EXIT_SUCCESS);
}

void signal_handle(int sign)
{
    cout << "发现信号： " << sign << endl;
    m.lock();
    switch (sign)
    {
    case SIGINT:
        cerr << "手动中断，即将退出" << endl;
        spider_exit = true;
        break;

    default:
        cerr << "未知信号" << endl;
        break;
    }
    m.unlock();
}

string getCurrTime()
{
    std::time_t now = std::time(nullptr);
    std::tm *local_time = std::localtime(&now);
    std::string time_str;
    time_str.resize(19);
    std::strftime((char *)time_str.c_str(), sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_time);
    return time_str;
}

void saveFile(bilibili::uid_t uid, const json &data)
{
    if (fuser->is_open())
    {
        string _data = nlohmann::to_string(data["data"]);
        fuser->write(_data.c_str(), _data.length());
        fuser->put('\n');
        fuser->flush();
        cout << getCurrTime() << "  用户编号：" << data["data"]["mid"] << "  用户名：" << data["data"]["name"] << endl;
    }
    else
    {
        string con = "保存用户信息出错 " + out_dir + user_name + "未打开";
        addLog(con);
        cerr << con << endl;
        exit(EXIT_FAILURE);
    }
}

void addErr(bilibili::uid_t uid, const json &data)
{
    string message(to_string(uid));
    message += "\t";
    message.append(data["message"]);
    cerr << message << endl;
    ferr->write(message.c_str(), message.length());
    ferr->write("\n\n", 2);
    ferr->flush();
}

void addLog(const string &content)
{
    const string &&time = getCurrTime();
    flog->write(time.c_str(), time.length());
    flog->put('\n');
    flog->write(content.c_str(), content.length());
    (*flog) << "\n\n";
    flog->flush();
}

void init(const util::argparser &args)
{
    // 加载参数
    delay = args.get_option<int>("-d");
    suid = args.get_argument<int64_t>("uid");
    euid = args.get_option<int64_t>("-e");
    out_dir = args.get_option_string("-o");
    if (*(out_dir.rbegin()) != '/')
        out_dir.append("/");

    // 开始初始化
    mkdir(out_dir.c_str(), mode_t(0755));
    fuser = new ofstream(out_dir + user_name);
    ferr = new ofstream(out_dir + err_name);
    flog = new ofstream(out_dir + log_name, ios::app);

    signal(SIGINT, signal_handle);
}

void destory()
{
    string con("正常退出，当前uid： ");
    con.append(to_string(suid));
    addLog(con);
    cout << con << endl;
    fuser->close();
    ferr->close();
    flog->close();
    delete fuser, ferr, flog;
}

void start()
{
    suid--;
    while (suid != euid or suid == 0)
    {
        try
        {
            m.lock();
            if (spider_exit)
            {
                m.unlock();
                return;
            }
            else
            {
                m.unlock();
                time_t t1 = time(nullptr);
                suid++;
                const json &&info = bilibili::getUserInfo(suid);
                info["code"] == 0 ? saveFile(suid, info) : addErr(suid, info);
                time_t t2 = time(nullptr);
                cout << "IO延迟： " << t2 - t1 << " 秒" << endl;
                sleep(delay);
            }
        }
        catch (const std::exception &e)
        {
            m.unlock();
            addLog(string("出错了：\n") + e.what());

            std::cerr << "出错了：" << endl
                      << time << endl
                      << e.what() << '\n';
            exit(EXIT_FAILURE);
        }
    }
}