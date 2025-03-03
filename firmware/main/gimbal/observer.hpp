#pragma once
#include <memory>
#include <vector>

template <typename T>
// 观察者接口
class Observer {
public:
    virtual ~Observer() = default;
    virtual void update(const T &data) = 0;  // 更新 IMU 数据
};


template <typename T>
// 主题接口
class Subject {
public:
    virtual ~Subject() = default;
    virtual void registerObserver(std::shared_ptr<Observer<T>> observer) = 0;
    virtual void removeObserver(std::shared_ptr<Observer<T>> observer) = 0;
    virtual void notifyObservers(const T &data) = 0;  // 通知所有观察者

protected:
    std::vector<std::shared_ptr<Observer<T>>> observers;  // 观察者列表
};
