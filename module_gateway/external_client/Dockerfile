FROM python:3.11.1

WORKDIR /external_client
RUN mkdir -p /module_gateway/external_client
RUN mkdir -p /module_gateway/protobuf
RUN mkdir -p /config

COPY ./requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY ./run_external_client.py .
COPY ./config ./config
COPY ./module_gateway/external_client ./module_gateway/external_client
COPY ./module_gateway/protobuf ./module_gateway/protobuf

CMD [ "python", "./run_external_client.py" ]