# Submit the same jobs across all the partitions.

partitions=("teton" "teton-cascade" "teton-hugemem" "teton-massmem" "teton-knl" "moran")

for i in "${partitions[@]}"; do
  ./run_job.sh "$i"
  sleep 300
done
