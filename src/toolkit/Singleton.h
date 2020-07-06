#pragma once

#include <list>
#include <functional>

// 使用范例

/*
class A
{
public:
    void Test() {
    }
}

typedef CSingleton<A> AInst;

void Fun() {
    AInst::Instance().Test();
}

*/

template<typename T>
class CSingleton
{
private:
    class CHolder
    {
    public:
        CHolder() {
            s_alive = true;
            s_created = true;
        }

        ~CHolder() {
            s_alive = false;
        }

    public:
        // 当前是否存活
        static bool s_alive;
        // 是否创建过
        static bool s_created;

    public:
        T inst;
    };

public:
    // 单例对象
    static T& singleton() {
        static CHolder holder;
        return holder.inst;
    }

    // 是否已经被释放
    inline static bool alive() {
        return CHolder::s_alive;
    }

    // 是否创建过
    inline static bool created() {
        return CHolder::s_created;
    }

private:
    CSingleton() {}
    ~CSingleton() {}

private:
    CSingleton(const CSingleton&) = delete;
    CSingleton& operator=(const CSingleton&) = delete;
};

template <typename T>
bool CSingleton<T>::CHolder::s_alive = false;

template <typename T>
bool CSingleton<T>::CHolder::s_created = false;

// 使用范例

/*
class A
{
public:
    bool onServerInit() {
        return true;
    }

    void onServerReady() {
    }

    void onServerExit() {
    }
}

class B
{
public:
    bool onServerInit() {
        return true;
    }

    void onServerReady() {
    }

    void onServerExit() {
    }
}

typedef CSingleton<B> BInst;

int main() {

    // 添加单例类
    SigSingletonInit.add<A>();
    SigSingletonInit.add<B>();

    // 服务器初始化
    std::string error;
    if (!SigSingletonInit.onServerInit(error)) {
        return -1;
    }

    // 服务器准备完毕
    SigSingletonInit.onServerReady();

    // 服务器释放
    SigSingletonInit.onServerExit();

    return 0;
}

*/

class CSingletonInitiator
{
public:
    friend class CServerSchedule;

    // 添加进来的单例类，必须实现onServerInit，onServerReady，onServerExit三个成员函数
    // 注意！！！ 请在IAppDelegate的init函数执行完毕之前调用
    template <typename T>
    void add() {
        m_list.push_back(Info());
        Info& info = m_list.back();

        info.name = typeid(T).name();

        info.init = []() -> bool {
            if (CSingleton<T>::alive() || !CSingleton<T>::created()) {
                return CSingleton<T>::singleton().onServerInit();
            }
            return false;
        };

        info.ready = []() {
            if (CSingleton<T>::alive() || !CSingleton<T>::created()) {
                CSingleton<T>::singleton().onServerReady();
            }
        };

        info.exit = []() {
            if (CSingleton<T>::alive() || !CSingleton<T>::created()) {
                CSingleton<T>::singleton().onServerExit();
            }
        };
    }

private:
    // 服务器初始化时期，对所有单例进行初始化
    bool onServerInit(std::string& error) {
        for (auto& info : m_list) {
            if (!info.init()) {
                error = "Singleton " + info.name + " Init Failed!";
                return false;
            }
        }
        return true;
    }

    // 服务器准备完毕时期，对所有单例进行准备，这个是为了解决，单例之间相互数据依赖
    void onServerReady() {
        for (auto& info : m_list) {
            info.ready();
        }
    }

    // 服务器退出时期，对所有单例数据进行释放
    void onServerExit() {
        for (auto& info : m_list) {
            info.exit();
        }
    }

private:
    struct Info
    {
        std::string name;
        std::function<bool()> init;
        std::function<void()> ready;
        std::function<void()> exit;
    };
    std::list<Info> m_list;
};

typedef CSingleton<CSingletonInitiator> SingletonInitiator;
#define SigSingletonInit SingletonInitiator::singleton()