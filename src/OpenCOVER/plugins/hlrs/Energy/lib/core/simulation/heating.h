#pragma once

#include <map>
#include <string>
#include <vector>


namespace core::simulation::heating {

typedef std::map<std::string, std::vector<double>> Data;

class Base {
 public:
  Base(const ::std::string &name, const Data &data = {})
      : m_name(name), m_data(data) {};
  virtual ~Base() = default;

  const auto &getName() const { return m_name; }

  auto &getData() { return m_data; }
  const auto &getData() const { return m_data; }

  void addData(const std::string &key, const std::vector<double> &value) {
    m_data[key] = value;
  }

  void addData(const std::string &key, const double &value) {
    m_data[key].push_back(value);
  }

 private:
  std::string m_name;
  Data m_data;  // timestep data
};

// Producer and Consumer are derived from Base => probably will differ more in the future
// if heating sim is final
class Producer : public Base {
 public:
  Producer(const std::string &name, const Data &data = {}) : Base(name, data) {}
};

class Consumer : public Base {
 public:
  Consumer(const std::string &name, const Data &data = {}) : Base(name, data) {}
};

class HeatingSimulation {
 public:
  HeatingSimulation() = default;

  void addConsumer(const std::string &name, const Data &data = {}) {
    addBase(m_consumers, name, data);
  }

  void addProducer(const std::string &name, const Data &data = {}) {
    addBase(m_producers, name, data);
  }

  void addDataToConsumer(const std::string &nameOfConsumer, const std::string &key,
                         const std::vector<double> &value) {
    addDataToBase(m_consumers, nameOfConsumer, key, value);
  }

  void addDataToConsumer(const std::string &nameOfConsumer, const std::string &key,
                         const double &value) {
    addDataToBase(m_consumers, nameOfConsumer, key, value);
  }

  void addDataToProducer(const std::string &nameOfProducer, const std::string &key,
                         const std::vector<double> &value) {
    addDataToBase(m_producers, nameOfProducer, key, value);
  }

  void addDataToProducer(const std::string &nameOfProducer, const std::string &key,
                         const double &value) {
    addDataToBase(m_producers, nameOfProducer, key, value);
  }

  void addData(const std::string &key, const std::vector<double> &value) {
    m_data[key] = value;
  }

  void addData(const std::string &key, const double &value) {
    m_data[key].push_back(value);
  }

  auto &getData() { return m_data; }
  const auto &getConsumers() const { return m_consumers; }
  const auto &getProducers() const { return m_producers; }

 private:
  template <typename U>
  void addBase(std::map<std::string, U> &baseMap, const std::string &name,
               const Data &data = {}) {
    static_assert(std::is_base_of_v<Base, U>, "U must be derived from Base");
    baseMap.insert({name, {name, data}});
  }

  template <typename U, typename... Args>
  void addDataToBase(std::map<std::string, U> &baseMap, const std::string &name,
                     Args &&...args) {
    static_assert(std::is_base_of_v<Base, U>, "U must be derived from Base");
    if (auto it = baseMap.find(name); it != baseMap.end())
      it->second.addData(std::forward<Args>(args)...);
  }

  std::map<std::string, Producer> m_producers;
  std::map<std::string, Consumer> m_consumers;
  Data m_data;
};
}  // namespace core::simulation::heating
