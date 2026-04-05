# Code Analysis Report

## Project Overview

This project implements a TCP/HTTP server framework in C++ using Boost.ASIO, with support for HTTP/1.1, Server-Sent Events (SSE), JSON commands, and email sending via Postmark. The codebase is reasonably structured but has several significant issues ranging from critical bugs to design concerns.

---

## 1. Critical Bugs

### 1.1 — SSE Dead Socket Leak (`SSEEventLoop.cpp`)

```cpp
// In SSEEventLoop::_init()
for (auto& _s : _subscribers.data())
    _server->sendAsSSE(_s, data.dump());
```

**Problem:** When a subscriber disconnects, `sendAsSSE` returns `false` to signal the dead connection, but the loop completely ignores this return value. Dead sockets are never removed from the `_subscribers` list. Over time, the list grows indefinitely with dead connections, causing resource exhaustion and write errors on every event emission.

**Fix:** Check the return value and remove dead subscribers:

```cpp
auto& subs = _subscribers.data();
for (int i = subs.size() - 1; i >= 0; i--)
{
    bool alive = _server->sendAsSSE(subs[i], data.dump());
    if (!alive)
        subs.removeByIndex(i);
}
```

---

### 1.2 — `sendAsSSE` String Overload Ignores Error Code (`HttpServer.cpp`)

```cpp
bool HttpServer::sendAsSSE(std::shared_ptr<tcp::socket> socket, const std::string& data)
{
    boost::system::error_code ec;
    auto s = network::sse_formated("message", data, "");
    s += "\n";
    this->write(socket, s); // <-- does NOT pass 'ec'
    return !(ec == boost::asio::error::eof || ...);
}
```

**Problem:** The `write()` call here uses the overload that takes no `error_code`, so `ec` is never populated. The function always returns `true` even when the connection is broken, making dead socket detection impossible for string-based SSE.

**Fix:** Use the correct `write` overload:

```cpp
this->write(socket, s, ec);
```

---

### 1.3 — HTTP Response Sent Even After SSE Handling (`HttpServer.cpp`)

```cpp
if (httpdata["path"] == "/sse")
{
    // ... handles SSE ...
    // NO return statement after the non-unique-loop SSE handling
}

// Falls through to here regardless
for (const auto& f : httpresponses)
{
    auto res = f(httpdata);
    // Sends an HTTP response on top of the already-sent SSE stream
    ...
}
```

**Problem:** After handling the `/sse` path with the `_onSSE` callback, execution falls through to the standard HTTP response loop. The server will attempt to write a regular HTTP response on a socket that has already been used as an SSE stream, corrupting the connection.

**Fix:** Add a `return` statement after the SSE block:

```cpp
if (httpdata["path"] == "/sse")
{
    if (!_sseUniqueLoop)
    {
        if (onsse)
        {
            auto hds = network::sse_headers();
            this->write(s, hds);
            onsse(s, httpdata);
        }
    }
    else
    {
        _sseUniqueLoop->addSubscriber(s);
    }
    return; // <-- critical
}
```

---

### 1.4 — `TcpClient::send` Buffer Over-Read (`TcpClient.cpp`)

```cpp
for (int i=0; i<_read_buffer_size; i++)
    _res += buf[i];
```

**Problem:** This loop always appends exactly `_read_buffer_size` (4096) bytes, regardless of how many bytes were actually read (`len`). The tail of the buffer contains uninitialized or old data, corrupting every response received.

**Fix:**
```cpp
_res.append(buf.data(), len);
```

---

## 2. Thread Safety Issues

### 2.1 — `_sseUniqueLoop` Accessed Without Synchronization (`HttpServer.cpp`)

```cpp
// In handleSocket (called from multiple threads in async mode)
if (!_sseUniqueLoop)          // read
{
    ...
}
else
{
    _sseUniqueLoop->addSubscriber(s); // read
}
```

**Problem:** `_sseUniqueLoop` is a `std::unique_ptr` that is read from multiple threads simultaneously (since `handleSocket` runs in a thread pool in async mode) and written once in `createUniqueSSELoop()`. Without synchronization, this is a data race and undefined behavior.

**Fix:** Set `_sseUniqueLoop` before the server starts, or protect it with an `std::atomic<bool>` flag or mutex.

---

### 2.2 — `_subscribers` Locked While Writing Over Network (`SSEEventLoop.cpp`)

```cpp
std::lock_guard lk(_subscribers);
for (auto& _s : _subscribers.data())
    _server->sendAsSSE(_s, data.dump()); // potentially slow, blocking network I/O
```

**Problem:** The lock is held during network I/O. If sending to a slow or dead client blocks, no new subscribers can be added or removed during this time, stalling the entire SSE system.

**Fix:** Copy the subscriber list under the lock, then release it before performing I/O:

```cpp
std::vector<std::shared_ptr<tcp::socket>> snapshot;
{
    std::lock_guard lk(_subscribers);
    snapshot = _subscribers.data().toVector(); // or equivalent copy
}
for (auto& s : snapshot)
    _server->sendAsSSE(s, data.dump());
```

---

### 2.3 — `network::_onDec` and `_signalConnectionMade` Are Not Thread-Safe (`network.cpp`)

```cpp
std::vector<std::function<void()>> _onDec;
bool _signalConnectionMade = false;
```

**Problem:** These are plain global variables modified by `addOnDeconnection` and `makeSignalConnection`, which could be called from multiple threads. No synchronization is present.

**Fix:** Use `std::mutex` or `std::atomic<bool>` for `_signalConnectionMade`, and a mutex-protected vector for `_onDec`.

---

## 3. Design and Architecture Issues

### 3.1 — `_onSSE` Stored as `th::Safe` but Checked with `operator bool` on a `std::function` Assigned `0`

```cpp
_onSSE.data() = 0; // assigned integer 0, relies on implicit conversion to null std::function
...
if (onsse) // checked for validity
```

**Problem:** While this technically works because `std::function` can be assigned `nullptr` (and `0` converts to `nullptr` for function pointers), using `0` to represent "no function" is confusing and fragile. It reads as if an integer is being assigned to a function, which makes the intent unclear.

**Fix:** Use `nullptr` explicitly:

```cpp
_onSSE.data() = nullptr;
```

---

### 3.2 — Hard-Coded `/sse` Route

```cpp
if (httpdata.find("path") != httpdata.end() && httpdata["path"] == "/sse")
```

**Problem:** The SSE route is hard-coded. This prevents users from customizing the SSE endpoint or having multiple SSE streams. The header comment itself acknowledges this: `//TODO : add a set of different paths for sse in a map`.

**Fix:** Make the SSE path configurable via a setter, or use a map of paths to SSE handlers, consistent with how `_pathsFuncs` works.

---

### 3.3 — `CommandsManager` (`_cmds`) Locked During Command Registration and Copy

```cpp
std::lock_guard lk(_cmds);
auto cmd = _cmds.data().createCommand<JsonCommand>(path);
```

Then later:

```cpp
std::lock_guard lk(_cmds);
cmd_cp = *cmd;
```

**Problem:** The lock on `_cmds` is acquired every time a command is executed (to copy it). This is a global lock across all command executions. In a high-throughput async server, this becomes a bottleneck. Commands should ideally be registered once and then read without locking after startup.

**Fix:** Use a `std::shared_mutex` with `shared_lock` for reading (command execution) and exclusive lock only for writing (registration).

---

### 3.4 — `network::send` Uses a Shared Temporary File (`network.cpp`)

```cpp
std::string tmppath = files::tmp() + files::sep() + "data_to_send.json";
files::write(tmppath, data.dump());
```

**Problem:** The file is always written to the same fixed path. In a concurrent environment (async server calling `network::send` from multiple threads), two simultaneous calls will corrupt each other's data.

**Fix:** Use a unique temporary filename per call, for example by appending a timestamp or UUID:

```cpp
std::string tmppath = files::tmp() + files::sep() + "data_to_send_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json";
```

And ensure cleanup after the process call.

---

### 3.5 — `HttpServer` Constructor Registers a Lambda That Captures `this` in `_httpResponses`

```cpp
auto main_cb = [this](std::unordered_map<std::string, std::string>& httpdata) -> std::string { ... };
_httpResponses.data().push(main_cb);
```

**Problem:** This is fine as long as the server is not moved or copied after construction. However, `_httpResponses` supports multiple callbacks, implying the architecture intends future extension. The current single-callback design mixed with a vector suggests an incomplete or over-engineered abstraction. Currently only one callback is ever added, making the `Vec` unnecessary.

---

## 4. Code Quality and Robustness Issues

### 4.1 — Silent JSON Parse Failure (`HttpServer.cpp`)

```cpp
try
{
    j = json::parse(httpdata["content"]);
}
catch(const std::exception& e){} // silently swallowed
```

**Problem:** If the body is malformed JSON, the function returns an empty object `{}` with no indication that parsing failed. Callers cannot distinguish between an intentionally empty body and a malformed one.

**Fix:** At minimum, log the error. Better, return a `std::optional<json>` or set a flag:

```cpp
catch(const std::exception& e)
{
    lg("JSON parse error: " + std::string(e.what()));
}
```

---

### 4.2 — `uri::decode` Is Inefficient and Incorrect for Case-Sensitivity

```cpp
for (auto const &[key, val] : refs)
    data = str::replace(data, key, val);
```

**Problem 1 — Performance:** This iterates through all ~150 entries in the map for every decode call, calling `str::replace` each time. For a server handling many requests, this is very slow.

**Problem 2 — Case sensitivity:** URI percent-encoding is case-insensitive per RFC 3986 (i.e., `%2F` and `%2f` are equivalent). The current implementation only handles uppercase hex digits, so `%2f`, `%3a`, etc., will not be decoded.

**Fix:** Implement a proper single-pass percent-decode algorithm:

```cpp
std::string decode(const std::string& input)
{
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i)
    {
        if (input[i] == '%' && i + 2 < input.size())
        {
            int hex = std::stoi(input.substr(i + 1, 2), nullptr, 16);
            result += static_cast<char>(hex);
            i += 2;
        }
        else if (input[i] == '+')
            result += ' ';
        else
            result += input[i];
    }
    return result;
}
```

---

### 4.3 — `TcpServer::handleSocket` Loops Forever on Non-EOF Errors

```cpp
while(!error)
{
    ...
    if (error == boost::asio::error::eof) { break; }
    else if (error) { _execOnError(...); break; }
    ...
}
```

**Problem:** While the `break` statements are present, errors are reported via `_execOnError` but do not propagate. This is acceptable, but the error callback is often not set in practice (no enforcement), so errors are silently discarded.

---

### 4.4 — `Content-Length` Calculation May Underflow (`HttpServer.cpp`)

```cpp
auto lengthToRead = length + realHeaderSize;
lengthToRead -= headers.size();
if (lengthToRead > 0)
    auto rest = this->readSocket(s, lengthToRead);
```

**Problem:** `lengthToRead` is computed as a signed or unsigned integer difference. If `headers.size()` is larger than `length + realHeaderSize` (e.g., due to a partial body already being in the first read buffer), this could underflow if the type is unsigned, wrapping to a very large number, causing an enormous read attempt.

**Fix:** Use signed arithmetic and clamp to zero:

```cpp
long long lengthToRead = (long long)(length + realHeaderSize) - (long long)headers.size();
if (lengthToRead > 0)
    auto rest = this->readSocket(s, (size_t)lengthToRead);
```

---

### 4.5 — Postmark Token Hard-Coded in Source (`emails.cpp`)

```cpp
std::string _token = "6580dba3-56f1-474f-a266-e227b49b801c";
```

**Problem:** A real API token is committed directly in the source code. This is a serious security vulnerability if this code is in a version-controlled repository. Even if the token has been revoked, this practice should never be established.

**Fix:** Load the token from an environment variable or a configuration file that is excluded from version control:

```cpp
std::string _token = "";

void init()
{
    const char* t = std::getenv("POSTMARK_TOKEN");
    if (t) _token = t;
}
```

---

### 4.6 — `http_formated` Uses `\n` Instead of `\r\n` (`network.cpp`)

```cpp
std::string r = "HTTP/1.1 " + std::to_string(serverCode) + " OK\n" +
"Content-type: " + contentType + "\n";
```

**Problem:** The HTTP/1.1 specification (RFC 7230) mandates `\r\n` (CRLF) as the line terminator for headers. Using bare `\n` (LF) is technically non-compliant and may cause issues with strict HTTP clients or proxies.

**Fix:** Replace all `\n` with `\r\n` in the HTTP header formatting:

```cpp
std::string r = "HTTP/1.1 " + std::to_string(serverCode) + " OK\r\n" +
"Content-type: " + contentType + "\r\n";
...
r += "\r\n" + res;
```

---

### 4.7 — Missing `Content-Length` Header in HTTP Responses

**Problem:** The `http_formated` function never includes a `Content-Length` header. While chunked or connection-close responses can work without it, omitting `Content-Length` forces clients to read until the connection closes, preventing HTTP keep-alive and degrading performance significantly.

**Fix:**

```cpp
r += "Content-Length: " + std::to_string(res.size()) + "\r\n";
```

---

## 5. Summary Table

| # | Severity | Location | Issue |
|---|----------|----------|-------|
| 1.1 | **Critical** | `SSEEventLoop.cpp` | Dead socket subscribers never removed |
| 1.2 | **Critical** | `HttpServer.cpp` | SSE string overload never reads error code |
| 1.3 | **Critical** | `HttpServer.cpp` | HTTP response sent after SSE handling |
| 1.4 | **Critical** | `TcpClient.cpp` | Buffer over-read, appends garbage data |
| 2.1 | **High** | `HttpServer.cpp` | `_sseUniqueLoop` data race in async mode |
| 2.2 | **High** | `SSEEventLoop.cpp` | Lock held during blocking network I/O |
| 2.3 | **High** | `network.cpp` | Global state not thread-safe |
| 3.4 | **High** | `network.cpp` | Shared temp file causes race condition |
| 4.5 | **High** | `emails.cpp` | API token hard-coded in source |
| 4.6 | **Medium** | `network.cpp` | HTTP headers use `\n` instead of `\r\n` |
| 4.7 | **Medium** | `network.cpp` | Missing `Content-Length` in responses |
| 4.2 | **Medium** | `uri.cpp` | Decode is slow and case-insensitive |
| 4.4 | **Medium** | `HttpServer.cpp` | Potential integer underflow in body read |
| 3.1 | **Low** | `HttpServer.cpp` | `nullptr` assigned as integer `0` |
| 3.2 | **Low** | `HttpServer.cpp` | Hard-coded `/sse` route |
| 3.3 | **Low** | `HttpServer.cpp` | Command execution lock is a bottleneck |
| 4.1 | **Low** | `HttpServer.cpp` | Silent JSON parse failure |
| 4.3 | **Low** | `TcpServer.cpp` | Errors silently discarded if no handler set |
