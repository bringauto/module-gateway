import os
import logging

from rich.logging import RichHandler

from module_gateway.module_handler import ModuleMQTTHandler


if __name__ == "__main__":
    mqtt_host = os.environ.get("IPC_MQTT_HOST", "127.0.0.1")
    try:
        mqtt_port = int(os.environ.get("IPC_MQTT_PORT", 1883))
    except ValueError:
        logging.error(f"Invalid port, expected int got {os.environ.get('MQTT_PORT')}")
        exit()

    logging.basicConfig(
        level=logging.DEBUG, format="%(message)s", datefmt="[%X]", handlers=[RichHandler()]
    )
    
    module_handler = ModuleMQTTHandler(mqtt_host, mqtt_port)
    logging.info("Starting module MQTT handler loop")
    try:
        module_handler.start_loop()
    except KeyboardInterrupt:
        art = """
            |\__/,|   (`
          _.|o o  |_   ) )
        -(((---(((--------
        """
        logging.info(f"Bye bye\n{art}")
