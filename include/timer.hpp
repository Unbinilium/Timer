#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <deque>
#include <execution>
#include <iostream>
#include <map>
#include <numeric>
#include <ratio>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

namespace ubn {
    template <typename T>
    concept const_char_pointer = std::is_nothrow_convertible_v<T, const char*>;

    template <
        typename T = std::chrono::high_resolution_clock,
        typename P = std::chrono::milliseconds,
        typename Q = double
    > class timer {
    public:
        constexpr explicit timer(
            const char* _self_tag_name = "timer",
            const std::size_t& _info_history_size = 5
        ) noexcept : m_self_tag_name(_self_tag_name), m_info_history_size(_info_history_size) { setTag(m_self_tag_name); }

        constexpr explicit timer(
            const std::map<std::string, std::chrono::time_point<T>>& _time_point_map,
            const char* _self_tag_name,
            const std::size_t& _info_history_size
        ) noexcept : m_time_point_map(_time_point_map), m_self_tag_name(_self_tag_name), m_info_history_size(_info_history_size) { setTag(m_self_tag_name); }

        constexpr ~timer() noexcept {
            if (std::strlen(m_self_tag_name) != 0) { setTag(m_self_tag_name); }
            printAllInfoHistory();
        }

        constexpr friend std::ostream& operator<<(std::ostream& _os, const timer& _timer) noexcept {
            _timer.printAllInfoHistory();
            return _os;
        }

        constexpr timer& operator<<(const timer& _timer) noexcept {
            const ticket_guard tg(this);
            std::for_each(std::execution::par_unseq, m_time_point_map.begin(), m_time_point_map.end(),
                [_this = this, __timer = &_timer](const auto& _pair) constexpr {
                    const auto key { _pair.first.c_str() };
                    _this->updateInfoHistory(
                        key,
                        std::chrono::duration_cast<P>(_this->m_time_point_map.at(key) - __timer->getTimePoint(key))
                    );
                }
            );
            return *this;
        }

        constexpr auto operator[](const char* _tag_name) const noexcept {
            return getInfo(_tag_name);
        }

        template <const_char_pointer... Args>
        constexpr auto setTag(const Args&... _args) noexcept {
            const ticket_guard tg(this);
            const auto time_point { T::now() };
            ([_this = this, _time_point = &time_point, __args = _args]() constexpr {
                if (_this->m_time_point_map.contains(__args)) {
                    const auto duration { std::chrono::duration_cast<P>(*_time_point - _this->m_time_point_map.at(__args)) };
                    _this->m_time_point_map.at(__args) = *_time_point;
                    _this->updateInfoHistory(__args, std::move(duration));
                } else {
                    _this->m_time_point_map.emplace(__args, *_time_point);
                    _this->initInfoHistory(__args, *_time_point);
                }
            }(), ...);
            return time_point;
        }

        template <const_char_pointer... Args>
        constexpr bool eraseTag(const Args&... _args) noexcept {
            const ticket_guard tg(this);
            return ((eraseTimePoint(_args) && eraseInfoHistory(_args)) & ...);
        }

        constexpr auto getTimePoint(const char* _tag_name) const noexcept {
            const ticket_guard tg(this);
            return m_time_point_map.contains(_tag_name)
                ? m_time_point_map.at(_tag_name)
                : T::now();
        }

        constexpr auto getInfo(const char* _tag_name) const noexcept {
            const ticket_guard tg(this);
            return m_info_history_map.contains(_tag_name)
                ? m_info_history_map.at(_tag_name).back()
                : std::unordered_map<std::string, std::variant<long, Q>>();
        }

        template <const_char_pointer... Args>
        constexpr void printInfo(const Args&... _args) const noexcept {
            const ticket_guard tg(this);
            ((m_info_history_map.contains(_args) && printInfo(_args, m_info_history_map.at(_args).back())), ...);
        }

        constexpr void printAllInfo() const noexcept {
            const ticket_guard tg(this);
            for (const auto& [key, _] : m_info_history_map) {
                printInfo(key.c_str(), m_info_history_map.at(key).back());
            }
        }

        constexpr auto getInfoHistory(const char* _tag_name) const noexcept {
            const ticket_guard tg(this);
            return m_info_history_map.contains(_tag_name)
                ? m_info_history_map.at(_tag_name)
                : std::deque<std::unordered_map<std::string, std::variant<long, Q>>>();
        }

        template <const_char_pointer... Args>
        constexpr void printInfoHistory(const Args&... _args) const noexcept {
            const ticket_guard tg(this);
            ([_this = this, __args = _args]() constexpr {
                if (_this->m_info_history_map.contains(__args)) {
                    for (const auto& info_history : _this->m_info_history_map.at(__args)) {
                        _this->printInfo(__args, info_history);
                    }
                }
            }(), ...);
        }

        constexpr void printAllInfoHistory() const noexcept {
            const ticket_guard tg(this);
            for (const auto& [key, _] : m_info_history_map) {
                for (const auto& info_history : m_info_history_map.at(key)) {
                    printInfo(key.c_str(), info_history);
                }
            }
        }

        template <const_char_pointer... Args>
        constexpr bool clearInfoHistory(const Args&... _args) noexcept {
            const auto time_point { T::now() };
            const ticket_guard tg(this);
            return ((eraseInfoHistory(_args) && initInfoHistory(_args, time_point)) & ...);
        }

        constexpr void clear() noexcept {
            const ticket_guard tg(this);
            m_time_point_map.clear();
            m_info_history_map.clear();
        }

    protected:
        constexpr void initInfoHistory(const char* _tag_name, const std::chrono::time_point<T>& _time_point) noexcept {
            std::unordered_map<std::string, std::variant<long, Q>> info;
            info.emplace("time_point_at", _time_point.time_since_epoch().count());
            for (const auto& key : { "id", "cur_duration", "min_duration", "max_duration" }) { info.emplace(key, 0); }
            for (const auto& key : { "avg_duration", "frequency" }) { info.emplace(key, static_cast<Q>(0)); }
            std::deque<std::unordered_map<std::string, std::variant<long, Q>>> info_history { info };
            m_info_history_map.emplace(_tag_name, std::move(info_history));
        }

        constexpr void updateInfoHistory(const char* _tag_name, const P& _duration) noexcept {
            while (m_info_history_map.at(_tag_name).size() >= m_info_history_size) { m_info_history_map.at(_tag_name).pop_front(); }
            const auto duration_count { _duration.count() };
            auto info { m_info_history_map.at(_tag_name).back() };
            if (std::get<long>(info.at("id")) == 0) {
                for (const auto& key : { "min_duration", "max_duration" }) { info.at(key) = duration_count; }
                info.at("avg_duration") = static_cast<Q>(duration_count);
            } else {
                if (std::get<long>(info.at("min_duration")) > duration_count) { info.at("min_duration") = duration_count; }
                if (std::get<long>(info.at("max_duration")) < duration_count) { info.at("max_duration") = duration_count; }
                info.at("avg_duration") = static_cast<Q>(
                    std::transform_reduce(std::execution::par_unseq, m_info_history_map.at(_tag_name).begin(), m_info_history_map.at(_tag_name).end(),
                        long { 0 },
                        [](const long& _lfs, const long& _rhs) constexpr { return _lfs + _rhs; },
                        [](const std::unordered_map<std::string, std::variant<long, Q>>& _info) constexpr { return std::get<long>(_info.at("cur_duration")); }
                    ) + duration_count
                ) / static_cast<Q>(
                    std::get<long>(info.at("id")) <= static_cast<long>(m_info_history_size - 2)
                        ? m_info_history_map.at(_tag_name).size()
                        : m_info_history_size
                );
            }
            info.at("id") = std::get<long>(info.at("id")) + 1;
            info.at("time_point_at") = m_time_point_map.at(_tag_name).time_since_epoch().count();
            info.at("cur_duration") = duration_count;
            info.at("frequency") = static_cast<Q>(1) / std::chrono::duration<Q, std::ratio<1l>>(_duration).count();
            m_info_history_map.at(_tag_name).push_back(std::move(info));
        }

        constexpr bool printInfo(const char* _tag_name, const std::unordered_map<std::string, std::variant<long, Q>>& _info_history) const noexcept {
            std::cout << "["
                << m_self_tag_name << "] Info '"
                << _tag_name << "' -> "
                << std::get<long>(_info_history.at("id")) << " set at: "
                << std::get<long>(_info_history.at("time_point_at")) << " duration (cur/min/max/avg): "
                << std::get<long>(_info_history.at("cur_duration")) << "/"
                << std::get<long>(_info_history.at("min_duration")) << "/"
                << std::get<long>(_info_history.at("max_duration")) << "/"
                << std::get<Q>(_info_history.at("avg_duration")) << ", frequency: "
                << std::get<Q>(_info_history.at("frequency")) << std::endl;
            return true;
        }

        constexpr bool eraseTimePoint(const char* _tag_name) noexcept {
            if (m_time_point_map.contains(_tag_name)) {
                m_time_point_map.erase(_tag_name);
                return true;
            } else { return false; }
        }

        constexpr bool eraseInfoHistory(const char* _tag_name) noexcept {
            if (m_info_history_map.contains(_tag_name)) {
                m_info_history_map.erase(_tag_name);
                return true;
            } else { return false; }
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
            constexpr ticket_guard(const timer* const _timer) noexcept : m_timer(_timer) { m_timer->lock(); }
            constexpr ~ticket_guard() noexcept { m_timer->unlock(); }
            const timer* const m_timer;
        };

        const char* const m_self_tag_name;
        std::size_t const m_info_history_size;

        std::map<std::string, std::chrono::time_point<T>> m_time_point_map;
        std::map<std::string, std::deque<std::unordered_map<std::string, std::variant<long, Q>>>> m_info_history_map;

        alignas(2 * sizeof(std::max_align_t)) mutable std::atomic<std::size_t> m_ticket_in;
        alignas(2 * sizeof(std::max_align_t)) mutable std::atomic<std::size_t> m_ticket_out;
    };
}
