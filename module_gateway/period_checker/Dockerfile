FROM python:3.11.1

WORKDIR /period_checker
RUN mkdir -p /module_gateway/protobuf

COPY ./requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY ./period_checker.py .
COPY ./module_gateway/protobuf ./module_gateway/protobuf

CMD [ "python", "./period_checker.py", "-p", "1.0" ]