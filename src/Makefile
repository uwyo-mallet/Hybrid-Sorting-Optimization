
all: exec

exec: jar
	echo '#!/usr/bin/java -jar' > QST-Java
	cat app/build/libs/app-*-all.jar >> QST-Java
	chmod +x QST-Java

jar:
	./gradlew shadowJar
