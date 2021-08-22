# Timer

A simple thread-safe timer util library written in stdandard C++20, for measuring and recording the time spent performing different functions.

```cpp
int main() {
    ubn::timer t("Main ⏱");
    std::jthread([&] {
        doSomeThing(1s);
        t.setTag("Sub thread", "Thread 1");
    });
    std::jthread([&] {
        doSomeThing(0.5s);
        t.setTag("Sub thread", "Thread 2");
    });
    doSomeThing(0.2s);
    t.setTag("Sub thread", "Thread 1", "Thread 2");
}
```

To use timer util library with your project, please clone this repository as git submodule and add these to your `CMakeLists.txt`:

```cmake
add_subdirectory(<time source dir>)
target_link_libraries(<your project name> PUBLIC timer)
```

## Class

Defined in header `timer.hpp`, namespace `ubn`:

```cpp
template <
    typename T = std::chrono::high_resolution_clock,
    typename P = std::chrono::milliseconds,
    typename Q = double
> class timer;
```

`T` is used for specifing the clock type, `P` for specifying the time precision type and `Q` for specifying the time casting unit of (`avg_duration` and `frequency`). The basic data structure of timer are listed as follows:

```cpp
std::map<
    std::string_view,
    std::chrono::time_point<T>
> m_time_point_map;

std::map<
    std::string_view,
    std::deque<std::unordered_map<std::string_view, std::variant<long, Q>>>
> m_info_history_map;
```

All the availiable keys of info history map:

| Type   | Key                                                  |
| ------ | ---------------------------------------------------- |
| `long` | `id`, `cur_duration`, `min_duration`, `max_duration` |
| `Q`    | `avg_duration`, `frequency`                          |


## Member functions

- Constructor

If init a timer with a specified `_self_tag_name` (not an empty string `""`), it will print all info history records with the identifier of this tag name after the timer was destructed. The default info history records limit is set to `5` by `_info_history_size`, it should always larger than `0`.

 ```cpp
template <std::convertible_to<std::string_view> Arg>
explicit timer(
    const Arg& _self_tag_name = "timer",
    const std::size_t& _info_history_size = 5
);

template <std::convertible_to<std::string_view> Arg>
explicit timer(
    const std::map<std::string_view, std::chrono::time_point<T>>& _time_point_map,
    const Arg& _self_tag_name,
    const std::size_t& _info_history_size
);
 ```

- Operator

It easy to call `printAllInfoHistory()` to print all info history record(s) by performing overloads for `std::ostream` with `std::cout` to the timer object.

```cpp
friend std::ostream& operator<<(std::ostream& _os, const timer& _timer);
```

Update duration(s) by substract another timer, returns the reference of this timer.

```cpp
timer& operator<<(const timer& _timer);
```

Use operator `[]` to get the last updated info history record by tag name, alternative to call function `getInfo()` with a single parameter directly.
```cpp
template <std::convertible_to<std::string_view> Arg>
auto operator[](const Arg& _tag_name);
```

- General

| Function name                     | Design purpose                                               |
| :-------------------------------- | :----------------------------------------------------------- |
| `auto setTag(name ...)`           | Set tag(s) by tag name(s), set new tag(s) and initialize info history, else update the tag's time point and update info history, returns time point |
| `bool eraseTag(name ...)`         | Erase tag(s) by tag name(s), returns `false` returns `false` if has at least one tag name is not found, otherwise is `true` |
| `auto getTimePoint(name)`         | Get time point by tag name                                   |
| `auto getInfo(name)`              | Get tag info by tag name, returns an unordered map that includes the last updated info |
| `void printInfo(name ...)`        | Print last updated info by tag name(s)                       |
| `void printAllInfo()`             | Print all last updated info for all tag(s)                   |
| `auto getInfoHistory(name)`       | Get tag info history by tag name, returns a deque that includes all the info history of this tag |
| `void printInfoHistory(name ...)` | Print tag info history recored(s) by tag name(s), the default size of info history records is `5` |
| `void printAllInfoHistory()`      | Print all info history record(s) for all tag(s)              |
| `bool clearInfoHistory(name ...)` | Clear tag(s) info history record(s) by tag name(s), returns `false` if has at least one tag name is not found, otherwise is `true` |
| `void clear()`                    | Clear time point map and info history map                    |

## License

MIT License Copyright (c) 2021 Unbinilium.
