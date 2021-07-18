# Timer

A simple thread-safe timer util library written in stdandared C++20, for measuring and recording the time spent performing different functions.

```cpp
int main() {
    ubn::timer t("Main thread");
    std::jthread([&] {
        doSomeThing(1s);
        t.setTag("Sub thread", "Thread 1");
    });
    std::jthread([&] {
        doSomeThing(0.5s);
        t.setTag("Sub thread", "Thread 2");
    });
    doSomeThing(0.2s);
    t.setTag("Thread 1", "Thread 2");
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
	typename P = std::chrono::milliseconds
> class timer;
```

## Member functions

- Constructor

If init a timer with a specified `_self_tag_name` (not an empty string `""`), it will print all info history records with the identifier of this tag name after the timer was destructed. The default info history records limit is set to `5` by `_info_history_size`, it should always larger than `0`.

 ```cpp
explicit timer(
    const std::string& _self_tag_name = "timer",
    const std::size_t _info_history_size = 5
);

explicit timer(
    const std::map<std::string, std::chrono::time_point<T>>& _time_point_map,
    const std::string& _self_tag_name,
    const std::size_t _info_history_size
);
 ```

- Operator

Update duration(s) by substract another timer.

```cpp
void operator-(timer& _timer);
```

- General

| Function name                            | Design purpose                                               |
| :--------------------------------------- | :----------------------------------------------------------- |
| `auto setTag(const char* ...)`           | If the tag(s) was not set, then set new tag(s) and initialize duration, else update the tag(s) time and update duration, returns time point |
| `auto getTimePoint(const char*)`         | Get time point by tag name                                   |
| `auto getAllTimePoint()`                 | Get all time point(s), returns time point map                |
| `bool eraseTag(const char* ...)`         | Erase tag(s) by tag name(s), returns `false` if some tag names not found, otherwise is `true` |
| `auto getDuration(const char*)`          | Get duration by tag name                                     |
| `auto getAllDuration()`                  | Get all duration(s), returns duration map                    |
| `auto getInfo(const char*)`              | Get tag info by tag name, returns an unordered map that includes the last updated info |
| `void printInfo(const char* ...)`        | Print last updated info by tag name(s)                       |
| `auto getInfoHistory(const char*)`       | Get tag info history by tag name, returns a deque that includes all the info history of this tag |
| `void printInfoHistory(const char* ...)` | Print tag info history recored(s) by tag name(s), the default size of info history records is `5` |
| `void printAllInfoHistory()`             | Print all info history record(s) for all tag(s)              |
| `bool clearInfoHistory(const char* ...)` | Clear tag(s) info history record(s) by tag name(s), returns `false` if some tag names not found, otherwise is `true` |
| `void clear()`                           | Clear time point map, duration map and info history map      |

## License

MIT License Copyright (c) 2021 Unbinilium.
