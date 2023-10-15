FROM postgres:latest

ENV POSTGRES_USER postgres
ENV POSTGRES_PASSWORD password
ENV POSTGRES_DB data
ENV PGDATA /tmp/dont-care-delete-me

RUN mkdir -p /tmp/dont-care-delete-me

COPY 00-psql_dump.sql /docker-entrypoint-initdb.d/
COPY 01-import-csv-to-db.sh /docker-entrypoint-initdb.d/
