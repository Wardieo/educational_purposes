#!/usr/bin/env python3
"""
Interactive scan server: send arbitrary JSON commands to a connected client.
"""

import socket
import threading
import json

HOST = "10.56.255.216"
PORT = 9100

def handle_client(conn, addr):
    client_ip = addr[0]
    print(f"[+] client connected: {addr}")
    print(f"[i] Using client IP as scan target: {client_ip}")
    try:
        while True:
            cmd = input("Enter command (e.g. 'scan all', 'scan 80,443', or 'exit'): ").strip()
            if cmd.lower() == "exit":
                break
            if cmd.lower() == "scan all":
                ports = list(range(1, 1025))
            elif cmd.lower().startswith("scan "):
                try:
                    ports = [int(p.strip()) for p in cmd[5:].split(",") if p.strip().isdigit()]
                    if not ports:
                        print("No valid ports specified.")
                        continue
                except Exception as e:
                    print("Invalid port list:", e)
                    continue
            else:
                print("Unknown command.")
                continue

            # Build JSON command with client IP as target
            json_cmd = {
                "task": "scan",
                "target": client_ip,
                "ports": ports
            }
            conn.sendall((json.dumps(json_cmd) + "\n").encode("utf-8"))

            # Receive a JSON result (newline-terminated)
            buf = b""
            while True:
                chunk = conn.recv(4096)
                if not chunk:
                    break
                buf += chunk
                if b"\n" in buf:
                    line, _, buf = buf.partition(b"\n")
                    try:
                        report = json.loads(line.decode("utf-8", errors="replace"))
                        print(f"[<] Report from {addr}:")
                        print(json.dumps(report, indent=2))
                    except Exception as e:
                        print("Failed to parse report:", e)
                    break
    finally:
        conn.close()
        print(f"[-] client disconnected: {addr}")

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  # Add this line
    s.bind((HOST, PORT))
    s.listen(1)
    print(f"[+] server listening on {HOST}:{PORT}")
    try:
        while True:
            conn, addr = s.accept()
            handle_client(conn, addr)
    except KeyboardInterrupt:
        print("Stopping server.")
    finally:
        s.close()

if __name__ == "__main__":
    main()