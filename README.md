# ⚡ linktest

A simple, zero-config tool to test network link throughput between two machines. Just run it on both sides — no setup, no flags, no IP addresses to remember.

## 🔍 The Problem

Testing network throughput between two devices usually means installing tools, configuring servers, remembering IPs and ports, and reading man pages. For a quick "how fast is this link?" check, that's too much friction.

## ✅ What linktest Does

- 📡 Auto-discovers the other machine on your LAN
- 🔄 Decides who sends and who receives (automatically)
- 🚀 Streams TCP traffic for 5 seconds
- 📊 Prints throughput per second and a final average
- 🛡️ Exits cleanly — never hangs

## 🏁 Quick Start

```bash
# Machine A:
./linktest

# Machine B:
./linktest
```

## 🔨 Build

```bash
cmake -B build
cmake --build build
./build/linktest
```

On Windows: `cmake --build build --config Release` → produces `build\Release\linktest.exe`.

Needs CMake 3.10+ and any C11 compiler.

## 💻 Usage

```
linktest                 auto-discover peer, run test
linktest <ip>            skip discovery, test against <ip>
linktest <ip> -s         force this side as receiver
linktest <ip> -c         force this side as sender
```

## 📁 Project Layout

```
linktest/
├── CMakeLists.txt
├── README.md
├── inc/
│   ├── linktest.h       shared defines, function prototypes
│   └── platform.h       OS abstraction (sockets, time)
└── src/
    ├── main.c           entry point, arg parsing, role pick
    ├── platform.c       socket/timer implementations per OS
    ├── discover.c       UDP broadcast peer discovery
    └── tput.c           TCP throughput sender + receiver
```

## ⚙️ How It Works

1. Both sides broadcast a UDP beacon on port 5199
2. When one hears a beacon from a different IP → that's the peer
3. Lower IP becomes receiver, higher IP becomes sender
4. Sender opens TCP to receiver on port 5200
5. Sender blasts 128 KB chunks for 5 seconds
6. Both report throughput per second and final average

## 🔌 Ports

| Port | Proto | What |
|------|-------|------|
| 5199 | UDP   | peer discovery (broadcast) |
| 5200 | TCP   | data transfer |

## 🖥️ Platforms

Works on Linux, macOS, and Windows. The platform layer (`platform.c` / `platform.h`) handles the differences.

## ⚠️ Limitations

- TCP only (no UDP throughput mode yet)
- Broadcast discovery = same subnet only
- Single TCP stream
- No encryption

## 📄 License

MIT
