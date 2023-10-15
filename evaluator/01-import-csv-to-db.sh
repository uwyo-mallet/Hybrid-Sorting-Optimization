#!/usr/bin/env sh

latest_result_dir="$(find /results/ -maxdepth 1 -type d | sort | tail -n 1)"
latest_csv="$(find "$latest_result_dir" -name "*.csv")"

psql -U postgres -d data -c "COPY result from '$latest_csv' DELIMITER ',' CSV HEADER;"
