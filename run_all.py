import subprocess


def main():
    internal_server = subprocess.Popen(['python3', 'run_internal_server.py'])
    modules = subprocess.Popen(['python3', 'run_modules.py'])
    external_client = subprocess.Popen(['python3', 'run_external_client.py'])
    try:
        internal_server.wait()
    except KeyboardInterrupt:
        internal_server.terminate()
        modules.terminate()
        external_client.terminate()

if __name__ == '__main__':
    main()
