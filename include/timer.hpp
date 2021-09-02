#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <deque>
#include <execution>
#include <iostream>
#include <map>
#include <numeric>
#include <ratio>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>

namespace ubn {
    template <
        typename Clock = std::chrono::high_resolution_clock,
        typename Unit = std::chrono::milliseconds,
        typename Precision = double
    > class timer {
    public:
        template <std::convertible_to<std::string_view> Arg>
        constexpr explicit timer(
            const Arg& _self_tag_name = "timer",
            const std::size_t& _info_history_size = 5
        ) noexcept : m_self_tag_name(_self_tag_name), m_info_history_size(_info_history_size) { setTag(m_self_tag_name); }

        template <std::convertible_to<std::string_view> Arg>
        constexpr explicit timer(
            const std::map<std::string_view, std::chrono::time_point<Clock>>& _time_point_map,
            const Arg& _self_tag_name,
            const std::size_t& _info_history_size
        ) noexcept : m_time_point_map(_time_point_map), m_self_tag_name(_self_tag_name), m_info_history_size(_info_history_size) { setTag(m_self_tag_name); }

        constexpr ~timer() noexcept {
            if (!m_self_tag_name.empty()) { setTag(m_self_tag_name); }
            printAllInfoHistory();
        }

        constexpr friend std::ostream& operator<<(std::ostream& _os, const timer& _timer) noexcept {
            _timer.printAllInfoHistory();
            return _os;
        }

        constexpr timer& operator<<(const timer& _timer) noexcept {
            const ticket_guard tg(this);
            std::for_each(std::execution::par_unseq, m_time_point_map.cbegin(), m_time_point_map.cend(),
                [_this = this, __timer = &_timer](const auto& _pair) constexpr {
                    const auto key { _pair.first };
                    _this->updateInfoHistory(
                        key,
                        std::chrono::duration_cast<Unit>(_this->m_time_point_map.at(key) - __timer->getTimePoint(key))
                    );
                }
            );
            return *this;
        }

        template <std::convertible_to<std::string_view> Arg>
        constexpr auto operator[](const Arg& _tag_name) const noexcept {
            return getInfo(_tag_name);
        }

        template <std::convertible_to<std::string_view>... Args>
        constexpr auto setTag(const Args&... _args) noexcept {
            const ticket_guard tg(this);
            const auto time_point { Clock::now() };
            ([_this = this, _time_point = &time_point, __args = &_args]() constexpr {
                if (_this->m_time_point_map.contains(*__args)) {
                    const auto duration { std::chrono::duration_cast<Unit>(*_time_point - _this->m_time_point_map.at(*__args)) };
                    _this->m_time_point_map.at(*__args) = *_time_point;
                    _this->updateInfoHistory(*__args, std::move(duration));
                } else {
                    _this->m_time_point_map.emplace(*__args, *_time_point);
                    _this->initInfoHistory(*__args, *_time_point);
                }
            }(), ...);
            return time_point;
        }

        template <std::convertible_to<std::string_view>... Args>
        constexpr bool eraseTag(const Args&... _args) noexcept {
            const ticket_guard tg(this);
            return ((eraseTimePoint(_args) && eraseInfoHistory(_args)) & ...);
        }

        template <std::convertible_to<std::string_view> Arg>
        constexpr auto getTimePoint(const Arg& _tag_name) const noexcept {
            const ticket_guard tg(this);
            return m_time_point_map.contains(_tag_name)
                ? m_time_point_map.at(_tag_name)
                : Clock::now();
        }

        template <std::convertible_to<std::string_view> Arg>
        constexpr auto getInfo(const Arg& _tag_name) const noexcept {
            const ticket_guard tg(this);
            return m_info_history_map.contains(_tag_name)
                ? m_info_history_map.at(_tag_name).back()
                : std::unordered_map<std::string_view, std::variant<long, Precision>>();
        }

        template <std::convertible_to<std::string_view>... Args>
        constexpr void printInfo(const Args&... _args) const noexcept {
            const ticket_guard tg(this);
            ((m_info_history_map.contains(_args) && printInfo(_args, m_info_history_map.at(_args).back())), ...);
        }

        constexpr void printAllInfo() const noexcept {
            const ticket_guard tg(this);
            for (const auto& [key, _] : m_info_history_map) {
                printInfo(key, m_info_history_map.at(key).back());
            }
        }

        template <std::convertible_to<std::string_view> Arg>
        constexpr auto getInfoHistory(const Arg& _tag_name) const noexcept {
            const ticket_guard tg(this);
            return m_info_history_map.contains(_tag_name)
                ? m_info_history_map.at(_tag_name)
                : std::deque<std::unordered_map<std::string_view, std::variant<long, Precision>>>();
        }

        template <std::convertible_to<std::string_view>... Args>
        constexpr void printInfoHistory(const Args&... _args) const noexcept {
            const ticket_guard tg(this);
            ([_this = this, __args = &_args]() constexpr {
                if (_this->m_info_history_map.contains(*__args)) {
                    for (const auto& info_history : _this->m_info_history_map.at(*__args)) {
                        _this->printInfo(*__args, info_history);
                    }
                }
            }(), ...);
        }

        constexpr void printAllInfoHistory() const noexcept {
            const ticket_guard tg(this);
            for (const auto& [key, _] : m_info_history_map) {
                for (const auto& info_history : m_info_history_map.at(key)) {
                    printInfo(key, info_history);
                }
            }
        }

        template <std::convertible_to<std::string_view>... Args>
        constexpr bool clearInfoHistory(const Args&... _args) noexcept {
            const auto time_point { Clock::now() };
            const ticket_guard tg(this);
            return ((eraseInfoHistory(_args) && initInfoHistory(_args, time_point)) & ...);
        }

        constexpr void clear() noexcept {
            const ticket_guard tg(this);
            m_time_point_map.clear();
            m_info_history_map.clear();
        }

    protected:
        template <std::convertible_to<std::string_view> Arg>
        constexpr void initInfoHistory(const Arg& _tag_name, const std::chrono::time_point<Clock>& _time_point) noexcept {
            std::unordered_map<std::string_view, std::variant<long, Precision>> info;
            info.emplace("time_point_at", _time_point.time_since_epoch().count());
            for (const auto& key : { "id", "cur_duration", "min_duration", "max_duration" }) { info.emplace(key, 0); }
            for (const auto& key : { "avg_duration", "frequency" }) { info.emplace(key, static_cast<Precision>(0)); }
            std::deque<std::unordered_map<std::string_view, std::variant<long, Precision>>> info_history { info };
            m_info_history_map.emplace(_tag_name, std::move(info_history));
        }

        template <std::convertible_to<std::string_view> Arg>
        constexpr void updateInfoHistory(const Arg& _tag_name, const Unit& _duration) noexcept {
            while (m_info_history_map.at(_tag_name).size() >= m_info_history_size) { m_info_history_map.at(_tag_name).pop_front(); }
            const auto duration_count { _duration.count() };
            auto info { m_info_history_map.at(_tag_name).back() };
            if (std::get<long>(info.at("id")) != 0) {
                if (std::get<long>(info.at("min_duration")) > duration_count) { info.at("min_duration") = duration_count; }
                if (std::get<long>(info.at("max_duration")) < duration_count) { info.at("max_duration") = duration_count; }
                info.at("avg_duration") = static_cast<Precision>(
                    std::transform_reduce(std::execution::par_unseq, m_info_history_map.at(_tag_name).cbegin(), m_info_history_map.at(_tag_name).cend(),
                        long { 0 },
                        [](const long& _lfs, const long& _rhs) constexpr { return _lfs + _rhs; },
                        [](const std::unordered_map<std::string_view, std::variant<long, Precision>>& _info) constexpr { return std::get<long>(_info.at("cur_duration")); }
                    ) + duration_count
                ) / static_cast<Precision>(
                    std::get<long>(info.at("id")) < static_cast<long>(m_info_history_size - 1)
                        ? m_info_history_map.at(_tag_name).size()
                        : m_info_history_size
                );
            } else {
                for (const auto& key : { "min_duration", "max_duration" }) { info.at(key) = duration_count; }
                info.at("avg_duration") = static_cast<Precision>(duration_count);
            }
            info.at("id") = std::get<long>(info.at("id")) + 1;
            info.at("time_point_at") = m_time_point_map.at(_tag_name).time_since_epoch().count();
            info.at("cur_duration") = duration_count;
            info.at("frequency") = static_cast<Precision>(1) / std::chrono::duration<Precision, std::ratio<1l>>(_duration).count();
            m_info_history_map.at(_tag_name).push_back(std::move(info));
        }

        template <std::convertible_to<std::string_view> Arg>
        constexpr bool printInfo(const Arg& _tag_name, const std::unordered_map<std::string_view, std::variant<long, Precision>>& _info_history) const noexcept {
            std::cout << "["
                << m_self_tag_name << "] Info '"
                << _tag_name << "' -> "
                << std::get<long>(_info_history.at("id")) << " set at: "
                << std::get<long>(_info_history.at("time_point_at")) << " duration (cur/min/max/avg): "
                << std::get<long>(_info_history.at("cur_duration")) << "/"
                << std::get<long>(_info_history.at("min_duration")) << "/"
                << std::get<long>(_info_history.at("max_duration")) << "/"
                << std::get<Precision>(_info_history.at("avg_duration")) << ", frequency: "
                << std::get<Precision>(_info_history.at("frequency")) << std::endl;
            return true;
        }

        template <std::convertible_to<std::string_view> Arg>
        constexpr bool eraseTimePoint(const Arg& _tag_name) noexcept {
            if (m_time_point_map.contains(_tag_name)) {
                m_time_point_map.erase(_tag_name);
                return true;
            } else { return false; }
        }

        template <std::convertible_to<std::string_view> Arg>
        constexpr bool eraseInfoHistory(const Arg& _tag_name) noexcept {
            if (m_info_history_map.contains(_tag_name)) {
                m_info_history_map.erase(_tag_name);
                return true;
            } else { return false; }
        }

    private:
        constexpr void lock() const noexcept {
            const auto ticket { m_ticket_out.fetch_add(1, std::memory_order::acquire) };
            while (true) {
                const auto now { m_ticket_rec.load(std::memory_order::acquire) };
                if (now == ticket) { return; }
                m_ticket_rec.wait(now, std::memory_order::relaxed);
            }
        }

        constexpr void unlock() const noexcept {
            m_ticket_rec.fetch_add(1, std::memory_order::release);
            m_ticket_rec.notify_all();
        }

        struct ticket_guard {
            constexpr ticket_guard(const timer* const _timer) noexcept : m_timer(_timer) { m_timer->lock(); }
            constexpr ~ticket_guard() noexcept { m_timer->unlock(); }
            const timer* const m_timer;
        };

        const std::string_view m_self_tag_name;
        const std::size_t m_info_history_size;

        std::map<std::string_view, std::chrono::time_point<Clock>> m_time_point_map;
        std::map<std::string_view, std::deque<std::unordered_map<std::string_view, std::variant<long, Precision>>>> m_info_history_map;

        alignas(2 * sizeof(std::max_align_t)) mutable std::atomic<std::size_t> m_ticket_out;
        alignas(2 * sizeof(std::max_align_t)) mutable std::atomic<std::size_t> m_ticket_rec;
    };
}
