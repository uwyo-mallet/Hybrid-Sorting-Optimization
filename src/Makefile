
all: exec

JAVA = $(shell which java)

exec: jar
	# echo '#!'-jar' > QST-Java
	printf "#!%s -jar\n" $(JAVA) > QST-Java
	cat app/build/libs/app-*-all.jar >> QST-Java
	chmod +x QST-Java

jar:
	./gradlew shadowJar
