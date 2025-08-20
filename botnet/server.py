import socket

def main():
    host = '0.0.0.0'
    port = 9001

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(1)
    print(f"Server listening on {host}:{port}")

    conn, addr = server.accept()
    print(f"Client connected from {addr}")

    while True:
        cmd = input("Enter command (PRINT <text> or MSGBOX <text>, or EXIT): ")
        conn.sendall(cmd.encode())
        if cmd.strip().upper() == "EXIT":
            break
        resp = conn.recv(1024).decode()
        print(f"Client response: {resp}")

    conn.close()
    server.close()

if __name__ == "__main__":
    main()