.PHONY: clean-pyc to-cluster from-cluster
.DEFAULT: to-cluster

current_dir = $(shell pwd)

to-cluster:
	find . -name '*.pyc' -exec rm --force {} +
	find . -name '*.pyo' -exec rm --force {} +
	rsync -auve 'ssh -o StrictHostKeyChecking=no' --delete ${current_dir} jarulsam@teton.arcc.uwyo.edu:/project/mallet/jarulsam/.

from-cluster:
	rsync -auve 'ssh -o StrictHostKeyChecking=no' --delete jarulsam@teton.arcc.uwyo.edu:/project/mallet/jarulsam/quicksort-tuning .

clean-pyc:
	find . -name '*.pyc' -exec rm --force {} +
	find . -name '*.pyo' -exec rm --force {} +
