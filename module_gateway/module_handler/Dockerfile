FROM python:3.11.1

WORKDIR /module_handler
RUN mkdir -p /module_gateway/module_handler
RUN mkdir -p /module_gateway/protobuf

COPY ./requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY ./run_modules.py .
COPY ./module_gateway/module_handler ./module_gateway/module_handler
COPY ./module_gateway/protobuf ./module_gateway/protobuf

CMD [ "python", "./run_modules.py" ]