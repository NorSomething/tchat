# tchat

A lightweight terminal-based LAN chat application written in C. The server multiplexes multiple client connections using `select()`, and the client renders a split-pane TUI using ncurses - all with no external dependencies beyond the standard POSIX socket API.

---

## Demo

https://github.com/user-attachments/assets/c594b420-9b35-472b-8725-e6a4e61fc891

---

## Features

- Multi-client chat over TCP using a single-threaded `select()`-based server
- Terminal UI with separate panes for the chat feed, message input, and online members list
- Username assignment on connect — the server broadcasts an updated online list to all clients whenever someone joins
- Up-arrow key recalls your last sent message in the input box
- Scrollable chat window

---

## Installation

**Requirements**

- GCC (or any C99-compatible compiler)
- ncurses (`libncurses-dev` on Debian/Ubuntu, `ncurses-devel` on Fedora/RHEL)
- A POSIX-compliant OS (Linux, macOS)

**Build**

```bash
# Server
gcc main.c -o server

# Client
gcc client_side.c -o client -lncurses
```

---

## Usage

**Start the server** (listens on port 8080 by default):

```bash
./server
```

**Connect a client** (connects to localhost — edit `inet_addr(...)` in `client_side.c` to target a remote host):

```bash
./client
```

On launch, the client prompts for a username. Once set, anything you type is broadcast to all other connected clients. The members pane on the right updates automatically as users join.

Multiple clients can be run in separate terminal windows on the same machine for local testing.

---

## Project Structure

```
.
├── main.c          # Server — socket setup, select() loop, broadcast logic
└── client_side.c   # Client — ncurses TUI, select() on stdin + socket
```

---

## Todo

- [ ] Add clean removal of client on exit (VV. Imp!!!)
- [ ] Clipboard paste support in the message input box
- [ ] Per-user color coding for chat messages
- [ ] Named text channels (rooms)
- [ ] UI Overhaul
