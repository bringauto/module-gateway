import asyncio
import logging

from module_gateway import Server
from module_gateway import argparse_init, ClientException

from rich.logging import RichHandler


def main() -> None:
    logging.basicConfig(
        level=logging.DEBUG, format="%(message)s", datefmt="[%X]", handlers=[RichHandler()]
    )
    args = argparse_init()
    server = Server(ClientException.create(args))
    try:
        asyncio.run(server.run_server(args.ip_address, args.port))
    except KeyboardInterrupt:
        logging.info("Server stopped")

def todo_testing_delete() -> None:
    from module_gateway.external_client import ExternalClient
    import module_gateway.protobuf.InternalProtocol_pb2 as InternalProtocolProtob
    status_to_send = InternalProtocolProtob.DeviceStatus()
    status_to_send.statusData = b'some byte data'
    device = InternalProtocolProtob.Device()
    device.module = 10
    device.deviceType = 3
    device.deviceRole = "left"
    device.deviceName = "button"
    status_to_send.device.CopyFrom(device)
    
    status_to_send2 = InternalProtocolProtob.DeviceStatus()
    status_to_send2.statusData = b'some byte data'
    device2 = InternalProtocolProtob.Device()
    device2.module = 10
    device2.deviceType = 3
    device2.deviceRole = "left"
    device2.deviceName = "button"
    status_to_send2.device.CopyFrom(device2)
    
    external_client = ExternalClient()
    external_client.send_status(status_to_send)
    external_client.send_status(status_to_send2)


if __name__ == '__main__':
    #todo_testing_delete()
    main()
