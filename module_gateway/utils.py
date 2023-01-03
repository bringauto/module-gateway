import argparse
from enum import Enum


def argparse_init() -> argparse.Namespace:
    parser = argparse.ArgumentParser(prog='Module gateway')
    parser.add_argument('-i', '--ip-address', type=str, default='127.0.0.1', help='ip address of the module gateway')
    parser.add_argument('-p', '--port', type=int, default=8888, help='port of the module gateway')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--comm-error', action=argparse.BooleanOptionalAction, help='raise CommunicationError')
    group.add_argument('--serv-too-long', action=argparse.BooleanOptionalAction, help='raise ServerTookTooLong')
    return parser.parse_args()


class ClientException(Enum):
    CommunicationError = -1
    ServerTookTooLong = -2

    @staticmethod
    def create(args: argparse.Namespace) -> 'ClientException':
        if args.comm_error is True:
            return ClientException.CommunicationError
        if args.serv_too_long is True:
            return ClientException.ServerTookTooLong
        return None
