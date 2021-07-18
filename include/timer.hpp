#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace ubn {
   namespace timer_concept {
        template <typename T>
        concept const_char_pointer = std::is_nothrow_convertible_v<T, const char*>;
   }

    template <typename T = std::chrono::high_resolution_clock, typename P = std::chrono::milliseconds>
    class timer {
    public:
        constexpr explicit timer(
            const std::string& _self_tag_name = "timer",
            const std::size_t _info_history_size = 5
        ) noexcept : m_self_tag_name(_self_tag_name), m_info_history_size(_info_history_size) {
            setTag(m_self_tag_name.c_str());
        }

        constexpr explicit timer(
            const std::map<std::string, std::chrono::time_point<T>>& _time_point_map,
            const std::string& _self_tag_name,
            const std::size_t _info_history_size
        ) noexcept : m_time_point_map(_time_point_map), m_self_tag_name(_self_tag_name), m_info_history_size(_info_history_size) {}

        constexpr ~timer() noexcept {
            if (!m_self_tag_name.empty()) {
                setTag(m_self_tag_name.c_str());
            }
            printAllInfoHistory();
        }

        constexpr void operator-(timer& _timer) noexcept {
            const ticket_guard tg(this);
            for (const auto& [key, _] : m_time_point_map) {
                const auto duration {
                    std::chrono::duration_cast<P>(m_time_point_map[key] - _timer.getTimePoint(key.c_str()))
                };
                updateInfoHistory(key.c_str(), std::move(duration));
            }
        }

        constexpr auto setTag(const char* _tag_name) noexcept {
            const auto time_point { T::now() };
            const ticket_guard tg(this);
            if (m_time_point_map.contains(_tag_name)) {
                const auto duration {
                    std::chrono::duration_cast<P>(time_point - m_time_point_map[_tag_name])
                };
                m_duration_map.insert_or_assign(_tag_name, duration);
                m_time_point_map[_tag_name] = time_point;
                updateInfoHistory(_tag_name, std::move(duration));
            } else {
                m_time_point_map.emplace(_tag_name, time_point);
                initInfoHistory(_tag_name, time_point);
            }
            return time_point;
        }

        template <timer_concept::const_char_pointer Arg, timer_concept::const_char_pointer... Args>
        constexpr auto setTag(const Arg& _arg, const Args&... _args) noexcept {
            setTag(_arg);
            return setTag(std::forward<const Args>(_args)...);
        }

        constexpr auto getTimePoint(const char* _tag_name) noexcept {
            const ticket_guard tg(this);
            return m_time_point_map.contains(_tag_name)
                ? m_time_point_map[_tag_name]
                : T::now();
        }

        constexpr auto getAllTimePoint() noexcept {
            const ticket_guard tg(this);
            return m_time_point_map;
        }

        constexpr bool eraseTag(const char* _tag_name) noexcept {
            const ticket_guard tg(this);
            return eraseTimePoint(_tag_name) &&
                eraseDuration(_tag_name) &&
                eraseInfoHistory(_tag_name);
        }

        template <timer_concept::const_char_pointer Arg, timer_concept::const_char_pointer... Args>
        constexpr bool eraseTag(const Arg& _arg, const Args&... _args) noexcept {
            return eraseTag(_arg) & eraseTag(std::forward<const Args>(_args)...);
        }

        constexpr auto getDuration(const char* _tag_name) noexcept {
            const ticket_guard tg(this);
            return m_duration_map.contains(_tag_name)
                ? m_duration_map[_tag_name]
                : P();
        }

        constexpr auto getAllDuration() noexcept {
            const ticket_guard tg(this);
            return m_duration_map;
        }

        constexpr auto getInfo(const char* _tag_name) noexcept {
            const ticket_guard tg(this);
            return m_info_history_map.contains(_tag_name)
                ? m_info_history_map[_tag_name].back()
                : std::unordered_map<std::string, double>();
        }

        constexpr void printInfo(const char* _tag_name) noexcept {
            const ticket_guard tg(this);
            if (m_info_history_map.contains(_tag_name)) {
                printInfo(_tag_name, m_info_history_map[_tag_name].back());
            }
        }

        template <timer_concept::const_char_pointer Arg, timer_concept::const_char_pointer... Args>
        constexpr void printInfo(const Arg& _arg, const Args&... _args) noexcept {
            printInfo(_arg);
            printInfo(std::forward<const Args>(_args)...);
        }

        constexpr void printAllInfo() noexcept {
            const ticket_guard tg(this);
            printAllInfo(m_duration_map);
        }

        constexpr auto getInfoHistory(const char* _tag_name) noexcept {
            const ticket_guard tg(this);
            return m_info_history_map.contains(_tag_name)
                ? m_info_history_map[_tag_name]
                : std::deque<std::unordered_map<std::string, double>>();
        }

        constexpr void printInfoHistory(const char* _tag_name) noexcept {
            const ticket_guard tg(this);
            if (m_info_history_map.contains(_tag_name)) {
                for (auto& info_history : m_info_history_map[_tag_name]) {
                    printInfo(_tag_name, info_history);
                }
            }
        }

        template <timer_concept::const_char_pointer Arg, timer_concept::const_char_pointer... Args>
        constexpr void printInfoHistory(const Arg& _arg, const Args&... _args) noexcept {
            printInfoHistory(_arg);
            printInfoHistory(std::forward<const Args>(_args)...);
        }

        constexpr void printAllInfoHistory() noexcept {
            const ticket_guard tg(this);
            printAllInfoHistory(m_duration_map);
        }

        constexpr bool clearInfoHistory(const char* _tag_name) noexcept {
            const ticket_guard tg(this);
            const auto status { eraseInfoHistory(_tag_name) };
            if (status) { initInfoHistory(_tag_name, T::now()); }
            return std::move(status);
        }

        template <timer_concept::const_char_pointer Arg, timer_concept::const_char_pointer... Args>
        constexpr bool clearInfoHistory(const Arg& _arg, const Args&... _args) noexcept {
            return clearInfoHistory(_arg) & clearInfoHistory(std::forward<const Args>(_args)...);
        }

        constexpr void clear() noexcept {
            const ticket_guard tg(this);
            m_time_point_map.clear();
            m_duration_map.clear();
            m_info_history_map.clear();
        }

    protected:
        constexpr void initInfoHistory(const char* _tag_name, const std::chrono::time_point<T>& _time_point) noexcept {
            std::unordered_map<std::string, double> info;
            info.emplace("time_point_at", _time_point.time_since_epoch().count());
            for (const auto& key : { "id", "cur_duration", "min_duration", "max_duration", "avg_duration", "frequency" }) {
                info.emplace(key, 0.);
            }
            std::deque<std::unordered_map<std::string, double>> info_history { info };
            m_info_history_map.emplace(_tag_name, std::move(info_history));
        }

        constexpr void updateInfoHistory(const char* _tag_name, const P& _duration) noexcept {
            const auto duration_count { _duration.count() };
            auto info { m_info_history_map[_tag_name].back() };
            if (static_cast<std::size_t>(info["id"]) == 0) {
                for (const auto& key : { "min_duration", "max_duration", "avg_duration" }) {
                    info[key] = duration_count;
                }
            } else {
                if (info["min_duration"] > duration_count) {
                    info["min_duration"] = duration_count;
                }
                if (info["max_duration"] < duration_count) {
                    info["max_duration"] = duration_count;
                }
                info["avg_duration"] = (info["avg_duration"] + duration_count) / 2.;
            }
            info["id"] += 1.;
            info["time_point_at"] = m_time_point_map[_tag_name].time_since_epoch().count();
            info["cur_duration"] = duration_count;
            info["frequency"] = 1. / std::chrono::duration<double, std::ratio<1>>(_duration).count();
            while (m_info_history_map[_tag_name].size() >= m_info_history_size) {
                m_info_history_map[_tag_name].pop_front();
            }
            m_info_history_map[_tag_name].push_back(std::move(info));
        }

        constexpr void printInfo(const char* _tag_name, std::unordered_map<std::string, double>& _info_history) noexcept {
            std::cout << "["
                << m_self_tag_name << "] Info '"
                << _tag_name << "' -> "
                << static_cast<std::size_t>(_info_history["id"]) << " set at: "
                << static_cast<std::size_t>(_info_history["time_point_at"]) << " duration (cur/min/max/avg): "
                << _info_history["cur_duration"] << "/"
                << _info_history["min_duration"] << "/"
                << _info_history["max_duration"] << "/"
                << _info_history["avg_duration"] << ", frequency: "
                << _info_history["frequency"] << std::endl;
        }

        constexpr void printAllInfo(const std::map<std::string, P>& _duration_map) noexcept {
            for (const auto& [key, _] : _duration_map) {
                if (m_info_history_map.contains(key)) {
                    printInfo(key.c_str(), m_info_history_map[key].back());
                }
            }
        }

        constexpr void printAllInfoHistory(const std::map<std::string, P>& _duration_map) noexcept {
            for (const auto& [key, _] : _duration_map) {
                for (auto& info_history : m_info_history_map[key]) {
                    printInfo(key.c_str(), info_history);
                }
            }
        }

        constexpr bool eraseTimePoint(const char* _tag_name) noexcept {
            if (m_time_point_map.contains(_tag_name)) {
                m_time_point_map.erase(_tag_name);
                return true;
            } else {
                return false;
            }
        }

        constexpr bool eraseDuration(const char* _tag_name) noexcept {
            if (m_duration_map.contains(_tag_name)) {
                m_duration_map.erase(_tag_name);
                return true;
            } else {
                return false;
            }
        }

        constexpr bool eraseInfoHistory(const char* _tag_name) noexcept {
            if (m_info_history_map.contains(_tag_name)) {
                m_info_history_map.erase(_tag_name);
                return true;
            } else {
                return false;
            }
        }

    private:
        constexpr void lock() const noexcept {
            const auto ticket { m_ticket_in.fetch_add(1, std::memory_order::acquire) };
            while (true) {
                const auto now { m_ticket_out.load(std::memory_order::acquire) };
                if (now == ticket) { return; }
                m_ticket_out.wait(now, std::memory_order::relaxed);
            }
        }

        constexpr void unlock() const noexcept {
            m_ticket_out.fetch_add(1, std::memory_order::release);
            m_ticket_out.notify_all();
        }

        struct ticket_guard {
            constexpr ticket_guard(timer* const _timer) noexcept : m_timer(_timer) { m_timer->lock(); }
            constexpr ~ticket_guard() noexcept { m_timer->unlock(); }
            timer* const m_timer { nullptr };
        };

        std::string const m_self_tag_name;
        std::size_t const m_info_history_size;

        std::map<std::string, std::chrono::time_point<T>> m_time_point_map;
        std::map<std::string, P> m_duration_map;
        std::map<std::string, std::deque<std::unordered_map<std::string, double>>> m_info_history_map;

        alignas(2 * sizeof(std::max_align_t)) mutable std::atomic<std::size_t> m_ticket_in;
        alignas(2 * sizeof(std::max_align_t)) mutable std::atomic<std::size_t> m_ticket_out;
    };
}
