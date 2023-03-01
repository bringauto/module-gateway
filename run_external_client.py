import os
import logging

from rich.logging import RichHandler

from module_gateway.external_client import ExternalClient


if __name__ == '__main__':
    logging.basicConfig(
        level=logging.DEBUG, format="%(message)s", datefmt="[%M:%S.%f]", handlers=[RichHandler()]
    )
    ipc_host = os.environ.get("IPC_MQTT_HOST", "127.0.0.1")
    try:
        ipc_port = int(os.environ.get("IPC_MQTT_PORT", 1883))
    except ValueError:
        logging.error(f"Invalid port in IPC_MQTT_PORT, expected int got {os.environ.get('IPC_MQTT_PORT')}")
        exit()

    external_client = ExternalClient(ipc_host, ipc_port)
    logging.info("Starting External Client")
    external_client.start()
