#!/usr/bin/env python3
import asyncio
import logging
import os

from module_gateway.internal_server import (
    argparse_init,
    ClientException,
    Server
)

from rich.logging import RichHandler


async def main() -> None:
    logging.basicConfig(
        level=logging.DEBUG, format="%(message)s", datefmt="[%X]", handlers=[RichHandler()]
    )
    args = argparse_init()
    ipc_host = os.environ.get("IPC_MQTT_HOST", "127.0.0.1")
    try:
        ipc_port = int(os.environ.get("IPC_MQTT_PORT", 1883))
    except ValueError:
        logging.error(f"Invalid port in IPC_MQTT_PORT, expected int got {os.environ.get('IPC_MQTT_PORT')}")
        exit()

    server = Server(ClientException.create(args))
    try:
        await asyncio.gather(
            server.run_server(args.ip_address, args.port, ipc_host, ipc_port),
            server.listen(ipc_host, ipc_port)
        )
    except KeyboardInterrupt:
        logging.info("Server stopped")


if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logging.info("Server stopped")
        art = """
\|/    \|/
 \    /
  \_/  ___   ___
  o o-'   '''   '
   O -.         |\\
       | |'''| |
        ||   | |
        ||    ||
        "     " """
        logging.info(art)