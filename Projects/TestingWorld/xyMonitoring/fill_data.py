import time

with open("src.csv", "r") as src:
	content = src.readlines()

for i in range(1570, len(content)):
	with open("run12806.csv", "w") as target:
		target.write("".join(content[:i]))

	time.sleep(0.7)
