FROM python:3.11.1

WORKDIR /internal_server
RUN mkdir -p /module_gateway/internal_server
RUN mkdir -p /module_gateway/protobuf

COPY ./requirements.txt .
RUN pip install -r requirements.txt

COPY ./run_internal_server.py .
COPY ./module_gateway/internal_server ./module_gateway/internal_server
COPY ./module_gateway/protobuf ./module_gateway/protobuf

CMD [ "python3", "./run_internal_server.py", "-i", "0.0.0.0" ]