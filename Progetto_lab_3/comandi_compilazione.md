Compliazione: 

javac -cp "lib/gson-2.10.1.jar" -d out src/common/*.java src/server/*.java src/client/*.java


Esecuzione:

java -cp "out:lib/gson-2.10.1.jar" server.ServerMain

java -cp "out:lib/gson-2.10.1.jar" client.ClientMain
