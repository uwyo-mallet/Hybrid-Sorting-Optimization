
all: exec

JAVA = $(shell which java)

exec: jar
	printf "#!%s -jar\n" $(JAVA) > QST-java
	cat app/build/libs/QST-Java*.jar >> QST-java
	chmod +x QST-java

jar:
	./gradlew shadowJar

clean:
	./gradlew clean
	rm -f QST-java
