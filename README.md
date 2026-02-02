# Health-track-network

A client-server application for remote patient monitoring using TCP/IP sockets and Multi-threading in C.

## Features
- **Server:** Handles concurrent connections using `pthreads` and protects shared data with Mutex locks.
- **Client:** Graphical User Interface (GUI) built with **GTK+ 3.0**.
- **Real-time Alerts:** Doctors receive instant notifications for critical patient values (Hypoxia, Tachycardia).
- **Protocol:** Custom text-based protocol for Login, Updates, and History retrieval.

